/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_CONSDEPENDENCIESIMPL_H
#define SPIDER2_CONSDEPENDENCIESIMPL_H

/* === Include(s) === */

#include <graphs-tools/numerical/detail/DependencyIterator.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/DelayVertex.h>

namespace spider {
    namespace pisdf {

        class Vertex;

        class Edge;

        namespace detail {

            /* === Function(s) prototype === */

            template<class ...Args>
            i32 computeConsDependency(const Edge *edge,
                                      int64_t lowerProd,
                                      int64_t upperProd,
                                      const GraphFiring *handler,
                                      Args &&...args);

            namespace impl {

                inline DependencyInfo createConsDependency(const Edge *edge,
                                                           int64_t lowerProd,
                                                           int64_t upperProd,
                                                           int64_t snkRate,
                                                           int64_t delayValue,
                                                           const pisdf::GraphFiring *handler) {
                    if (!snkRate) {
                        return { nullptr, nullptr, 0, 0, 0, 0, 0, 0 };
                    }
                    DependencyInfo dep{ };
                    dep.vertex_ = edge->sink();
                    dep.handler_ = handler;
                    dep.rate_ = snkRate;
                    dep.edgeIx_ = static_cast<u32>(edge->sinkPortIx());
                    const auto delayedLowerProd = lowerProd + delayValue;
                    const auto startDiv = delayedLowerProd / snkRate;
                    const auto memStart = delayedLowerProd % snkRate;
                    const auto delayedUpperProd = upperProd + delayValue;
                    const auto endDiv = delayedUpperProd / snkRate;
                    const auto memEnd = delayedUpperProd % snkRate;
                    /* == floor div == */
                    const auto start = memStart ? startDiv - (delayedLowerProd < 0) : startDiv;
                    const auto end = memEnd ? endDiv - (delayedUpperProd < 0) : endDiv;
                    dep.memoryStart_ = static_cast<u32>(memStart);
                    dep.memoryEnd_ = static_cast<u32>(memEnd);
                    dep.firingStart_ = static_cast<u32>(start);
                    dep.firingEnd_ = static_cast<u32>(end);
                    return dep;
                }

                template<class ...Args>
                i32 computeConsDependencyOutput(const Edge *edge,
                                                int64_t lowerProd,
                                                int64_t upperProd,
                                                int64_t totalRate,
                                                const pisdf::GraphFiring *handler,
                                                Args &&...args) {
                    /* == Case of output interface == */
                    const auto *sink = edge->sink();
                    const auto srcRate = handler->getSrcRate(edge);
                    const auto srcRV = handler->getRV(edge->source());
                    const auto snkRate = handler->getSnkRate(edge);
                    const auto *delay = edge->delay();
                    const auto delayValue = delay ? delay->value() : 0;
                    auto dep = createConsDependency(edge, lowerProd, upperProd, totalRate, delayValue, handler);
                    /* == Now check where we fall == */
                    const auto minValidMemWDelay = srcRate * srcRV - snkRate;
                    const auto minValidMemWODelay = minValidMemWDelay + delayValue;
                    const auto parentLProd = snkRate * handler->firingValue();
                    const auto *upperEdge = sink->graph()->outputEdge(sink->ix());
                    const auto *gh = handler->getParent()->handler();
                    if (dep.memoryEnd_ < minValidMemWDelay) {
                        /* == void dependency == */
                        return -1;
                    } else if ((dep.memoryStart_ >= minValidMemWODelay) ||
                               (!delayValue && (dep.memoryEnd_ >= minValidMemWODelay))) {
                        /* == forward dependency == */
                        lowerProd = parentLProd +
                                    std::max(int64_t{ 0 }, int64_t{ dep.memoryStart_ - minValidMemWODelay });
                        upperProd = parentLProd + (dep.memoryEnd_ - minValidMemWODelay);
                        return computeConsDependency(upperEdge, lowerProd, upperProd, gh,
                                                     std::forward<Args>(args)...);
                    } else if (delay && dep.memoryEnd_ < minValidMemWODelay) {
                        /* == getter only == */
                        lowerProd = std::max(int64_t{ 0 }, int64_t{ dep.memoryStart_ - minValidMemWDelay });
                        upperProd = dep.memoryEnd_ - minValidMemWDelay;
                        const auto *getterEdge = delay->getter()->inputEdge(delay->getterPortIx());
                        return computeConsDependency(getterEdge, lowerProd, upperProd, handler,
                                                     std::forward<Args>(args)...);
                    } else if (delay) {
                        /* == mix of getter and interface == */
                        const auto *getterEdge = delay->getter()->inputEdge(delay->getterPortIx());
                        const auto getterLowerProd = dep.memoryStart_ - minValidMemWDelay;
                        /* == Getter dependencies, same level as current actor == */
                        auto count = computeConsDependency(getterEdge, getterLowerProd, delayValue - 1, handler,
                                                           std::forward<Args>(args)...);
                        /* == Sink dependencies, one level up of the one of current actor == */
                        lowerProd = parentLProd +
                                    std::max(int64_t{ 0 }, int64_t{ dep.memoryStart_ - minValidMemWODelay });
                        upperProd = parentLProd + (dep.memoryEnd_ - minValidMemWODelay);
                        return count + computeConsDependency(upperEdge, lowerProd, upperProd, gh,
                                                             std::forward<Args>(args)...);
                    } else {
                        throwSpiderException("unexpected behavior.");
                    }
                }

