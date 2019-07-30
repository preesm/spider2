/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
 *
 * Spider is a dataflow based runtime used to execute dynamic PiSDF
 * applications. The Preesm tool may be used to design PiSDF applications.
 *
 * This software is governed by the CeCILL  license under French law and
 * abiding by the rules of distribution of free software.  You can  use,
 * modify and/ or redistribute the software under the terms of the CeCILL
 * license as circulated by CEA, CNRS and INRIA at the following URL
 * "http://www.cecill.info".
 *
 * As a counterpart to the access to the source code and  rights to copy,
 * modify and redistribute granted by the license, users are provided only
 * with a limited warranty  and the software's author,  the holder of the
 * economic rights,  and the successive licensors  have only  limited
 * liability.
 *
 * In this respect, the user's attention is drawn to the risks associated
 * with loading,  using,  modifying and/or developing or reproducing the
 * software by the user in light of its specific status of free software,
 * that may mean  that it is complicated to manipulate,  and  that  also
 * therefore means  that it is reserved for developers  and  experienced
 * professionals having in-depth computer knowledge. Users are therefore
 * encouraged to load and test the software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */

/* === Include(s) === */

#include <memory/Allocator.h>
#include <graphs/pisdf/PiSDFGraph.h>
#include <graphs/pisdf/PiSDFVertex.h>
#include <spider-api/pisdf.h>
#include "SRDAGTransformer.h"

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */


SRDAGTransformer::SRDAGTransformer(const PiSDFGraph *graph) : piSdfGraph_{graph} {

    srdag_ = Spider::allocate<PiSDFGraph>(StackID::PISDF);
    Spider::construct(srdag_,
                      "srdag-" + graph->name(),
                      0, /* = nActors = */
                      0, /* = nEdges = */
                      0, /* = nParams = */
                      0, /* = nInputInterfaces = */
                      0, /* = nOutputInterfaces = */
                      0  /* = nConfigActors = */);
}

SRDAGTransformer::~SRDAGTransformer() {
    if (srdag_) {
        Spider::destroy(srdag_);
        Spider::deallocate(srdag_);
    }
}

void SRDAGTransformer::execute() {
    if (!piSdfGraph_) {
        throwSpiderException("Cannot transform nullptr PiSDFGraph.");
    }

    /* == Extract the vertices from the top graph == */
    extractAndLinkActors(piSdfGraph_);

    /* == Iterate over the subgraphs == */
//    for (const auto *subgraph : piSdfGraph_->subgraphs()) {
//
//    }
}

void SRDAGTransformer::resume() {
    if (stoppedFromConfig_) {

    }
}

/* === Private method(s) === */

PiSDFVertex *SRDAGTransformer::copyVertex(const PiSDFVertex *vertex, std::uint32_t instance) {
    auto *copyVertex = Spider::allocate<PiSDFVertex>(StackID::TRANSFO);
    Spider::construct(copyVertex,
                      StackID::TRANSFO,
                      srdag_,
                      std::string(vertex->name()) + "_" + std::to_string(instance),
                      vertex->type(),
                      vertex->nEdgesIN(),
                      vertex->nEdgesOUT(),
                      vertex->nParamsIN(),
                      vertex->nParamsOUT());
    copyVertex->setRepetitionValue(1);

    /* == Copy the input parameter == */
    std::uint32_t ix = 0;
    for (auto *param : vertex->inputParams()) {
        copyVertex->setInputParam(param, ix);
        ix += 1;
    }
    return copyVertex;
}

void SRDAGTransformer::extractConfigActors(const PiSDFGraph *graph) {
    /* == Pre-cache (if needed) the number of config actors of current graph == */
    srdag_->precacheConfigVertices(graph->nConfigs());

    /* == Copy all config actors == */
    for (const auto *vertex : graph->configActors()) {
        copyVertex(vertex);
    }
}

