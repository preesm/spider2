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

/* === Include(s) === */

#include <graphs-tools/transformation/optims/optimizations.h>
#include <graphs-tools/transformation/optims/visitors/UnitaryOptimizerVisitor.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Graph.h>
#include <api/pisdf-api.h>

/* === Private structure(s) === */

struct EdgeLinker {
    spider::pisdf::Vertex *vertex_ = nullptr;
    uint64_t rate_ = 0;
    size_t portIx_ = 0;

    EdgeLinker() = default;

    EdgeLinker(spider::pisdf::Vertex *vertex, uint64_t rate, size_t portIx) : vertex_{ vertex },
                                                                              rate_{ rate },
                                                                              portIx_{ portIx } { };
};

/* === Static function(s) === */

/**
 * @brief Creates one new @refitem pisdf::ForkVertex out of two.
 *        detail: pattern is as follow:
 *         -> firstFork | -> secondaryFork | ->
 *                      | .. fv_j          | .. v_i
 *                      | -> fv_m          | -> v_n
 *
 *    to   -> newFork | ->
 *                    | .. v_i
 *                    | -> v_n
 *                    | .. fv_j
 *                    | -> fv_m
 * @param firstFork   Pointer to the first vertex.
 * @param secondFork  Pointer to the secondary vertex.
 * @return pointer to the created Vertex.
 */
static spider::pisdf::Vertex *createNewFork(spider::pisdf::Vertex *firstFork, spider::pisdf::Vertex *secondFork) {
    auto *graph = firstFork->graph();
    const auto &outputCount = static_cast<uint32_t>((firstFork->outputEdgeCount() - 1) +
                                                    secondFork->outputEdgeCount());
    auto *newFork = spider::api::createFork(graph,
                                            "merged-" + firstFork->name() + "-" + secondFork->name(),
                                            outputCount);

    /* == Connect the input of the first Fork to the new Fork == */
    auto *edge = firstFork->inputEdge(0);
    edge->setSink(newFork, 0, spider::Expression(edge->sinkRateExpression()));
    return newFork;
}

/**
 * @brief Creates one new @refitem pisdf::JoinVertex out of two.
 *        detail: pattern is as follow:
 *         -> | firstJoin -> | secondaryJoin ->
 *    fv_j .. |       v_i .. |
 *    fv_m -> |       v_n -> |
 *
 *    to   -> | newJoin ->
 *    fv_i .. |
 *    fv_m -> |
 *     v_i .. |
 *     v_n -> |
 * @param firstJoin   Pointer to the first vertex.
 * @param secondJoin  Pointer to the secondary vertex.
 * @return pointer to the created Vertex.
 */
static spider::pisdf::Vertex *createNewJoin(spider::pisdf::Vertex *firstJoin, spider::pisdf::Vertex *secondJoin) {
    auto *graph = firstJoin->graph();
    const auto &inputCount = static_cast<uint32_t>(firstJoin->inputEdgeCount() +
                                                   (secondJoin->inputEdgeCount() - 1));
    auto *newJoin = spider::api::createJoin(graph,
                                            "merged-" + firstJoin->name() + "-" + secondJoin->name(),
                                            inputCount);

    /* == Connect the output of the second Join to the new Join == */
    auto *edge = secondJoin->outputEdge(0);
    edge->setSource(newJoin, 0, spider::Expression(edge->sourceRateExpression()));
    return newJoin;
}

static uint32_t computeNJoinEdge(uint64_t sinkRate, spider::array<EdgeLinker> &sourceArray, size_t sourceIx) {
    uint32_t nJoinEdge = 0;
    uint64_t totalRate = 0;
    while (sinkRate > totalRate) {
        totalRate += sourceArray[sourceIx++].rate_;
        nJoinEdge += 1;
    }
    return nJoinEdge;
}

