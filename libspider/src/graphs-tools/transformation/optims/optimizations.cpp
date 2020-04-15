/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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
#include <graphs-tools/transformation/optims/helper/unitaryOptimizer.h>
#include <graphs-tools/transformation/optims/helper/patternOptimizer.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/DelayVertex.h>
#include <api/pisdf-api.h>

/* === Private structure(s) === */

struct EdgeLinker {
    spider::pisdf::Vertex *vertex_ = nullptr;
    int64_t rate_ = 0;
    size_t portIx_ = 0;

    EdgeLinker() = default;

    EdgeLinker(spider::pisdf::Vertex *vertex, int64_t rate, size_t portIx) : vertex_{ vertex },
                                                                             rate_{ rate },
                                                                             portIx_{ portIx } { };
};

/* === Static function(s) === */

/**
 * @brief Creates one new @refitem pisdf::VertexType::FORK out of two.
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
static spider::pisdf::Vertex *createNewFork(spider::pisdf::Vertex *secondFork, spider::pisdf::Vertex *firstFork) {
    auto *graph = firstFork->graph();
    const auto &outputCount = static_cast<uint32_t>((firstFork->outputEdgeCount() - 1) +
                                                    secondFork->outputEdgeCount());
    auto *newFork = spider::api::createFork(graph,
                                            "merged-" + firstFork->name() + "-" + secondFork->name(),
                                            outputCount);

    /* == Connect the input of the first Fork to the new Fork == */
    auto *edge = firstFork->inputEdge(0);
    edge->setSink(newFork, 0, edge->sinkRateExpression());
    return newFork;
}

/**
 * @brief Creates one new @refitem pisdf::VertexType::DUPLICATE out of two.
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
 * @param firstDuplicate   Pointer to the first vertex.
 * @param secondDuplicate  Pointer to the secondary vertex.
 * @return pointer to the created Vertex.
 */
static spider::pisdf::Vertex *
createNewDuplicate(spider::pisdf::Vertex *secondDuplicate, spider::pisdf::Vertex *firstDuplicate) {
    auto *graph = firstDuplicate->graph();
    const auto &outputCount = static_cast<uint32_t>((firstDuplicate->outputEdgeCount() - 1) +
                                                    secondDuplicate->outputEdgeCount());
    auto *newDupl = spider::api::createDuplicate(graph,
                                                 "merged-" + firstDuplicate->name() + "-" + secondDuplicate->name(),
                                                 outputCount);

    /* == Connect the input of the first Fork to the new Fork == */
    auto *edge = firstDuplicate->inputEdge(0);
    edge->setSink(newDupl, 0, edge->sinkRateExpression());
    return newDupl;
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
    edge->setSource(newJoin, 0, edge->sourceRateExpression());
    return newJoin;
}

static size_t computeNEdge(int64_t sinkRate, spider::array<EdgeLinker>::iterator it) {
    size_t edgeCount = 0;
    int64_t totalRate = 0;
    while (sinkRate > totalRate) {
        totalRate += (*(it++)).rate_;
        edgeCount += 1;
    }
    return edgeCount;
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
    reduceRepeatFork(graph);
    reduceDupDup(graph);
    reduceJoinEnd(graph);
    reduceInitEnd(graph);
}

bool spider::optims::reduceRepeatFork(spider::pisdf::Graph *graph) {
    if (!graph) {
        return false;
    }
    auto verticesToOptimize = factory::vector<pisdf::Vertex *>(StackID::TRANSFO);
    /* == Retrieve the vertices to remove == */
    for (auto &vertex : graph->vertices()) {
        if (vertex->subtype() == pisdf::VertexType::REPEAT && vertex->scheduleTaskIx() == SIZE_MAX) {
            auto inputRate = vertex->inputEdge(0)->sinkRateValue();
            auto outputRate = vertex->outputEdge(0)->sourceRateValue();
            auto *sink = vertex->outputEdge(0)->sink();
            if (!(outputRate % inputRate) &&
                (sink->subtype() == pisdf::VertexType::FORK && sink->scheduleTaskIx() == SIZE_MAX)) {
                verticesToOptimize.push_back(vertex.get());
            }
        }
    }

    /* == Remove repeate / fork connections and replace them with duplicate vertex == */
    for (auto *repeat : verticesToOptimize) {
        auto *edge = repeat->outputEdge(0);
        auto *fork = edge->sink();
        auto *duplicate = api::createDuplicate(graph, repeat->name(), fork->outputEdgeCount());
        auto *inEdge = repeat->inputEdge(0);
        inEdge->setSink(duplicate, 0, inEdge->sinkRateExpression());
        for (auto *outputEdge : fork->outputEdgeVector()) {
            outputEdge->setSource(duplicate, outputEdge->sourcePortIx(), outputEdge->sourceRateExpression());
        }
        if (log::enabled<log::OPTIMS>()) {
            log::verbose<log::OPTIMS>("reduceRepeatFork: removing repeat [%s] and fork [%s] vertices.\n",
                                      repeat->name().c_str(), fork->name().c_str());
        }
        graph->removeEdge(edge);
        graph->removeVertex(repeat);
        graph->removeVertex(fork);
    }
    return verticesToOptimize.empty();
}