void SRDAGTransformer::extractAndLinkActors(const PiSDFGraph *graph) {
    /* == Pre-cache (if needed) == */
    srdag_->precacheVertices(graph->nVertices());
    srdag_->precacheEdges(graph->nVertices());

    /* == Array to keep track of who has been done == */
    Spider::Array<Spider::Array<PiSDFVertex *>> vertex2Vertex{StackID::TRANSFO, graph->nVertices()};

    /* == Copy all vertices == */
    for (const auto *vertex : graph->vertices()) {
        Spider::construct(&vertex2Vertex[vertex->getIx()], StackID::TRANSFO, vertex->repetitionValue());
        for (std::uint32_t i = 0; i < vertex->repetitionValue(); ++i) {
            vertex2Vertex[vertex->getIx()][i] = copyVertex(vertex, i);
        }
    }

    /* == Do the linkage == */
    for (const auto *edge : graph->edges()) {
        auto linker = SRLinker{edge, vertex2Vertex};

        /* == Do the linkage == */
        if (linker.delay) {
            /* == Delay linkage == */
            delayLinkage(linker);
        } else {
            /* == No delay linkage == */
            nodelayLinkage(linker);
        }

        /* == Check that everything has been linked == */
        if (linker.sinkCount != linker.sink->repetitionValue()) {
            throwSpiderException("Remaining %"
                                         PRIu32
                                         " instances of %s to link.",
                                 linker.sink->repetitionValue() - linker.sinkCount,
                                 linker.sink->name().c_str());
        }
        if (linker.sourceCount != linker.source->repetitionValue()) {
            throwSpiderException("Remaining %"
                                         PRIu32
                                         " instances of %s to link.",
                                 linker.source->repetitionValue() - linker.sourceCount,
                                 linker.source->name().c_str());
        }
    }

    /* == Free memory of the arrays == */
    for (auto &array : vertex2Vertex) {
        Spider::destroy(&array);
    }
}

void SRDAGTransformer::nodelayLinkage(SRDAGTransformer::SRLinker &linker) {
    if (linker.sourceRate == linker.sinkRate) {
        /* == Forward case == */
        forwardLinkage(linker);
    } else if (linker.sourceRate < linker.sinkRate) {
        /* == Join patterns == */
//        joinPatternLinkage(linker);
        delayedJoinPatternLinkage(linker);
    } else {
        /* == Fork patterns == */
//        forkPatternLinkage(linker);
        delayedForkPatternLinkage(linker);
    }
}

void SRDAGTransformer::forwardLinkage(SRDAGTransformer::SRLinker &linker) {
    auto &sourceArray = *(linker.sourceArray);
    auto &sinkArray = *(linker.sinkArray);
    for (auto *src : sourceArray) {
        Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                sinkArray[linker.sinkCount], linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
        linker.sourceCount += 1;
        linker.sinkCount += 1;
    }
}

void SRDAGTransformer::joinPatternLinkage(SRDAGTransformer::SRLinker &linker) {
    auto &sourceArray = *(linker.sourceArray);
    auto &sinkArray = *(linker.sinkArray);
    auto nProducer = Spider::Math::ceilDiv(linker.sinkRate, linker.sourceRate);
    Spider::Array<PiSDFVertex *> joinArray{StackID::TRANSFO, linker.sink->repetitionValue()};
    for (auto *snk : sinkArray) {
        auto *join = Spider::API::createJoin(srdag_, "join-" + snk->name(), nProducer, 0,
                                             StackID::TRANSFO);
        joinArray[linker.sinkCount] = join;
        Spider::API::createEdge(srdag_, join, 0, linker.sinkRate,
                                snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
        linker.sinkCount += 1;
    }

    std::uint32_t joinPortIx = 0;
    std::uint32_t joinIx = 0;
    for (auto *src : sourceArray) {
        auto lowerDep = (linker.sourceCount * linker.sourceRate + linker.delay) / linker.sinkRate;
        auto upperDep = ((linker.sourceCount + 1) * linker.sourceRate + linker.delay - 1) / linker.sinkRate;
        auto *join = joinArray[joinIx];
        if (lowerDep == upperDep) {
            Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                    join, joinPortIx, linker.sourceRate, StackID::TRANSFO);
            joinPortIx = (joinPortIx + 1) % join->nEdgesIN();
        } else {
            auto *fork = Spider::API::createFork(srdag_, "fork-" + src->name(), 2, 0, StackID::TRANSFO);
            Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                    fork, 0, linker.sourceRate, StackID::TRANSFO);
            auto remainderJoinLower = linker.sinkRate - (join->nEdgesIN() - 1) * linker.sourceRate;
            auto consumptionJoinUpper = linker.sourceRate - remainderJoinLower;
            Spider::API::createEdge(srdag_, fork, 0, remainderJoinLower,
                                    join, join->nEdgesIN() - 1, remainderJoinLower,
                                    StackID::TRANSFO);
            Spider::API::createEdge(srdag_, fork, 1, consumptionJoinUpper,
                                    joinArray[joinIx + 1], 0, consumptionJoinUpper, StackID::TRANSFO);
            joinIx += 1;
            joinPortIx = 1;
        }
        linker.sourceCount += 1;
    }
}