static uint32_t computeNForkEdge(uint64_t sourceRate, spider::array<EdgeLinker> &sinkArray, size_t sinkIx) {
    uint32_t nForkEdge = 0;
    uint64_t totalRate = 0;
    while (sourceRate > totalRate) {
        totalRate += sinkArray[sinkIx++].rate_;
        nForkEdge += 1;
    }
    return nForkEdge;
}

/* === Function(s) definition === */

void spider::optims::optimize(spider::pisdf::Graph *graph) {
    if (!graph) {
        return;
    }
    reduceUnitaryRateActors(graph);
    bool done = false;
    while (!done) {
        done = true;
        done &= reduceForkFork(graph);
        done &= reduceJoinJoin(graph);
        done &= reduceJoinFork(graph);
    }
    reduceJoinEnd(graph);
    reduceInitEnd(graph);
}

bool spider::optims::reduceForkFork(pisdf::Graph *graph) {
    if (!graph) {
        return false;
    }
    auto verticesToOptimize = factory::vector<std::pair<pisdf::Vertex *, pisdf::Vertex *>>(StackID::TRANSFO);

    /* == Search for the pair of fork to optimize == */
    for (const auto &vertex : graph->vertices()) {
        if (vertex->subtype() == pisdf::VertexType::FORK && vertex->scheduleJobIx() == SIZE_MAX) {
            auto *source = vertex->inputEdge(0)->source();
            if (source->subtype() == pisdf::VertexType::FORK && vertex->scheduleJobIx() == SIZE_MAX) {
                verticesToOptimize.emplace_back(source, vertex.get());
            }
        }
    }

    /* == Do the optimization == */
    for (auto it = verticesToOptimize.begin(); it != verticesToOptimize.end(); ++it) {
        auto &pair = (*it);
        auto *firstFork = pair.first;
        auto *secondFork = pair.second;

        /* == Create the new fork == */
        auto *newFork = createNewFork(firstFork, secondFork);

        /* === Link the edges === */

        /* == Connect the output edges of the first Fork into the new Fork == */
        auto secondForkEdgeIx = secondFork->inputEdge(0)->sourcePortIx();
        for (size_t i = 0; i < secondForkEdgeIx; ++i) {
            auto *edge = firstFork->outputEdge(i);
            edge->setSource(newFork, i, Expression(edge->sourceRateExpression()));
        }

        /* == Remove the edge between the two Forks == */
        graph->removeEdge(firstFork->outputEdge(secondForkEdgeIx));

        /* == Connect the output edges of the second fork into the new fork == */
        for (size_t i = 0; i < secondFork->outputEdgeCount(); ++i) {
            auto *edge = secondFork->outputEdge(i);
            const auto &ix = edge->sourcePortIx() + secondForkEdgeIx;
            edge->setSource(newFork, ix, Expression(edge->sourceRateExpression()));
        }

        /* == Connect the remaining output edges of the first Fork into the new Fork == */
        const auto &offset = secondFork->outputEdgeCount() - 1;
        for (size_t i = secondForkEdgeIx + 1; i < firstFork->outputEdgeCount(); ++i) {
            auto *edge = firstFork->outputEdge(i);
            const auto &ix = edge->sourcePortIx() + offset;
            edge->setSource(newFork, ix, Expression(edge->sourceRateExpression()));
        }

        /* == Search for the pair to modify (if any) == */
        for (auto it2 = std::next(it); it2 != std::end(verticesToOptimize); ++it2) {
            auto &secPair = (*it2);
            if (secPair.first == firstFork || secPair.first == secondFork) {
                secPair.first = newFork;
            }
            if (secPair.second == firstFork || secPair.second == secondFork) {
                secPair.second = newFork;
            }
        }

        /* == Remove the vertices == */
        if (log::enabled<log::OPTIMS>()) {
            log::verbose<log::OPTIMS>("ForkForkOptimizer: removing [%s] and [%s] fork vertices.\n",
                                            secondFork->name().c_str(), firstFork->name().c_str());
        }
        graph->removeVertex(secondFork);
        graph->removeVertex(firstFork);
    }

    return verticesToOptimize.empty();
}

