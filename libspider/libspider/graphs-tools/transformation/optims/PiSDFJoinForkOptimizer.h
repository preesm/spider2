/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
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
#ifndef SPIDER2_PISDFJOINFORKOPTIMIZER_H
#define SPIDER2_PISDFJOINFORKOPTIMIZER_H

/* === Includes === */

#include <graphs-tools/transformation/optims/PiSDFOptimizer.h>
#include <common/Math.h>
#include <api/pisdf.h>

/* === Class definition === */

/**
 * @brief Optimize Join -> Fork patterns in a PiSDFGraph.
 * @see: https://tel.archives-ouvertes.fr/tel-01301642
 */
class PiSDFJoinForkOptimizer final : public PiSDFOptimizer {
public:
    inline bool operator()(PiSDFGraph *graph) const override;

private:
    struct EdgeLinker {
        PiSDFAbstractVertex *vertex = nullptr;
        uint64_t rate = 0;
        uint32_t portIx = 0;

        EdgeLinker() = default;

        EdgeLinker(PiSDFAbstractVertex *vertex, uint64_t rate, uint32_t portIx) : vertex{ vertex },
                                                                                  rate{ rate },
                                                                                  portIx{ portIx } { };
    };

    inline uint32_t computeNJoinEdge(uint64_t sinkRate,
                                     spider::array<EdgeLinker> &sourceArray,
                                     size_t sourceIx) const;

    inline uint32_t computeNForkEdge(uint64_t sourceRate,
                                     spider::array<EdgeLinker> &sinkArray,
                                     size_t sinkIx) const;
};