bool spider::optims::reduceDupDup(pisdf::Graph *graph) {
    if (!graph) {
        return false;
    }
    /* == Declare the lambdas == */
    auto getNextVertex = [](pisdf::Vertex *vertex) -> pisdf::Vertex * {
        return vertex->inputEdge(0)->source();
    };
    auto removeEdge = [](pisdf::Vertex *vertex, pisdf::Vertex *vertexB) -> size_t {
        const auto offset = vertex->inputEdge(0)->sourcePortIx();
        vertex->graph()->removeEdge(vertexB->outputEdge(offset));
        return offset;
    };
    auto reconnectEdge = [](pisdf::Vertex *vertex, size_t srcIx, pisdf::Vertex *target, size_t snkIx) {
        auto *edge = vertex->outputEdge(srcIx);
        edge->setSource(target, snkIx, edge->sourceRateExpression());
    };

    /* == Do the optimization == */
    return reduceFFJJWorker(pisdf::VertexType::DUPLICATE,
                            graph,
                            createNewDuplicate,
                            std::move(getNextVertex),
                            std::move(removeEdge),
                            std::move(reconnectEdge));
}

bool spider::optims::reduceForkFork(pisdf::Graph *graph) {
    if (!graph) {
        return false;
    }
    /* == Declare the lambdas == */
    auto getNextVertex = [](pisdf::Vertex *vertex) -> pisdf::Vertex * {
        return vertex->inputEdge(0)->source();
    };
    auto removeEdge = [](pisdf::Vertex *vertex, pisdf::Vertex *vertexB) -> size_t {
        const auto offset = vertex->inputEdge(0)->sourcePortIx();
        vertex->graph()->removeEdge(vertexB->outputEdge(offset));
        return offset;
    };
    auto reconnectEdge = [](pisdf::Vertex *vertex, size_t srcIx, pisdf::Vertex *target, size_t snkIx) {
        auto *edge = vertex->outputEdge(srcIx);
        edge->setSource(target, snkIx, edge->sourceRateExpression());
    };

    /* == Do the optimization == */
    return reduceFFJJWorker(pisdf::VertexType::FORK,
                            graph,
                            createNewFork,
                            std::move(getNextVertex),
                            std::move(removeEdge),
                            std::move(reconnectEdge));
}

bool spider::optims::reduceJoinJoin(pisdf::Graph *graph) {
    if (!graph) {
        return false;
    }
    /* == Declare the lambdas == */
    auto getNextVertex = [](pisdf::Vertex *vertex) -> pisdf::Vertex * {
        return vertex->outputEdge(0)->sink();
    };
    auto removeEdge = [](pisdf::Vertex *vertex, pisdf::Vertex *vertexB) -> size_t {
        const auto offset = vertex->outputEdge(0)->sinkPortIx();
        vertex->graph()->removeEdge(vertexB->inputEdge(offset));
        return offset;
    };
    auto reconnectEdge = [](pisdf::Vertex *vertex, size_t srcIx, pisdf::Vertex *target, size_t snkIx) {
        auto *edge = vertex->inputEdge(srcIx);
        edge->setSink(target, snkIx, edge->sinkRateExpression());
    };
    /* == Do the optimization == */
    return reduceFFJJWorker(pisdf::VertexType::JOIN,
                            graph,
                            createNewJoin,
                            std::move(getNextVertex),
                            std::move(removeEdge),
                            std::move(reconnectEdge));
}