bool spider::optims::reduceJoinFork(pisdf::Graph *graph) {
    if (!graph) {
        return false;
    }
    auto verticesToOptimize = factory::vector<std::pair<pisdf::Vertex *, pisdf::Vertex *>>(StackID::TRANSFO);

    /* == Search for the pair of join / fork to optimize == */
    for (auto &vertex : graph->vertices()) {
        if (vertex->subtype() == pisdf::VertexType::JOIN && vertex->scheduleJobIx() == SIZE_MAX) {
            auto *sink = vertex->outputEdge(0)->sink();
            if (sink->subtype() == pisdf::VertexType::FORK && vertex->scheduleJobIx() == SIZE_MAX) {
                verticesToOptimize.emplace_back(vertex.get(), sink);
            }
        }
    }

    /* == Go through the different pair to optimize == */
    for (auto &pair : verticesToOptimize) {
        auto *join = pair.first;
        auto *fork = pair.second;
        spider::array<EdgeLinker> sourceArray{ join->inputEdgeCount(), StackID::TRANSFO };
        spider::array<EdgeLinker> sinkArray{ fork->outputEdgeCount(), StackID::TRANSFO };

        for (auto *edge : join->inputEdgeVector()) {
            const auto &rate = static_cast<uint64_t>(edge->sourceRateValue());
            sourceArray[edge->sinkPortIx()] = EdgeLinker{ edge->source(), rate, edge->sourcePortIx() };
            graph->removeEdge(edge);
        }
        graph->removeEdge(join->outputEdge(0));
        for (auto *edge : fork->outputEdgeVector()) {
            const auto &rate = static_cast<uint64_t>(edge->sinkRateValue());
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
            if (sink.rate_ == source.rate_) {
                auto sourcePortIx = source.portIx_ == UINT32_MAX ? forkEdgeIx : source.portIx_;
                const auto &srcRate = static_cast<int64_t>(source.rate_);
                const auto &snkRate = static_cast<int64_t>(sink.rate_);
                api::createEdge(source.vertex_, sourcePortIx, srcRate, sink.vertex_, sink.portIx_, snkRate);
                sourceIx += 1;
            } else if (source.rate_ > sink.rate_) {
                /* == Need for a Fork == */
                auto nForkEdge = computeNForkEdge(source.rate_, sinkArray, sinkIx);
                auto *addedFork = spider::api::createFork(graph,
                                                          "fork-" + source.vertex_->name() + "-out" +
                                                          std::to_string(source.portIx_),
                                                          nForkEdge);
                const auto &srcRate = static_cast<int64_t>(source.rate_);
                api::createEdge(source.vertex_, source.portIx_, srcRate, addedFork, 0, srcRate);
                for (size_t forkPortIx = 0; forkPortIx < addedFork->outputEdgeCount(); ++forkPortIx) {
                    sink = sinkArray[sinkIx];
                    if (source.rate_ >= sink.rate_) {
                        const auto &snkRate = static_cast<int64_t>(sink.rate_);
                        api::createEdge(addedFork, forkPortIx, snkRate, sink.vertex_, sink.portIx_, snkRate);
                        source.rate_ -= sink.rate_;
                        sinkIx += 1;
                    } else {
                        source.vertex_ = addedFork;
                        source.portIx_ = forkPortIx;
                        sinkIx -= 1;
                    }
                }
                if (source.vertex_ != addedFork) {
                    sourceIx += 1;
                    sinkIx -= 1; /* = sinkIx is going to be incremented by the for loop = */
                }
            } else {
                /* == Need for a Join == */
                auto nJoinEdge = computeNJoinEdge(sink.rate_, sourceArray, sourceIx);
                auto *addedJoin = api::createJoin(graph,
                                                  "join-" +
                                                  sink.vertex_->name() +
                                                  "-in" +
                                                  std::to_string(sink.portIx_),
                                                  nJoinEdge);
                const auto &snkRate = static_cast<int64_t>(sink.rate_);
                api::createEdge(addedJoin, 0, snkRate, sink.vertex_, sink.portIx_, snkRate);
                for (size_t joinPortIx = 0; joinPortIx < addedJoin->inputEdgeCount(); ++joinPortIx) {
                    source = sourceArray[sourceIx];
                    if (source.rate_ <= sink.rate_) {
                        const auto &srcRate = static_cast<int64_t>(source.rate_);
                        api::createEdge(source.vertex_, source.portIx_, srcRate, addedJoin, joinPortIx, srcRate);
                        sink.rate_ -= source.rate_;
                        sourceIx += 1;
                    } else {
                        sink.vertex_ = addedJoin;
                        sink.portIx_ = joinPortIx;
                        sinkIx -= 1;
                    }
                }
            }
        }
    }
    return verticesToOptimize.empty();
}

