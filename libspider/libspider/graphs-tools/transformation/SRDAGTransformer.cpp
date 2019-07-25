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
#include <graphs/pisdf/PiSDFDelay.h>
#include <graphs/pisdf/PiSDFEdge.h>
#include <graphs/pisdf/PiSDFGraph.h>
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
    for (const auto *subgraph : piSdfGraph_->subgraphs()) {

    }
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
        auto *source = edge->source();
        auto &sourceArray = vertex2Vertex[source->getIx()];
        auto *sink = edge->sink();
        auto &sinkArray = vertex2Vertex[sink->getIx()];
        std::uint64_t sinkRate = edge->sinkRate();
        std::uint64_t sourceRate = edge->sourceRate();
//        auto delay = edge->delay() ? edge->delay()->value() : 0;

        auto totalSourceCount = source->repetitionValue();
        std::uint32_t sourceCount = 0;
        auto totalSinkCount = sink->repetitionValue();
        std::uint32_t sinkCount = 0;

        auto nConsumer = Spider::Math::ceilDiv(sourceRate, sinkRate);

        while (sourceCount != totalSourceCount) {
            if (sourceRate > sinkRate) {
                /* == Adding a Fork vertex == */
                auto *fork = createFork(edge, sourceRate, nConsumer, sourceArray, sourceCount++);
                auto production = sourceRate;
                for (std::uint64_t i = 0; i < nConsumer; ++i) {
                    /* == Link the edges from the fork to the different sinks == */
                    auto *forkEdge = Spider::API::createEdge(srdag_,
                                                             fork, i, sinkRate,
                                                             sinkArray[sinkCount++], edge->sinkPortIx(), sinkRate,
                                                             StackID::TRANSFO);
                    production -= sinkRate;
                    if (production && production < sinkRate) {
                        /* == Need to add a Join - Fork == */
                        auto *join = createJoin(forkEdge, sinkRate, 2, sinkArray, sinkCount++);
                        Spider::API::createEdge(srdag_,
                                                fork, i + 1, production,
                                                join, 0, production,
                                                StackID::TRANSFO);

                        /* == Reset the variables for the rest of the linking == */
                        fork = createFork(edge, sourceRate, nConsumer, sourceArray, sourceCount++);
                        Spider::API::createEdge(srdag_,
                                                fork, 0, sinkRate - production,
                                                join, 1, sinkRate - production,
                                                StackID::TRANSFO);
                        production = sourceRate - production;
                        Spider::API::createEdge(srdag_,
                                                fork, 1, production,
                                                sinkArray[sinkCount++], edge->sinkPortIx(), sinkRate,
                                                StackID::TRANSFO);
                        production -= production;
                        i = 2;
                    }
                }
            }
        }
        if (sinkCount != totalSinkCount) {
            throwSpiderException("Remaining %"
                                         PRIu32
                                         " instances of %s to link.", sink->name().c_str());
        }
    }

    /* == Free memory of the arrays == */
    for (auto &array : vertex2Vertex) {
        Spider::destroy(&array);
    }
}

PiSDFVertex *SRDAGTransformer::createFork(const PiSDFEdge *edge,
                                          std::int64_t sourceRate,
                                          std::int64_t nConsumer,
                                          Spider::Array<PiSDFVertex *> &sourceArray,
                                          std::uint32_t sourceCount) {
    auto *source = sourceArray[sourceCount];
    auto *sink = edge->sink();
    auto *fork = Spider::API::createFork(srdag_,
                                         "fork-" + source->name() + "-" + sink->name(),
                                         nConsumer,
                                         0,
                                         StackID::TRANSFO);
    Spider::API::createEdge(srdag_,
                            source, edge->sourcePortIx(), sourceRate,
                            fork, 0, sourceRate,
                            StackID::TRANSFO);
    return fork;
}

PiSDFVertex *SRDAGTransformer::createJoin(const PiSDFEdge *edge,
                                          std::int64_t sinkRate,
                                          std::int64_t nProducer,
                                          Spider::Array<PiSDFVertex *> &sinkArray,
                                          std::uint32_t sinkCount) {
    auto *source = edge->source();
    auto *sink = sinkArray[sinkCount];
    auto *join = Spider::API::createJoin(srdag_,
                                         "join-" + source->name() + "-" + sink->name(),
                                         nProducer,
                                         0,
                                         StackID::TRANSFO);
    Spider::API::createEdge(srdag_,
                            join, 0, sinkRate,
                            sink, edge->sinkPortIx(), sinkRate,
                            StackID::TRANSFO);
    return join;
}
