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
        auto edgeLinker = EdgeLinker{edge, vertex2Vertex};

        /* == Do the linkage == */
        singleRateLinkage(edgeLinker);

        /* == Check that everything has been linked == */
        if (edgeLinker.sinkCount != edgeLinker.sink->repetitionValue()) {
            throwSpiderException("Remaining %"
                                         PRIu32
                                         " instances of %s to link.",
                                 edgeLinker.sink->repetitionValue() - edgeLinker.sinkCount,
                                 edgeLinker.sink->name().c_str());
        }
        if (edgeLinker.sourceCount != edgeLinker.source->repetitionValue()) {
            throwSpiderException("Remaining %"
                                         PRIu32
                                         " instances of %s to link.",
                                 edgeLinker.source->repetitionValue() - edgeLinker.sourceCount,
                                 edgeLinker.source->name().c_str());
        }
    }

    /* == Free memory of the arrays == */
    for (auto &array : vertex2Vertex) {
        Spider::destroy(&array);
    }
}

void SRDAGTransformer::singleRateLinkage(SRDAGTransformer::EdgeLinker &edgeLinker) {
    auto hasDelay = edgeLinker.delay != 0;
    Spider::Array<PiSDFVertex *> sourceLinkArray{StackID::TRANSFO,
                                                 edgeLinker.source->repetitionValue() + hasDelay};
    /* == Build the sourceLinkArray == */
    buildSourceLinkArray(edgeLinker, sourceLinkArray);

    /* == Build the sinkLinkArray == */
    Spider::Array<SinkLinker> sinkLinkArray{StackID::TRANSFO,
                                            edgeLinker.sink->repetitionValue() + hasDelay};
    buildSinkLinkArray(edgeLinker, sinkLinkArray);

    /* == Do the actual linkage == */
    std::uint32_t forkPortIx = 0;
    std::uint64_t forkConsumption = 0;
//    for (auto &sinkLinker : sinkLinkArray) {
    for (std::uint64_t sinkIx = 0; sinkIx < sinkLinkArray.size(); ++sinkIx) {
        auto &sinkLinker = sinkLinkArray[sinkIx];
        auto *src = sourceLinkArray[sinkLinker.lowerDep + hasDelay];
        auto *snk = sinkLinker.vertex;
        if (sinkLinker.lowerDep == sinkLinker.upperDep) {
            /* == Sink can be connected directly == */
            switch (src->type()) {
                case PiSDFVertexType::FORK:
                    /* == Case sinkRate < sourceRate == */
                    Spider::API::createEdge(srdag_, src, forkPortIx, sinkLinker.sinkRate,
                                            snk, sinkLinker.sinkPortIx, sinkLinker.sinkRate, StackID::TRANSFO);
                    forkPortIx = (forkPortIx + 1) % src->nEdgesOUT();
                    forkConsumption += sinkLinker.sinkRate;
                    break;
                case PiSDFVertexType::INIT:
                    /* == Case delay == sourceRate == */
                    Spider::API::createEdge(srdag_, src, 0, edgeLinker.sinkRate,
                                            snk, sinkLinker.sinkPortIx, edgeLinker.sinkRate, StackID::TRANSFO);
                    break;
                default:
                    /* == Case sinkRate == sourceRate == */
                    Spider::API::createEdge(srdag_, src, edgeLinker.sourcePortIx, edgeLinker.sourceRate,
                                            snk, sinkLinker.sinkPortIx, sinkLinker.sinkRate, StackID::TRANSFO);
                    break;
            }
        } else {
            /* == Sink needs a join == */
            auto nInput = (sinkLinker.upperDep - sinkLinker.lowerDep) + 1;
            auto *join = Spider::API::createJoin(srdag_, "join-" + snk->name(), nInput, 0, StackID::TRANSFO);
            Spider::API::createEdge(srdag_, join, 0, sinkLinker.sinkRate,
                                    snk, sinkLinker.sinkPortIx, sinkLinker.sinkRate, StackID::TRANSFO);
            std::uint64_t firstEdgeConsumption = 0;
            switch (src->type()) {
                case PiSDFVertexType::FORK:
                    /* == Case sinkRate < sourceRate == */
                    firstEdgeConsumption = src->inputEdge(0)->sinkRate() - forkConsumption;
                    Spider::API::createEdge(srdag_, src, (src->nEdgesOUT() - 1), firstEdgeConsumption,
                                            join, 0, firstEdgeConsumption, StackID::TRANSFO);
                    forkPortIx = (forkPortIx + 1) % src->nEdgesOUT();
                    break;
                case PiSDFVertexType::INIT:
                    /* == Case delay < sinkRate == */
                    firstEdgeConsumption = edgeLinker.delay;
                    Spider::API::createEdge(srdag_, src, 0, firstEdgeConsumption,
                                            join, 0, firstEdgeConsumption, StackID::TRANSFO);
                    break;
                default:
                    /* == Case sinkRate > sourceRate == */
                    firstEdgeConsumption = edgeLinker.sourceRate;
                    Spider::API::createEdge(srdag_, src, edgeLinker.sourcePortIx, firstEdgeConsumption,
                                            join, 0, firstEdgeConsumption, StackID::TRANSFO);
                    break;
            }

            /* == Connect everything else to the join == */
            /* == case of pattern:  F -> J -> B == */
            /* ==                 A_i ->        == */
            /* ==                [..] ->        == */
            /* ==                 A_j ->        == */
            /* ==                   F ->        == */
            std::uint32_t joinPortIx = 1;
            auto joinProduction = edgeLinker.sinkRate - firstEdgeConsumption;
            for (auto i = sinkLinker.lowerDep + hasDelay + 1; i < sinkLinker.upperDep + hasDelay; ++i) {
                src = sourceLinkArray[i];
                Spider::API::createEdge(srdag_, src, edgeLinker.sourcePortIx, edgeLinker.sourceRate,
                                        join, joinPortIx, edgeLinker.sourceRate, StackID::TRANSFO);
                joinProduction -= edgeLinker.sourceRate;
                joinPortIx += 1;
            }

            /* == Replace sink with join == */
            /* == last source can be either: A_j -> J == */
            /* ==                        or:   F -> J == */
            sinkLinker.vertex = join;
            sinkLinker.sinkRate = joinProduction;
            sinkLinker.lowerDep = sinkLinker.upperDep;
            sinkLinker.sinkPortIx = joinPortIx;
            sinkIx -= 1;
            forkConsumption = 0;
        }
    }
}