bool PiSDFJoinForkOptimizer::operator()(PiSDFGraph *graph) const {
    auto verticesToOptimize = spider::containers::vector<std::pair<PiSDFAbstractVertex *, PiSDFAbstractVertex *>>(
            StackID::TRANSFO);

    /* == Search for the pair of join / fork to optimize == */
    for (auto &vertex : graph->vertices()) {
        if (vertex->subtype() == PiSDFVertexType::JOIN) {
            auto *sink = vertex->outputEdge(0)->sink();
            if (sink->subtype() == PiSDFVertexType::FORK) {
                verticesToOptimize.push_back(std::make_pair(vertex, sink));
            }
        }
    }

    /* == Go through the different pair to optimize == */
    const auto &params = graph->params();
    for (auto &pair : verticesToOptimize) {
        auto *join = pair.first;
        auto *fork = pair.second;
        spider::array<EdgeLinker> sourceArray{ join->inputEdgeCount(), StackID::TRANSFO };
        spider::array<EdgeLinker> sinkArray{ fork->outputEdgeCount(), StackID::TRANSFO };

        for (auto *edge : join->inputEdgeArray()) {
            const auto &rate = static_cast<uint64_t>(edge->sourceRateExpression().evaluate(params));
            sourceArray[edge->sinkPortIx()] = EdgeLinker{ edge->source(), rate, edge->sourcePortIx() };
            graph->removeEdge(edge);
        }
        graph->removeEdge(join->outputEdge(0));
        for (auto *edge : fork->outputEdgeArray()) {
            const auto &rate = static_cast<uint64_t>(edge->sinkRateExpression().evaluate(params));
            sinkArray[edge->sourcePortIx()] = EdgeLinker{ edge->sink(), rate, edge->sinkPortIx() };
            graph->removeEdge(edge);
        }

        /* == Remove fork / join == */
        graph->removeVertex(join);
        graph->removeVertex(fork);

        /* == Re-do the linking == */
        uint32_t sourceIx = 0;
        uint32_t forkEdgeIx = 0;
        for (size_t sinkIx = 0; sinkIx < sinkArray.size(); ++sinkIx) {
            auto &sink = sinkArray[sinkIx];
            auto &source = sourceArray[sourceIx];
            if (sink.rate == source.rate) {
                auto sourcePortIx = source.portIx == UINT32_MAX ? forkEdgeIx : source.portIx;
                const auto &srcRate = static_cast<int64_t>(source.rate);
                const auto &snkRate = static_cast<int64_t>(sink.rate);
                spider::api::createEdge(source.vertex, sourcePortIx, srcRate, sink.vertex, sink.portIx, snkRate,
                                        StackID::TRANSFO);
                sourceIx += 1;
            } else if (source.rate > sink.rate) {
                if (source.portIx == UINT32_MAX) {
                    /* == Case of added Fork == */
                    const auto &snkRate = static_cast<int64_t>(sink.rate);
                    spider::api::createEdge(source.vertex, forkEdgeIx, snkRate, sink.vertex, sink.portIx, snkRate,
                                            StackID::TRANSFO);
                    source.rate -= sink.rate;
                    forkEdgeIx += 1;
                } else {
                    /* == Add a Fork == */
                    auto nForkEdge = computeNForkEdge(source.rate, sinkArray, sinkIx);
                    auto *addedFork = spider::api::createFork(graph,
                                                              "fork-" + source.vertex->name() + "-out" +
                                                              std::to_string(source.portIx),
                                                              nForkEdge,
                                                              StackID::TRANSFO);
                    const auto &srcRate = static_cast<int64_t>(source.rate);
                    spider::api::createEdge(source.vertex, source.portIx, srcRate, addedFork, 0, srcRate,
                                            StackID::TRANSFO);
                    const auto &snkRate = static_cast<int64_t>(sink.rate);
                    spider::api::createEdge(addedFork, 0, snkRate, sink.vertex, sink.portIx, snkRate,
                                            StackID::TRANSFO);
                    source.vertex = addedFork;
                    source.portIx = UINT32_MAX;
                    source.rate -= sink.rate;
                    forkEdgeIx = 1;
                }
            } else {
                /* == Need for a Join == */
                auto nJoinEdge = computeNJoinEdge(sink.rate, sourceArray, sourceIx);
                auto *addedJoin = spider::api::createJoin(graph,
                                                          "join-" +
                                                          sink.vertex->name() +
                                                          "-in" +
                                                          std::to_string(sink.portIx),
                                                          nJoinEdge,
                                                          StackID::TRANSFO);
                const auto &snkRate = static_cast<int64_t>(sink.rate);
                spider::api::createEdge(addedJoin, 0, snkRate, sink.vertex, sink.portIx, snkRate,
                                        StackID::TRANSFO);
                for (uint64_t joinPortIx = 0; joinPortIx < addedJoin->inputEdgeCount(); ++joinPortIx) {
                    source = sourceArray[sourceIx];
                    if (source.rate <= sink.rate) {
                        auto sourcePortIx = source.portIx == UINT32_MAX ? forkEdgeIx : source.portIx;
                        const auto &srcRate = static_cast<int64_t>(source.rate);
                        spider::api::createEdge(source.vertex,
                                                sourcePortIx,
                                                srcRate,
                                                addedJoin,
                                                static_cast<uint32_t >(joinPortIx),
                                                srcRate,
                                                StackID::TRANSFO);
                        sink.rate -= source.rate;
                        sourceIx += 1;
                    } else {
                        sink.vertex = addedJoin;
                        sink.portIx = static_cast<uint32_t >(joinPortIx);
                        sinkIx -= 1;
                    }
                }
            }
        }
    }
    return verticesToOptimize.empty();
}

uint32_t PiSDFJoinForkOptimizer::computeNJoinEdge(uint64_t sinkRate,
                                                  spider::array<EdgeLinker> &sourceArray,
                                                  size_t sourceIx) const {
    uint32_t nJoinEdge = 0;
    uint64_t totalRate = 0;
    while (sinkRate > totalRate) {
        totalRate += sourceArray[sourceIx].rate;
        nJoinEdge += 1;
        sourceIx += 1;
    }
    return nJoinEdge;
}

uint32_t PiSDFJoinForkOptimizer::computeNForkEdge(uint64_t sourceRate,
                                                  spider::array<EdgeLinker> &sinkArray,
                                                  size_t sinkIx) const {
    uint32_t nForkEdge = 0;
    uint64_t totalRate = 0;
    while (sourceRate > totalRate) {
        totalRate += sinkArray[sinkIx].rate;
        nForkEdge += 1;
        sinkIx += 1;
    }
    return nForkEdge;
}

#endif //SPIDER2_PISDFJOINFORKOPTIMIZER_H