bool spider::optims::reduceJoinJoin(pisdf::Graph *graph) {
    if (!graph) {
        return false;
    }
    auto verticesToOptimize = factory::vector<std::pair<pisdf::Vertex *, pisdf::Vertex *>>(StackID::TRANSFO);

    /* == Search for the pair of fork to optimize == */
    for (auto &vertex : graph->vertices()) {
        if (vertex->subtype() == pisdf::VertexType::JOIN && vertex->scheduleJobIx() == SIZE_MAX) {
            auto *sink = vertex->outputEdge(0)->sink();
            if (sink->subtype() == pisdf::VertexType::JOIN && vertex->scheduleJobIx() == SIZE_MAX) {
                verticesToOptimize.emplace_back(vertex.get(), sink);
            }
        }
    }

    /* == Do the optimization == */
    for (auto it = verticesToOptimize.begin(); it != verticesToOptimize.end(); ++it) {
        auto &pair = (*it);
        auto *firstJoin = pair.first;
        auto *secondJoin = pair.second;

        /* == Create the new join == */
        auto *newJoin = createNewJoin(firstJoin, secondJoin);

        /* === Link the edges === */

        /* == Connect the output edges of the first Join into the new Join == */
        auto firstJoinEdgeIx = firstJoin->outputEdge(0)->sinkPortIx();
        for (size_t i = 0; i < firstJoinEdgeIx; ++i) {
            auto *edge = secondJoin->inputEdge(i);
            edge->setSink(newJoin, i, Expression(edge->sinkRateExpression()));
        }

        /* == Remove the edge between the two Joins == */
        graph->removeEdge(secondJoin->inputEdge(firstJoinEdgeIx));

        /* == Connect the input edges of the first join into the new join == */
        for (size_t i = 0; i < firstJoin->inputEdgeCount(); ++i) {
            auto *edge = firstJoin->inputEdge(i);
            const auto &ix = edge->sinkPortIx() + firstJoinEdgeIx;
            edge->setSink(newJoin, ix, Expression(edge->sinkRateExpression()));
        }

        /* == Connect the remaining output edges of the first Fork into the new Fork == */
        const auto &offset = firstJoin->inputEdgeCount() - 1;
        for (size_t i = firstJoinEdgeIx + 1; i < secondJoin->inputEdgeCount(); ++i) {
            auto *edge = secondJoin->inputEdge(i);
            const auto &ix = edge->sinkPortIx() + offset;
            edge->setSink(newJoin, ix, Expression(edge->sinkRateExpression()));
        }

        /* == Search for the pair to modify (if any) == */
        for (auto it2 = std::next(it); it2 != std::end(verticesToOptimize); ++it2) {
            auto &secPair = (*it2);
            if (secPair.first == firstJoin || secPair.first == secondJoin) {
                secPair.first = newJoin;
            }
            if (secPair.second == firstJoin || secPair.second == secondJoin) {
                secPair.second = newJoin;
            }
        }

        /* == Remove the vertices == */
        if (log::enabled<log::OPTIMS>()) {
            log::verbose<log::OPTIMS>("JoinJoinOptimizer: removing [%s] and [%s] join vertices.\n",
                                            firstJoin->name().c_str(), secondJoin->name().c_str());
        }
        graph->removeVertex(firstJoin);
        graph->removeVertex(secondJoin);
    }
    return verticesToOptimize.empty();
}