void SRDAGTransformer::buildSourceLinkArray(SRDAGTransformer::EdgeLinker &linker,
                                            Spider::Array<PiSDFVertex *> &sourceLinkArray) {
    auto &sourceArray = *(linker.sourceArray);
    auto hasDelay = linker.delay != 0;

    /* == If delay, then first sink will be init (or fork-init) == */
    if (linker.delay) {
        auto *init = Spider::API::createInit(srdag_, "init-" + linker.sink->name(), 0, StackID::TRANSFO);
        if (linker.delay > linker.sinkRate) {
            auto nConsumer = Spider::Math::ceilDiv(linker.delay, linker.sinkRate);
            auto *fork = Spider::API::createFork(srdag_, "fork-" + init->name(), nConsumer, 0, StackID::TRANSFO);
            sourceLinkArray[0] = fork;
            Spider::API::createEdge(srdag_, init, 0, linker.delay,
                                    fork, 0, linker.delay, StackID::TRANSFO);
        } else {
            sourceLinkArray[0] = init;
        }
    }

    /* == Set the sources or sources-fork == */
    for (auto *src : sourceArray) {
        auto lowerDep = computeProdLowerDep(linker.sinkRate, linker.sourceRate, linker.sourceCount,
                                            linker.delay,
                                            linker.sink->repetitionValue());
        auto upperDep = computeProdUpperDep(linker.sinkRate, linker.sourceRate, linker.sourceCount,
                                            linker.delay,
                                            linker.sink->repetitionValue());
        if (lowerDep == upperDep) {
            sourceLinkArray[linker.sourceCount + hasDelay] = src;
        } else {
            auto nConsumer = (upperDep - lowerDep) + 1;
            auto *fork = Spider::API::createFork(srdag_, "fork-" + src->name(), nConsumer, 0, StackID::TRANSFO);
            sourceLinkArray[linker.sourceCount + hasDelay] = fork;
            Spider::API::createEdge(srdag_, src, linker.sourcePortIx, linker.sourceRate,
                                    fork, 0, linker.sourceRate, StackID::TRANSFO);
        }
        linker.sourceCount += 1;
    }
}

void SRDAGTransformer::buildSinkLinkArray(SRDAGTransformer::EdgeLinker &linker,
                                          Spider::Array<SinkLinker> &sinkLinkArray) {
    auto &sinkArray = *(linker.sinkArray);

    /* == Create the end vertex if needed and put it at the end == */
    if (linker.delay) {
        auto *end = Spider::API::createEnd(srdag_, "end-" + linker.source->name(), 0, StackID::TRANSFO);
        auto &lastSinkLinker = sinkLinkArray[sinkLinkArray.size() - 1];
        lastSinkLinker.vertex = end;
        lastSinkLinker.sinkRate = linker.delay;
        lastSinkLinker.sinkPortIx = 0;
        lastSinkLinker.lowerDep = computeConsLowerDep(linker.sinkRate, linker.sourceRate,
                                                      linker.sink->repetitionValue(), linker.delay);
        lastSinkLinker.upperDep = linker.source->repetitionValue() - 1;
    }

    /* == Add the sinks == */
    for (auto *snk : sinkArray) {
        auto &sinkLinker = sinkLinkArray[linker.sinkCount];
        sinkLinker.vertex = snk;
        sinkLinker.sinkRate = linker.sinkRate;
        sinkLinker.sinkPortIx = linker.sinkPortIx;
        sinkLinker.lowerDep = computeConsLowerDep(linker.sinkRate, linker.sourceRate, linker.sinkCount, linker.delay);
        sinkLinker.upperDep = computeConsUpperDep(linker.sinkRate, linker.sourceRate, linker.sinkCount, linker.delay);
        linker.sinkCount += 1;
    }
}