                template<class ...Args>
                i32 computeConsDependencyGraph(const Edge *edge,
                                               int64_t lowerProd,
                                               int64_t upperProd,
                                               int64_t delayValue,
                                               const pisdf::GraphFiring *handler,
                                               Args &&...args) {
                    static DependencyInfo unresolved = { nullptr, nullptr, -1,
                                                         UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };
                    /* == Case of source graph == */
                    const auto *sink = edge->sink();
                    const auto snkRate = handler->getSnkRate(edge);
                    const auto firingStart = static_cast<u32>(math::floorDiv(lowerProd + delayValue, snkRate));
                    const auto firingEnd = static_cast<u32>(math::floorDiv(upperProd + delayValue, snkRate));
                    const auto *graph = sink->convertTo<pisdf::Graph>();
                    const auto *innerEdge = graph->inputInterface(edge->sinkPortIx())->edge();
                    i32 count = 0;
                    for (auto k = firingStart; k <= firingEnd; ++k) {
                        const auto *gh = handler->getSubgraphGraphFiring(graph, k);
                        if (gh->isResolved()) {
                            const auto adjustedSnkRate = gh->getSnkRate(innerEdge) * gh->getRV(innerEdge->sink());
                            const auto fullRepCount = adjustedSnkRate / snkRate;
                            const auto lProd = k == firingStart ? (lowerProd + delayValue) % snkRate : 0;
                            const auto uProd = k == firingEnd ? (upperProd + delayValue) % snkRate : snkRate - 1;
                            for (auto i = 0; i < fullRepCount; ++i) {
                                const auto offset = i * snkRate;
                                count += computeConsDependency(innerEdge, lProd + offset, uProd + offset, gh,
                                                               std::forward<Args>(args)...);
                            }
                            const auto lp = lProd + fullRepCount * snkRate;
                            if ((snkRate * fullRepCount) != adjustedSnkRate && (lp < adjustedSnkRate)) {
                                const auto up = std::min(uProd + fullRepCount * snkRate, adjustedSnkRate - 1);
                                count += computeConsDependency(innerEdge, lp, up, gh, std::forward<Args>(args)...);
                            }
                        } else {
                            impl::apply(unresolved, std::forward<Args>(args)...);
                        }
                    }
                    return count;
                }
            }

            template<class ...Args>
            i32 computeConsDependency(const Edge *edge,
                                      int64_t lowerProd,
                                      int64_t upperProd,
                                      const pisdf::GraphFiring *handler,
                                      Args &&...args) {
                /* == Precompute some numerical values == */
                const auto *sink = edge->sink();
                const auto sinkType = sink->subtype();
                const auto snkRate = handler->getSnkRate(edge);
                const auto srcRate = handler->getSrcRate(edge);
                const auto srcRV = handler->getRV(edge->source());
                const auto snkRV = handler->getRV(sink);
                const auto *delay = edge->delay();
                const auto delayValue = delay ? delay->value() : 0;
                const auto totalRate = sinkType == VertexType::OUTPUT ? srcRate * srcRV + delayValue : snkRate * snkRV;
                const auto delayedTotalRate = sinkType == VertexType::OUTPUT ? totalRate : totalRate - delayValue;
                if (!srcRate) {
                    impl::apply({ nullptr, nullptr, 0, 0, 0, 0, 0, 0 }, std::forward<Args>(args)...);
                    return 0;
                }
                /* == Handle specific cases == */
                if (sinkType == VertexType::DELAY) {
                    /* == Case of setter vertex == */
                    const auto *nextDelay = sink->convertTo<pisdf::DelayVertex>()->delay();
                    const auto offset = nextDelay->value() - delayValue;
                    return computeConsDependency(nextDelay->edge(), lowerProd - offset, upperProd - offset, handler,
                                                 std::forward<Args>(args)...);
                } else if (delay && (lowerProd >= delayedTotalRate)) {
                    /* == getter only == */
                    const auto *getterEdge = delay->getter()->inputEdge(delay->getterPortIx());
                    return computeConsDependency(getterEdge, lowerProd - delayedTotalRate, upperProd - delayedTotalRate,
                                                 handler, std::forward<Args>(args)...);
                } else if (upperProd < delayedTotalRate) {
                    /* == sink only == */
                    if (sinkType == VertexType::OUTPUT) {
                        return impl::computeConsDependencyOutput(edge, lowerProd, upperProd, totalRate, handler,
                                                                 std::forward<Args>(args)...);
                    } else if (sinkType == VertexType::GRAPH) {
                        return impl::computeConsDependencyGraph(edge, lowerProd, upperProd, delayValue, handler,
                                                                std::forward<Args>(args)...);
                    } else {
                        auto dep = impl::createConsDependency(edge, lowerProd, upperProd, snkRate, delayValue, handler);
                        impl::apply(dep, std::forward<Args>(args)...);
                        return static_cast<i32>(dep.firingEnd_ - dep.firingStart_ + 1);
                    }
                } else if (delay) {
                    /* == sink + getter == */
                    const auto *getterEdge = delay->getter()->inputEdge(delay->getterPortIx());
                    return computeConsDependency(edge, lowerProd, totalRate - delayValue - 1, handler,
                                                 std::forward<Args>(args)...) +
                           computeConsDependency(getterEdge, 0, upperProd - delayedTotalRate, handler,
                                                 std::forward<Args>(args)...);
                } else {
                    throwSpiderException("unexpected behavior.");
                }
            }
        }
    }
}
#endif //SPIDER2_CONSDEPENDENCIESIMPL_H