bool spider::optims::reduceJoinFork(pisdf::Graph *graph) {
    if (!graph) {
        return false;
    }
    auto verticesToOptimize = factory::vector<pisdf::Vertex *>(StackID::TRANSFO);

    /* == Search for the pair of join / fork to optimize == */
    for (const auto &vertex : graph->vertices()) {
        if (vertex->subtype() == pisdf::VertexType::JOIN && vertex->scheduleTaskIx() == SIZE_MAX) {
            auto *sink = vertex->outputEdge(0)->sink();
            if (sink->subtype() == pisdf::VertexType::FORK && sink->scheduleTaskIx() == SIZE_MAX) {
                verticesToOptimize.emplace_back(vertex.get());
            }
        }
    }

    /* == Go through the different pair to optimize == */
    for (auto &join : verticesToOptimize) {
        auto *fork = join->outputEdge(0)->sink();
        spider::array<EdgeLinker> sourceArray{ join->inputEdgeCount(), StackID::TRANSFO };
        spider::array<EdgeLinker> sinkArray{ fork->outputEdgeCount(), StackID::TRANSFO };

        for (auto *edge : join->inputEdgeVector()) {
            sourceArray[edge->sinkPortIx()] = EdgeLinker{ edge->source(), edge->sourceRateValue(),
                                                          edge->sourcePortIx() };
            graph->removeEdge(edge);
        }
        graph->removeEdge(join->outputEdge(0));
        for (auto *edge : fork->outputEdgeVector()) {
            sinkArray[edge->sourcePortIx()] = EdgeLinker{ edge->sink(), edge->sinkRateValue(), edge->sinkPortIx() };
            graph->removeEdge(edge);
        }

        /* == Remove fork / join == */
        graph->removeVertex(join);
        graph->removeVertex(fork);

        /* == Re-do the linking == */
        auto sourceIt = std::begin(sourceArray);
        auto sinkIt = std::begin(sinkArray);
        while (sinkIt != std::end(sinkArray)) {
            if (sinkIt->rate_ == sourceIt->rate_) {
                api::createEdge(sourceIt->vertex_, sourceIt->portIx_, sourceIt->rate_, sinkIt->vertex_, sinkIt->portIx_,
                                sinkIt->rate_);
                sourceIt++;
                sinkIt++;
            } else if (sourceIt->rate_ > sinkIt->rate_) {
                /* == Need for a Fork == */
                auto name = std::string("fork::").append(sourceIt->vertex_->name()).append("::out::").append(
                        std::to_string(sourceIt->portIx_));
                const auto nForkEdge = computeNEdge(sourceIt->rate_, sinkIt);
                auto *addedFork = spider::api::createFork(graph, std::move(name), nForkEdge);
                api::createEdge(sourceIt->vertex_, sourceIt->portIx_, sourceIt->rate_, addedFork, 0, sourceIt->rate_);
                for (size_t forkPortIx = 0; forkPortIx < (nForkEdge - 1); ++forkPortIx) {
                    api::createEdge(addedFork, forkPortIx, sinkIt->rate_,
                                    sinkIt->vertex_, sinkIt->portIx_, sinkIt->rate_);
                    sourceIt->rate_ -= sinkIt->rate_;
                    sinkIt++;
                }
                sourceIt->vertex_ = addedFork;
                sourceIt->portIx_ = (nForkEdge - 1);
            } else {
                /* == Need for a Join == */
                auto name = std::string("join::").append(sinkIt->vertex_->name()).append("::in::").append(
                        std::to_string(sinkIt->portIx_));
                const auto nJoinEdge = computeNEdge(sinkIt->rate_, sourceIt);
                auto *addedJoin = api::createJoin(graph, std::move(name), nJoinEdge);
                api::createEdge(addedJoin, 0, sinkIt->rate_, sinkIt->vertex_, sinkIt->portIx_, sinkIt->rate_);
                for (size_t joinPortIx = 0; joinPortIx < (nJoinEdge - 1); ++joinPortIx) {
                    api::createEdge(sourceIt->vertex_, sourceIt->portIx_, sourceIt->rate_,
                                    addedJoin, joinPortIx, sourceIt->rate_);
                    sinkIt->rate_ -= sourceIt->rate_;
                    sourceIt++;
                }
                sinkIt->vertex_ = addedJoin;
                sinkIt->portIx_ = (nJoinEdge - 1);
            }
        }
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
        if (vertex->subtype() == pisdf::VertexType::JOIN && vertex->scheduleTaskIx() == SIZE_MAX) {
            auto *sink = vertex->outputEdge(0)->sink();
            if (sink->subtype() == pisdf::VertexType::END && sink->scheduleTaskIx() == SIZE_MAX) {
                verticesToOptimize.push_back(vertex.get());
            }
        }
    }

    /* == Remove useless init / end connections == */
    for (auto *join : verticesToOptimize) {
        auto *edge = join->outputEdge(0);
        auto *end = edge->sink();
        auto *ref = const_cast<pisdf::Vertex *>(end->reference());
        auto *refSource = ref->inputEdge(0)->source();
        if (refSource->subtype() == pisdf::VertexType::DELAY) {
            auto *delay = ref->inputEdge(0)->source()->convertTo<pisdf::DelayVertex>()->delay();
            if (delay->isPersistent()) {
                continue;
            }
        }
        graph->removeEdge(edge);
        for (auto *inputEdge : join->inputEdgeVector()) {
            auto *newEnd = api::createEnd(graph, "end-" + inputEdge->source()->name());
            if (ref != end) {
                ref->setAsReference(newEnd);
            }
            inputEdge->setSink(newEnd, 0, inputEdge->sinkRateExpression());
        }

        if (log::enabled<log::OPTIMS>()) {
            log::verbose<log::OPTIMS>("reduceJoinEnd: removing join [%s] and end [%s] vertices.\n",
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
        if (vertex->subtype() == pisdf::VertexType::INIT && vertex->scheduleTaskIx() == SIZE_MAX) {
            auto *sink = vertex->outputEdge(0)->sink();
            if (sink->subtype() == pisdf::VertexType::END && sink->scheduleTaskIx() == SIZE_MAX) {
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

    auto it = graph->vertices().begin();
    while (it != graph->vertices().end()) {
        auto &vertex = (*it);
        auto result = optimizeUnitaryVertex(vertex.get());
        it += (!result);
        optimized |= (result);
    }
    return optimized;
}
