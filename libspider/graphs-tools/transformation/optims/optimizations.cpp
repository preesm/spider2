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
#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <graphs-tools/transformation/optims/optimizations.h>
#include <graphs-tools/transformation/optims/helper/unitaryOptimizer.h>
#include <graphs-tools/transformation/optims/helper/patternOptimizer.h>
#include <graphs-tools/transformation/optims/helper/partialSingleRate.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/srdag/SRDAGGraph.h>
#include <graphs/srdag/SRDAGEdge.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <common/Printer.h>

/* === Static function(s) === */

static u32 countNonNullEdges(const spider::array_view<spider::srdag::Edge *> &edges) {
    u32 count = 0;
    for (const auto *edge : edges) {
        if (edge) {
            count += edge->sourceRateValue() != 0;
        }
    }
    return count;
}

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
static spider::srdag::Vertex *
createNewFork(const spider::srdag::Vertex *secondFork, const spider::srdag::Vertex *firstFork) {
    auto *graph = const_cast<spider::srdag::Graph *>(firstFork->graph());
    const auto outputCount = countNonNullEdges(firstFork->outputEdges()) +
                             countNonNullEdges(secondFork->outputEdges()) - 1;
    auto *newFork = graph->createForkVertex("merged-" + firstFork->name() + "-" + secondFork->name(), outputCount);
    /* == Connect the input of the first Fork to the new Fork == */
    auto *edge = firstFork->inputEdge(0);
    edge->setSink(newFork, 0);
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
static spider::srdag::Vertex *
createNewDuplicate(const spider::srdag::Vertex *secondDuplicate, const spider::srdag::Vertex *firstDuplicate) {
    auto *graph = const_cast<spider::srdag::Graph *>(firstDuplicate->graph());
    const auto outputCount = countNonNullEdges(firstDuplicate->outputEdges()) +
                             countNonNullEdges(secondDuplicate->outputEdges()) - 1;
    auto *newDupl = graph->createDuplicateVertex("merged-" + firstDuplicate->name() + "-" + secondDuplicate->name(),
                                                 outputCount);
    /* == Connect the input of the first Fork to the new Fork == */
    auto *edge = firstDuplicate->inputEdge(0);
    edge->setSink(newDupl, 0);
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
static spider::srdag::Vertex *
createNewJoin(const spider::srdag::Vertex *firstJoin, const spider::srdag::Vertex *secondJoin) {
    auto *graph = const_cast<spider::srdag::Graph *>(firstJoin->graph());
    const auto inputCount = countNonNullEdges(firstJoin->inputEdges()) +
                            countNonNullEdges(secondJoin->inputEdges()) - 1;
    auto *newJoin = graph->createJoinVertex("merged-" + firstJoin->name() + "-" + secondJoin->name(), inputCount);
    /* == Connect the output of the second Join to the new Join == */
    auto *edge = secondJoin->outputEdge(0);
    edge->setSource(newJoin, 0);
    return newJoin;
}

/* === Function(s) definition === */

void spider::optims::optimize(spider::srdag::Graph *graph) {
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

bool spider::optims::reduceRepeatFork(spider::srdag::Graph *graph) {
    if (!graph) {
        return false;
    }

    /* == Retrieve the vertices to remove == */
    auto verticesToOptimize = factory::vector<srdag::Vertex *>(StackID::TRANSFO);
    for (auto &vertex : graph->vertices()) {
        if (vertex->subtype() == pisdf::VertexType::REPEAT && vertex->scheduleTaskIx() == SIZE_MAX) {
            auto inputRate = vertex->inputEdge(0)->sinkRateValue();
            auto outputRate = vertex->outputEdge(0)->sourceRateValue();
            const auto *sink = vertex->outputEdge(0)->sink();
            if (inputRate && !(outputRate % inputRate) &&
                (sink->subtype() == pisdf::VertexType::FORK && sink->scheduleTaskIx() == SIZE_MAX)) {
                verticesToOptimize.push_back(vertex.get());
            }
        }
    }

    /* == Remove repeat / fork connections and replace them with duplicate vertex == */
    for (auto &repeat : verticesToOptimize) {
        auto *outEdge = repeat->outputEdge(0);
        auto *inEdge = repeat->inputEdge(0);
        const auto inRate = inEdge->sinkRateValue();
        const auto nEdges = static_cast<size_t>(outEdge->sourceRateValue() / inRate);
        auto *duplicate = graph->createDuplicateVertex(repeat->name(), nEdges);
        inEdge->setSink(duplicate, 0);

        /* == Creates the source array == */
        spider::array<EdgeLinker> sourceArray{ nEdges, StackID::TRANSFO };
        for (size_t i = 0; i < nEdges; ++i) {
            sourceArray[i] = EdgeLinker{ duplicate, inRate, i };
        }

        /* == Creates the sink array == */
        auto *fork = outEdge->sink();
        spider::array<EdgeLinker> sinkArray{ fork->outputEdgeCount(), StackID::TRANSFO };
        for (auto *edge : fork->outputEdges()) {
            sinkArray[edge->sourcePortIx()] = EdgeLinker{ edge->sink(), edge->sinkRateValue(), edge->sinkPortIx() };
            graph->removeEdge(edge);
        }

        /* == Re-do the linking == */
        partialSingleRateTransformation(graph, sourceArray, sinkArray);

        if (log::enabled<log::OPTIMS>()) {
            log::verbose<log::OPTIMS>("reduceRepeatFork: removing repeat [%s] and fork [%s] vertices.\n",
                                      repeat->name().c_str(), fork->name().c_str());
        }
        graph->removeVertex(repeat);
        graph->removeVertex(fork);
        graph->removeEdge(outEdge);
    }
    return verticesToOptimize.empty();
}

bool spider::optims::reduceDupDup(srdag::Graph *graph) {
    if (!graph) {
        return false;
    }
    /* == Declare the lambdas == */
    auto getNextVertex = [](const srdag::Vertex *vertex) -> srdag::Vertex * {
        return vertex->inputEdge(0)->source();
    };
    auto removeEdge = [](srdag::Vertex *vertex, const srdag::Vertex *vertexB) -> size_t {
        const auto offset = vertex->inputEdge(0)->sourcePortIx();
        vertex->graph()->removeEdge(vertexB->outputEdge(offset));
        return offset;
    };
    auto reconnectEdge = [](srdag::Vertex *vertex, size_t srcIx, srdag::Vertex *target, size_t snkIx) {
        auto *edge = vertex->outputEdge(srcIx);
        if (edge && !edge->sourceRateValue() && !edge->sink()->executable()) {
            vertex->graph()->removeVertex(edge->sink());
            vertex->graph()->removeEdge(edge);
        } else if (edge) {
            edge->setSource(target, snkIx);
        }
    };
    auto countEdges = [](const srdag::Vertex *vertex) -> u32 {
        return countNonNullEdges(vertex->outputEdges());
    };

    /* == Do the optimization == */
    return reduceFFJJWorker(pisdf::VertexType::DUPLICATE,
                            graph,
                            createNewDuplicate,
                            std::move(getNextVertex),
                            std::move(removeEdge),
                            std::move(countEdges),
                            std::move(reconnectEdge));
}

bool spider::optims::reduceForkFork(srdag::Graph *graph) {
    if (!graph) {
        return false;
    }
    /* == Declare the lambdas == */
    auto getNextVertex = [](const srdag::Vertex *vertex) -> srdag::Vertex * {
        return vertex->inputEdge(0)->source();
    };
    auto removeEdge = [](srdag::Vertex *vertex, const srdag::Vertex *vertexB) -> size_t {
        const auto offset = vertex->inputEdge(0)->sourcePortIx();
        vertex->graph()->removeEdge(vertexB->outputEdge(offset));
        return offset;
    };
    auto reconnectEdge = [](srdag::Vertex *vertex, size_t srcIx, srdag::Vertex *target, size_t snkIx) {
        auto *edge = vertex->outputEdge(srcIx);
        if (edge && !edge->sourceRateValue() && !edge->sink()->executable()) {
            vertex->graph()->removeVertex(edge->sink());
            vertex->graph()->removeEdge(edge);
        } else if (edge) {
            edge->setSource(target, snkIx);
        }
    };
    auto countEdges = [](const srdag::Vertex *vertex) -> u32 {
        return countNonNullEdges(vertex->outputEdges());
    };

    /* == Do the optimization == */
    return reduceFFJJWorker(pisdf::VertexType::FORK,
                            graph,
                            createNewFork,
                            std::move(getNextVertex),
                            std::move(removeEdge),
                            std::move(countEdges),
                            std::move(reconnectEdge));
}

bool spider::optims::reduceJoinJoin(srdag::Graph *graph) {
    if (!graph) {
        return false;
    }
    /* == Declare the lambdas == */
    auto getNextVertex = [](const srdag::Vertex *vertex) -> srdag::Vertex * {
        return vertex->outputEdge(0)->sink();
    };
    auto removeEdge = [](srdag::Vertex *vertex, const srdag::Vertex *vertexB) -> size_t {
        const auto offset = vertex->outputEdge(0)->sinkPortIx();
        vertex->graph()->removeEdge(vertexB->inputEdge(offset));
        return offset;
    };
    auto reconnectEdge = [](srdag::Vertex *vertex, size_t srcIx, srdag::Vertex *target, size_t snkIx) {
        auto *edge = vertex->inputEdge(srcIx);
        if (edge && !edge->sinkRateValue() && !edge->source()->executable()) {
            vertex->graph()->removeVertex(edge->source());
            vertex->graph()->removeEdge(edge);
        } else if (edge) {
            edge->setSink(target, snkIx);
        }
    };
    auto countEdges = [](const srdag::Vertex *vertex) -> u32 {
        return countNonNullEdges(vertex->inputEdges());
    };
    /* == Do the optimization == */
    return reduceFFJJWorker(pisdf::VertexType::JOIN,
                            graph,
                            createNewJoin,
                            std::move(getNextVertex),
                            std::move(removeEdge),
                            std::move(countEdges),
                            std::move(reconnectEdge));
}

bool spider::optims::reduceJoinFork(srdag::Graph *graph) {
    if (!graph) {
        return false;
    }
    auto verticesToOptimize = factory::vector<srdag::Vertex *>(StackID::TRANSFO);

    /* == Search for the pair of join / fork to optimize == */
    for (const auto &vertex : graph->vertices()) {
        if (vertex->subtype() == pisdf::VertexType::JOIN && vertex->scheduleTaskIx() == SIZE_MAX) {
            const auto *sink = vertex->outputEdge(0)->sink();
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

        for (auto *edge : join->inputEdges()) {
            sourceArray[edge->sinkPortIx()] = EdgeLinker{ edge->source(), edge->sourceRateValue(),
                                                          edge->sourcePortIx() };
            graph->removeEdge(edge);
        }
        graph->removeEdge(join->outputEdge(0));
        for (auto *edge : fork->outputEdges()) {
            sinkArray[edge->sourcePortIx()] = EdgeLinker{ edge->sink(), edge->sinkRateValue(), edge->sinkPortIx() };
            graph->removeEdge(edge);
        }

        /* == Remove fork / join == */
        graph->removeVertex(join);
        graph->removeVertex(fork);

        /* == Re-do the linking == */
        partialSingleRateTransformation(graph, sourceArray, sinkArray);
    }
    return verticesToOptimize.empty();
}

bool spider::optims::reduceJoinEnd(srdag::Graph *graph) {
    if (!graph) {
        return false;
    }
    auto verticesToOptimize = factory::vector<srdag::Vertex *>(StackID::TRANSFO);

    /* == Retrieve the vertices to remove == */
    for (auto &vertex : graph->vertices()) {
        if (vertex->subtype() == pisdf::VertexType::JOIN && vertex->scheduleTaskIx() == SIZE_MAX) {
            const auto *sink = vertex->outputEdge(0)->sink();
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
        if (ref->inputEdge(0)) {
            const auto *refSource = ref->inputEdge(0)->source();
            if (refSource && refSource->subtype() == pisdf::VertexType::DELAY) {
                const auto *delay = ref->inputEdge(0)->source()->convertTo<pisdf::DelayVertex>()->delay();
                if (delay->isPersistent()) {
                    continue;
                }
            }
        }
        graph->removeEdge(edge);
        for (auto *inputEdge : join->inputEdges()) {
            auto *newEnd = graph->createEndVertex("end-" + inputEdge->source()->name());
            inputEdge->setSink(newEnd, 0);
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

bool spider::optims::reduceInitEnd(srdag::Graph *graph) {
    if (!graph) {
        return false;
    }
    auto verticesToOptimize = factory::vector<srdag::Vertex *>(StackID::TRANSFO);

    /* == Retrieve the vertices to remove == */
    for (auto &vertex : graph->vertices()) {
        if (vertex->subtype() == pisdf::VertexType::INIT && vertex->scheduleTaskIx() == SIZE_MAX) {
            const auto *sink = vertex->outputEdge(0)->sink();
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

bool spider::optims::reduceUnitaryRateActors(const srdag::Graph *graph) {
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

#endif