void SRDAGTransformer::forkPatternLinkage(SRDAGTransformer::SRLinker &linker) {
    auto &sourceArray = *(linker.sourceArray);
    auto &sinkArray = *(linker.sinkArray);
    auto nConsumer = Spider::Math::ceilDiv(linker.sourceRate, linker.sinkRate);
    Spider::Array<PiSDFVertex *> forkArray{StackID::TRANSFO, linker.source->repetitionValue()};
    for (auto *src : sourceArray) {
        auto *fork = Spider::API::createFork(srdag_, "fork-" + src->name(), nConsumer, 0, StackID::TRANSFO);
        forkArray[linker.sourceCount] = fork;
        Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                fork, 0, linker.sourceRate, StackID::TRANSFO);
        linker.sourceCount += 1;
    }

    /* == Link the sinks == */
    std::uint32_t forkPortIx = 0;
    for (auto *snk : sinkArray) {
        /* == See "Numerical Representation of Directed Acyclic Graphs for Efficient Dataflow Embedded Resource
         * Allocation" for more details == */
        auto lowerDep = (linker.sinkCount * linker.sinkRate - linker.delay) / linker.sourceRate;
        auto upperDep = ((linker.sinkCount + 1) * linker.sinkRate - linker.delay - 1) / linker.sourceRate;
        if (lowerDep == upperDep) {
            Spider::API::createEdge(srdag_, forkArray[lowerDep], forkPortIx, linker.sinkRate,
                                    snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
            forkPortIx = (forkPortIx + 1) % nConsumer;
        } else {
            auto *join = Spider::API::createJoin(srdag_, "join-" + snk->name(), 2, 0, StackID::TRANSFO);
            Spider::API::createEdge(srdag_, join, 0, linker.sinkRate,
                                    snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
            auto remainderForkLower = linker.sourceRate - (nConsumer - 1) * linker.sinkRate;
            auto consumptionForkUpper = linker.sinkRate - remainderForkLower;
            Spider::API::createEdge(srdag_, forkArray[lowerDep], (nConsumer - 1), remainderForkLower,
                                    join, 0, remainderForkLower, StackID::TRANSFO);
            Spider::API::createEdge(srdag_, forkArray[upperDep], 0, consumptionForkUpper,
                                    join, 1, consumptionForkUpper, StackID::TRANSFO);
            forkPortIx = 1;
        }
        linker.sinkCount += 1;
    }
}

void SRDAGTransformer::delayLinkage(SRDAGTransformer::SRLinker &linker) {
    if (linker.delay >= linker.sinkRate * linker.sink->repetitionValue()) {
        /* == Delay linkage == */
        fullyPipelinedLinkage(linker);
    } else {
        /* == Init pattern == */
        delayInitLinkage(linker);

        /* == Do linkage == */
        if (linker.sourceRate > linker.sinkRate) {
            delayedForkPatternLinkage(linker);
        } else {
            delayedJoinPatternLinkage(linker);
        }

        /* == End pattern == */
        delayEndLinkage(linker);
    }
}

void SRDAGTransformer::fullyPipelinedLinkage(SRDAGTransformer::SRLinker &linker) {
    auto &sourceArray = *(linker.sourceArray);
    auto &sinkArray = *(linker.sinkArray);

    /* == Create and link the Init vertex == */
    auto *init = Spider::API::createInit(srdag_, "init-" + linker.sink->name(), 0, StackID::TRANSFO);
    auto nConsumer = Spider::Math::ceilDiv(linker.delay, linker.sinkRate);
    auto *fork = Spider::API::createFork(srdag_, "fork-" + init->name(), nConsumer, 0, StackID::TRANSFO);
    Spider::API::createEdge(srdag_, init, 0, linker.delay, fork, 0, linker.delay, StackID::TRANSFO);
    std::uint32_t forkPortIx = 0;
    for (auto *snk : sinkArray) {
        Spider::API::createEdge(srdag_, fork, forkPortIx, linker.sinkRate,
                                snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
        forkPortIx += 1;
    }

    /* == Create and link the End vertex == */
    auto *end = Spider::API::createEnd(srdag_, "end-" + linker.source->name(), 0, StackID::TRANSFO);
    auto nProducer = fork->nEdgesOUT() - linker.sink->repetitionValue() + linker.source->repetitionValue();
    auto *join = Spider::API::createJoin(srdag_, "join-" + end->name(), nProducer, 0, StackID::TRANSFO);
    Spider::API::createEdge(srdag_, join, 0, linker.delay, end, 0, linker.delay, StackID::TRANSFO);
    std::uint32_t joinPortIx = 0;
    auto remainderInit = linker.delay - linker.sinkRate * linker.sink->repetitionValue();
    if (remainderInit) {
        Spider::API::createEdge(srdag_, fork, forkPortIx, remainderInit, join, 0, remainderInit, StackID::TRANSFO);
        joinPortIx = 1;
    }
    for (auto *src : sourceArray) {
        Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                join, joinPortIx, linker.sourceRate, StackID::TRANSFO);
        joinPortIx += 1;
    }
    linker.sinkCount = sinkArray.size();
    linker.sourceCount = sourceArray.size();
}

void SRDAGTransformer::delayInitLinkage(SRDAGTransformer::SRLinker &linker) {
    auto &sinkArray = *(linker.sinkArray);
    auto *init = Spider::API::createInit(srdag_, "init-" + linker.sink->name(), 0, StackID::TRANSFO);
    linker.init = init;
    if (linker.delay == linker.sinkRate) {
        Spider::API::createEdge(srdag_, init, 0, linker.delay,
                                sinkArray[0], linker.sinkPortIx, linker.delay, StackID::TRANSFO);
        linker.init = nullptr;
        linker.sinkCount = 1;
    } else if (linker.delay > linker.sinkRate) {
        auto nOutput = Spider::Math::ceilDiv(linker.delay, linker.sinkRate);
        auto *fork = Spider::API::createFork(srdag_, "fork-" + init->name(), nOutput, 0, StackID::TRANSFO);
        Spider::API::createEdge(srdag_, init, 0, linker.delay,
                                fork, 0, linker.delay, StackID::TRANSFO);
        linker.init = nullptr;
        linker.sinkCount = linker.delay / linker.sinkRate;
        for (std::uint32_t i = 0; i < linker.sinkCount; ++i) {
            auto *snk = sinkArray[i];
            Spider::API::createEdge(srdag_, fork, i, linker.sinkRate,
                                    snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
        }
        if (linker.delay != (linker.sinkCount * linker.sinkRate)) {
            linker.init = fork;
        }
    }
    linker.sourceCount = linker.delay / linker.sourceRate;
}

void SRDAGTransformer::delayEndLinkage(SRDAGTransformer::SRLinker &linker) {
    auto &sourceArray = *(linker.sourceArray);
    auto *end = Spider::API::createEnd(srdag_, "end-" + linker.source->name(), 0, StackID::TRANSFO);
    if (linker.delay == linker.sourceRate) {
        /* == Connect the last source == */
        auto *src = sourceArray[linker.sourceCount];
        Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.delay,
                                end, 0, linker.delay, StackID::TRANSFO);
        linker.sourceCount += 1;
    } else if (linker.delay < linker.sourceRate) {
        /* == Connect the last output of the last fork == */
        auto *lastSource = sourceArray[linker.sourceCount - 1];
        auto *fork = lastSource->outputEdge(linker.sourcePortIx)->sink();
        Spider::API::createEdge(srdag_, fork, fork->nEdgesOUT() - 1, linker.delay,
                                end, 0, linker.delay, StackID::TRANSFO);
    } else {
        /* == Create a join for the end == */
        auto nRemainingSource = linker.source->repetitionValue() - linker.sourceCount;
        auto lastForkPresent = nRemainingSource * linker.sourceRate != linker.delay;
        auto nInput = nRemainingSource + lastForkPresent;
        auto *join = Spider::API::createJoin(srdag_, "join-" + end->name(), nInput, 0, StackID::TRANSFO);
        Spider::API::createEdge(srdag_, join, 0, linker.delay,
                                end, 0, linker.delay, StackID::TRANSFO);

        /* == Link the remaining output of the last fork (if any) == */
        if (lastForkPresent) {
            auto *lastSource = sourceArray[linker.sourceCount - 1];
            auto *fork = lastSource->outputEdge(linker.sourcePortIx)->sink();
            auto rate = linker.delay - nRemainingSource * linker.sourceRate;
            Spider::API::createEdge(srdag_, fork, fork->nEdgesOUT() - 1, rate,
                                    join, 0, rate, StackID::TRANSFO);
        }

        /* == Link the last sources == */
        std::uint32_t joinPortIx = lastForkPresent;
        for (std::uint32_t i = 0; i < nRemainingSource; ++i) {
            auto *src = sourceArray[linker.sourceCount];
            Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                    join, joinPortIx, linker.sourceRate, StackID::TRANSFO);
            linker.sourceCount += 1;
            joinPortIx += 1;
        }
    }
}