bool spider::optims::reduceJoinEnd(pisdf::Graph *graph) {
    if (!graph) {
        return false;
    }
    auto verticesToOptimize = factory::vector<pisdf::Vertex *>(StackID::TRANSFO);

    /* == Retrieve the vertices to remove == */
    for (auto &vertex : graph->vertices()) {
        if (vertex->subtype() == pisdf::VertexType::JOIN && vertex->scheduleJobIx() == SIZE_MAX) {
            auto *sink = vertex->outputEdge(0)->sink();
            if (sink->subtype() == pisdf::VertexType::END && vertex->scheduleJobIx() == SIZE_MAX) {
                verticesToOptimize.push_back(vertex.get());
            }
        }
    }

    /* == Remove useless init / end connections == */
    for (auto *join : verticesToOptimize) {
        auto *edge = join->outputEdge(0);
        auto *end = edge->sink();
        graph->removeEdge(edge);
        // TODO: see how to deal with persistent delay memory allocation
        for (auto *inputEdge : join->inputEdgeVector()) {
            auto *newEnd = api::createEnd(graph, "end-" + inputEdge->source()->name());
            inputEdge->setSink(newEnd, 0, Expression(inputEdge->sinkRateExpression()));
        }

        if (log::enabled<log::OPTIMS>()) {
            log::verbose<log::OPTIMS>("JoinEndOptimizer: removing join [%s] and end [%s] vertices.\n",
                                            join->name().c_str(), end->name().c_str());
        }
        graph->removeVertex(join);
        graph->removeVertex(end);
    }
    return verticesToOptimize.empty();
}

bool spider::optims::reduceInitEnd(pisdf::Graph *graph) {
    if (!graph) {
        return false;
    }
    auto verticesToOptimize = factory::vector<pisdf::Vertex *>(StackID::TRANSFO);

    /* == Retrieve the vertices to remove == */
    for (auto &vertex : graph->vertices()) {
        if (vertex->subtype() == pisdf::VertexType::INIT && vertex->scheduleJobIx() == SIZE_MAX) {
            auto *sink = vertex->outputEdge(0)->sink();
            if (sink->subtype() == pisdf::VertexType::END && vertex->scheduleJobIx() == SIZE_MAX) {
                verticesToOptimize.push_back(vertex.get());
            }
        }
    }

    /* == Remove useless init / end connections == */
    for (auto *init : verticesToOptimize) {
        auto *edge = init->outputEdge(0);
        auto *end = edge->sink();
        graph->removeEdge(edge);
        if (log::enabled<log::OPTIMS>()) {
            log::verbose<log::OPTIMS>("InitEndOptimizer: removing init [%s] and end [%s] vertices.\n",
                                            init->name().c_str(), end->name().c_str());
        }
        graph->removeVertex(init);
        graph->removeVertex(end);
    }
    return verticesToOptimize.empty();
}

bool spider::optims::reduceUnitaryRateActors(pisdf::Graph *graph) {
    if (!graph) {
        return false;
    }
    bool optimized = false;

    UnitaryOptimizerVisitor optimizer{ graph };
    auto it = graph->vertices().begin();
    while (it != graph->vertices().end()) {
        auto &vertex = (*it);
        vertex->visit(&optimizer);
        it += (!optimizer.removed_);
        optimized |= (optimizer.removed_);
    }
    return optimized;
}
