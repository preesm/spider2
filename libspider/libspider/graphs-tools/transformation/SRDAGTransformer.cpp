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
        if (linker.delay > linker.sinkRate * linker.sink->repetitionValue()) {
            /* == Full pipeline linkage == */
            fullyPipelinedLinkage(linker);
        } else {
            if (linker.sourceRate >= linker.sinkRate) {
                /* == Fork pattern linkage == */
                forkPatternLinkage(linker);
            } else {
                /* == Join pattern linkage == */
                delayLinkage(linker);
            }

            /* == Check for delay at the end == */
            if (linker.delay) {
                delayEndLinkage(linker);
            }
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
    auto hasDelay = linker.delay != 0;
    Spider::Array<PiSDFVertex *> updatedSourceArray{StackID::TRANSFO,
                                                    linker.source->repetitionValue() + hasDelay};
    if (linker.delay) {
        auto *init = Spider::API::createInit(srdag_, "init-" + linker.sink->name(), 0, StackID::TRANSFO);
        if (linker.delay > linker.sinkRate) {
            auto nConsumer = Spider::Math::ceilDiv(linker.delay, linker.sinkRate);
            auto *fork = Spider::API::createFork(srdag_, "fork-" + init->name(), nConsumer, 0, StackID::TRANSFO);
            updatedSourceArray[0] = fork;
            Spider::API::createEdge(srdag_, init, 0, linker.delay,
                                    fork, 0, linker.delay, StackID::TRANSFO);
        } else {
            updatedSourceArray[0] = init;
        }
    }
    for (auto *src : sourceArray) {
        auto lowerDep = computeProdLowerDep(linker.sinkRate, linker.sourceRate, linker.sourceCount,
                                            linker.delay,
                                            linker.sink->repetitionValue());
        auto upperDep = computeProdUpperDep(linker.sinkRate, linker.sourceRate, linker.sourceCount,
                                            linker.delay,
                                            linker.sink->repetitionValue());
        if (lowerDep == upperDep) {
            updatedSourceArray[linker.sourceCount + hasDelay] = src;
        } else {
            auto nConsumer = (upperDep - lowerDep) + 1;
            auto *fork = Spider::API::createFork(srdag_, "fork-" + src->name(), nConsumer, 0, StackID::TRANSFO);
            updatedSourceArray[linker.sourceCount + hasDelay] = fork;
            Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                    fork, 0, linker.sourceRate, StackID::TRANSFO);
        }
        linker.sourceCount += 1;
    }

    /* == Link the sinks == */
    std::uint32_t forkPortIx = 0;
    for (auto *snk : sinkArray) {
        /* == See "Numerical Representation of Directed Acyclic Graphs for Efficient Dataflow Embedded Resource
         * Allocation" for more details == */
        auto lowerDep = computeConsLowerDep(linker.sinkRate, linker.sourceRate, linker.sinkCount, linker.delay);
        auto upperDep = computeConsUpperDep(linker.sinkRate, linker.sourceRate, linker.sinkCount, linker.delay);
        auto *src = updatedSourceArray[lowerDep + hasDelay];
        if (lowerDep == upperDep) {
            switch (src->type()) {
                case PiSDFVertexType::FORK:
                    Spider::API::createEdge(srdag_, src, forkPortIx, linker.sinkRate,
                                            snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
                    forkPortIx = (forkPortIx + 1) % src->nEdgesOUT();
                    break;
                case PiSDFVertexType::INIT:
                    Spider::API::createEdge(srdag_, src, 0, linker.sinkRate,
                                            snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
                    break;
                default:
                    /* == Forward case == */
                    Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                            snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
                    break;
            }
        } else {
            auto *join = Spider::API::createJoin(srdag_, "join-" + snk->name(), 2, 0, StackID::TRANSFO);
            Spider::API::createEdge(srdag_, join, 0, linker.sinkRate,
                                    snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
            std::uint64_t consumptionLower = 0;
            if (src->type() == PiSDFVertexType::INIT) {
                consumptionLower = linker.delay;
            } else {
                consumptionLower = src->inputEdge(0)->sinkRate() - (src->nEdgesOUT() - 1) * linker.sinkRate;
            }
            auto consumptionForkUpper = linker.sinkRate - consumptionLower;
            Spider::API::createEdge(srdag_, src, (src->nEdgesOUT() - 1), consumptionLower,
                                    join, 0, consumptionLower, StackID::TRANSFO);
            auto *nextFork = updatedSourceArray[upperDep + hasDelay];
            Spider::API::createEdge(srdag_, nextFork, 0, consumptionForkUpper,
                                    join, 1, consumptionForkUpper, StackID::TRANSFO);
            forkPortIx = 1;
        }
        linker.sinkCount += 1;
    }
}

void SRDAGTransformer::delayLinkage(SRDAGTransformer::SRLinker &linker) {
    auto &sourceArray = *(linker.sourceArray);
    auto &sinkArray = *(linker.sinkArray);
    auto hasDelay = linker.delay != 0;
    Spider::Array<PiSDFVertex *> updatedSourceArray{StackID::TRANSFO,
                                                    linker.source->repetitionValue() + hasDelay};
    if (linker.delay) {
        auto *init = Spider::API::createInit(srdag_, "init-" + linker.sink->name(), 0, StackID::TRANSFO);
        if (linker.delay > linker.sinkRate) {
            auto nConsumer = Spider::Math::ceilDiv(linker.delay, linker.sinkRate);
            auto *fork = Spider::API::createFork(srdag_, "fork-" + init->name(), nConsumer, 0, StackID::TRANSFO);
            updatedSourceArray[0] = fork;
            Spider::API::createEdge(srdag_, init, 0, linker.delay,
                                    fork, 0, linker.delay, StackID::TRANSFO);
        } else {
            updatedSourceArray[0] = init;
        }
    }
    for (auto *src : sourceArray) {
        auto lowerDep = computeProdLowerDep(linker.sinkRate, linker.sourceRate, linker.sourceCount,
                                            linker.delay,
                                            linker.sink->repetitionValue());
        auto upperDep = computeProdUpperDep(linker.sinkRate, linker.sourceRate, linker.sourceCount,
                                            linker.delay,
                                            linker.sink->repetitionValue());
        if (lowerDep == upperDep) {
            updatedSourceArray[linker.sourceCount + hasDelay] = src;
        } else {
            auto nConsumer = (upperDep - lowerDep) + 1;
            auto *fork = Spider::API::createFork(srdag_, "fork-" + src->name(), nConsumer, 0, StackID::TRANSFO);
            updatedSourceArray[linker.sourceCount + hasDelay] = fork;
            Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                    fork, 0, linker.sourceRate, StackID::TRANSFO);
        }
        linker.sourceCount += 1;
    }

    /* == Link the sinks == */
    std::uint32_t forkPortIx = 0;
    for (auto *snk : sinkArray) {
        /* == See "Numerical Representation of Directed Acyclic Graphs for Efficient Dataflow Embedded Resource
         * Allocation" for more details == */
        auto lowerDep = computeConsLowerDep(linker.sinkRate, linker.sourceRate, linker.sinkCount, linker.delay);
        auto upperDep = computeConsUpperDep(linker.sinkRate, linker.sourceRate, linker.sinkCount, linker.delay);
        auto *src = updatedSourceArray[lowerDep + hasDelay];
        if (lowerDep == upperDep) {
            switch (src->type()) {
                case PiSDFVertexType::FORK:
                    Spider::API::createEdge(srdag_, src, forkPortIx, linker.sinkRate,
                                            snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
                    forkPortIx = (forkPortIx + 1) % src->nEdgesOUT();
                    break;
                case PiSDFVertexType::INIT:
                    Spider::API::createEdge(srdag_, src, 0, linker.sinkRate,
                                            snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
                    break;
                default:
                    /* == Forward case == */
                    Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                            snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
                    break;
            }
        } else {
            auto nInput = (upperDep - lowerDep) + 1;
            auto *join = Spider::API::createJoin(srdag_, "join-" + snk->name(), nInput, 0, StackID::TRANSFO);
            Spider::API::createEdge(srdag_, join, 0, linker.sinkRate,
                                    snk, linker.sinkPortIx, linker.sinkRate, StackID::TRANSFO);
            std::uint64_t consumptionLower = 0;
            switch (src->type()) {
                case PiSDFVertexType::FORK:
                    for (auto *edge : src->outputEdges()) {
                        if (!edge) {
                            break;
                        }
                        consumptionLower += edge->sourceRate();
                    }
                    consumptionLower = src->inputEdge(0)->sinkRate() - consumptionLower;
                    Spider::API::createEdge(srdag_, src, (src->nEdgesOUT() - 1), consumptionLower,
                                            join, 0, consumptionLower, StackID::TRANSFO);
                    break;
                case PiSDFVertexType::INIT:
                    consumptionLower = linker.delay;
                    Spider::API::createEdge(srdag_, src, 0, consumptionLower,
                                            join, 0, consumptionLower, StackID::TRANSFO);
                    break;
                default:
                    consumptionLower = linker.sourceRate;
                    Spider::API::createEdge(srdag_, src, linker.sourcePortIx, consumptionLower,
                                            join, 0, consumptionLower, StackID::TRANSFO);
                    break;
            }

            /* == Connect everything else to the join == */
            auto nextDep = lowerDep + hasDelay + 1;
            std::uint32_t joinPortIx = 1;
            auto totalConso = linker.sinkRate - consumptionLower;
            for (auto i = nextDep; i <= upperDep + hasDelay; ++i) {
                auto *nextSrc = updatedSourceArray[i];
                if (nextSrc->type() == PiSDFVertexType::FORK) {
                    Spider::API::createEdge(srdag_, nextSrc, 0, totalConso,
                                            join, joinPortIx, totalConso, StackID::TRANSFO);
                } else {
                    Spider::API::createEdge(srdag_, nextSrc, linker.sourcePortIx, linker.sourceRate,
                                            join, joinPortIx, linker.sourceRate, StackID::TRANSFO);
                    totalConso -= linker.sourceRate;
                }
                joinPortIx += 1;
            }
            forkPortIx = 1;
        }
        linker.sinkCount += 1;
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


void SRDAGTransformer::delayEndLinkage(SRDAGTransformer::SRLinker &linker) {
    auto &sourceArray = *(linker.sourceArray);
    auto *end = Spider::API::createEnd(srdag_, "end-" + linker.source->name(), 0, StackID::TRANSFO);
    auto *lastSource = sourceArray[linker.sourceCount - 1];
    if (linker.delay == linker.sourceRate) {
        /* == Connect the last source == */
        Spider::API::createEdge(srdag_, lastSource, linker.sourcePortIx, linker.delay,
                                end, 0, linker.delay, StackID::TRANSFO);
    } else if (linker.delay < linker.sourceRate) {
        /* == Connect the last output of the last fork == */
        auto *fork = lastSource->outputEdge(linker.sourcePortIx)->sink();
        Spider::API::createEdge(srdag_, fork, fork->nEdgesOUT() - 1, linker.delay,
                                end, 0, linker.delay, StackID::TRANSFO);
    } else {
        /* == Create a join for the end == */
        auto nInput = Spider::Math::ceilDiv(linker.delay, linker.sourceRate);
        auto *join = Spider::API::createJoin(srdag_, "join-" + end->name(), nInput, 0, StackID::TRANSFO);
        Spider::API::createEdge(srdag_, join, 0, linker.delay,
                                end, 0, linker.delay, StackID::TRANSFO);

        /* == Link the last sources == */
        std::uint32_t joinPortIx = 0;
        for (std::uint64_t i = 0; i < nInput; ++i) {
            auto *src = sourceArray[linker.sourceCount - (nInput - i)];
            if (src->outputEdge(linker.sourcePortIx)) {
                auto *fork = src->outputEdge(linker.sourcePortIx)->sink();
                auto rate = linker.delay - (nInput - 1) * linker.sourceRate;
                Spider::API::createEdge(srdag_, fork, fork->nEdgesOUT() - 1, rate,
                                        join, joinPortIx, rate, StackID::TRANSFO);
            } else {
                Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                        join, joinPortIx, linker.sourceRate, StackID::TRANSFO);
            }
            joinPortIx += 1;
        }
    }
}