void SRDAGTransformer::delayedJoinPatternLinkage(SRDAGTransformer::SRLinker &linker) {
    auto &sourceArray = *(linker.sourceArray);
    auto &sinkArray = *(linker.sinkArray);

    /* == Declaring join array == */
    std::uint32_t joinPortIx = 0;
    std::uint32_t joinIx = 0;
    auto sinkRepetitionValue = linker.sink->repetitionValue();
    Spider::Array<PiSDFVertex *> joinArray{StackID::TRANSFO, sinkRepetitionValue};
    auto firstSink = linker.sinkCount;
    auto lastSink = sinkRepetitionValue;
    for (std::uint32_t i = firstSink; i < lastSink; ++i) {
        auto lowerDep = computeConsLowerDep(linker.sinkRate, linker.sourceRate, i, linker.delay);
        auto upperDep = computeConsUpperDep(linker.sinkRate, linker.sourceRate, i, linker.delay);
        auto nInput = upperDep - lowerDep + 1;
        auto *snk = sinkArray[i];
        auto *join = Spider::API::createJoin(srdag_, "join-" + snk->name(), nInput, 0, StackID::TRANSFO);
        Spider::API::createEdge(srdag_, join, 0, linker.sinkRate,
                                snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
        joinArray[joinIx++] = join;
        linker.sinkCount += 1;
        if (lowerDep < 0) {
            joinPortIx = 1;
            auto rate = linker.delay - firstSink * linker.sinkRate;
            Spider::API::createEdge(srdag_, linker.init, linker.init->nEdgesOUT() - 1, rate,
                                    join, 0, rate, StackID::TRANSFO);
        }
    }

    /* == Join pattern linkage == */
    auto lastSource = linker.source->repetitionValue() - linker.sourceCount;
    linker.sourceCount = 0;
    joinIx = 0;
    for (std::uint32_t i = 0; i < lastSource; ++i) {
        auto *src = sourceArray[i];
        auto lowerDep = computeProdLowerDep(linker.sinkRate, linker.sourceRate, i, linker.delay, sinkRepetitionValue);
        auto upperDep = computeProdUpperDep(linker.sinkRate, linker.sourceRate, i, linker.delay, sinkRepetitionValue);
        auto *join = joinArray[joinIx];
        if (lowerDep == upperDep) {
            Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                    join, joinPortIx, linker.sourceRate, StackID::TRANSFO);
            joinPortIx = (joinPortIx + 1) % join->nEdgesIN();
            joinIx += (joinPortIx == 0);
        } else {
            auto *fork = Spider::API::createFork(srdag_, "fork-" + src->name(), 2, 0, StackID::TRANSFO);
            Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                    fork, 0, linker.sourceRate, StackID::TRANSFO);
            auto joinConsumption = join->outputEdge(0)->sourceRate();
            auto remainderJoin = joinConsumption - (join->nEdgesIN() - 1) * linker.sourceRate;
            auto upperJoin = joinArray[joinIx + 1];
            auto consumptionJoinUpper = linker.sourceRate - remainderJoin;
            Spider::API::createEdge(srdag_, fork, 0, remainderJoin,
                                    join, join->nEdgesIN() - 1, remainderJoin,
                                    StackID::TRANSFO);
            Spider::API::createEdge(srdag_, fork, 1, consumptionJoinUpper,
                                    upperJoin, 0, consumptionJoinUpper, StackID::TRANSFO);
            joinIx += 1;
            joinPortIx = 1;
        }
        linker.sourceCount += 1;
    }
}

void SRDAGTransformer::delayedForkPatternLinkage(SRDAGTransformer::SRLinker &linker) {
    auto &sourceArray = *(linker.sourceArray);
    auto &sinkArray = *(linker.sinkArray);

    /* == Create the fork array == */
    Spider::Array<PiSDFVertex *> forkArray{StackID::TRANSFO, linker.source->repetitionValue()};
    std::uint32_t forkArrayOffset = 0;
    if (linker.init && linker.init->type() == PiSDFVertexType::FORK) {
        forkArray[0] = linker.init;
        forkArrayOffset = 1;
    }
    auto lastSource = linker.source->repetitionValue() - linker.sourceCount;
    linker.sourceCount = 0;
    for (std::uint32_t i = 0; i < lastSource; ++i) {
        auto lowerDep = computeProdLowerDep(linker.sinkRate, linker.sourceRate, i, linker.delay,
                                            linker.sink->repetitionValue());
        auto upperDep = computeProdUpperDep(linker.sinkRate, linker.sourceRate, i, linker.delay,
                                            linker.sink->repetitionValue());
        auto nOutput = upperDep - lowerDep + 1;
        auto *src = sourceArray[i];
        auto *fork = Spider::API::createFork(srdag_, "fork-" + src->name(), nOutput, 0, StackID::TRANSFO);
        Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                fork, 0, linker.sourceRate, StackID::TRANSFO);
        forkArray[i + forkArrayOffset] = fork;
        linker.sourceCount += 1;
    }

    /* == Link the sinks == */
    std::uint32_t forkPortIx = 0;
    std::uint32_t forkIx = 0;
    auto firstSink = linker.sinkCount;
    auto lastSink = linker.sink->repetitionValue();
    for (std::uint32_t i = firstSink; i < lastSink; ++i) {
        /* == See "Numerical Representation of Directed Acyclic Graphs for Efficient Dataflow Embedded Resource
         * Allocation" for more details == */
        auto *snk = sinkArray[i];
        auto lowerDep = computeConsLowerDep(linker.sinkRate, linker.sourceRate, linker.sinkCount, linker.delay);
        auto upperDep = computeConsUpperDep(linker.sinkRate, linker.sourceRate, linker.sinkCount, linker.delay);
        auto *fork = forkArray[forkIx];
        if (lowerDep == upperDep) {
            Spider::API::createEdge(srdag_, fork, forkPortIx, linker.sinkRate,
                                    snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
            forkPortIx = (forkPortIx + 1) % fork->nEdgesOUT();
            forkIx += (forkPortIx == 0);
        } else {
            auto *join = Spider::API::createJoin(srdag_, "join-" + snk->name(), 2, 0, StackID::TRANSFO);
            Spider::API::createEdge(srdag_, join, 0, linker.sinkRate,
                                    snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
            std::uint64_t remainderFork = 0;
            if (fork->type() == PiSDFVertexType::FORK) {
                auto forkProd = fork->inputEdge(0)->sinkRate();
                remainderFork = forkProd - (fork->nEdgesOUT() - 1) * linker.sinkRate;
                Spider::API::createEdge(srdag_, fork, (fork->nEdgesOUT() - 1), remainderFork,
                                        join, 0, remainderFork, StackID::TRANSFO);
            } else {
                /* == Case of init == */
                Spider::API::createEdge(srdag_, linker.init, 0, linker.delay,
                                        join, 0, linker.delay, StackID::TRANSFO);
            }
            auto *upperFork = forkArray[forkIx + 1];
            auto consumptionUpperFork = linker.sinkRate - remainderFork;
            Spider::API::createEdge(srdag_, upperFork, 0, consumptionUpperFork,
                                    join, 1, consumptionUpperFork, StackID::TRANSFO);
            forkPortIx = 1;
            forkIx += 1;
        }
        linker.sinkCount += 1;
    }
}