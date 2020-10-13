/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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

#include <graphs-tools/numerical/detail/dependenciesImpl.h>
#include <graphs-tools/transformation/srless/GraphHandler.h>
#include <graphs-tools/transformation/srless/GraphFiring.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/DelayVertex.h>
#include <iterator>

/* === Static variable(s) === */

namespace spider {
    namespace pisdf {
        static DependencyInfo unresolved = { nullptr, nullptr, -1,
                                             UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };

    }
}

/* === Static function(s) === */

namespace spider {
    namespace pisdf {
        namespace detail {
            DependencyInfo createExecDependency(const Edge *edge,
                                                int64_t lowerCons,
                                                int64_t upperCons,
                                                int64_t srcRate,
                                                int64_t delayValue,
                                                const srless::GraphFiring *handler) {
                if (!srcRate) {
                    return { nullptr, nullptr, 0, 0, 0, 0, 0, 0 };
                }
                DependencyInfo dep{ };
                dep.vertex_ = edge->source();
                dep.handler_ = handler;
                dep.rate_ = srcRate;
                dep.edgeIx_ = static_cast<u32>(edge->sourcePortIx());
                dep.memoryStart_ = static_cast<u32>((lowerCons - delayValue) % srcRate);
                dep.memoryEnd_ = static_cast<u32>((upperCons - delayValue) % srcRate);
                dep.firingStart_ = static_cast<u32>(math::floorDiv(lowerCons - delayValue, srcRate));
                dep.firingEnd_ = static_cast<u32>(math::floorDiv(upperCons - delayValue, srcRate));
                return dep;
            }

            DependencyInfo createConsDependency(const Edge *edge,
                                                int64_t lowerProd,
                                                int64_t upperProd,
                                                int64_t snkRate,
                                                int64_t delayValue,
                                                const srless::GraphFiring *handler) {
                if (!snkRate) {
                    return { nullptr, nullptr, 0, 0, 0, 0, 0, 0 };
                }
                DependencyInfo dep{ };
                dep.vertex_ = edge->sink();
                dep.handler_ = handler;
                dep.rate_ = snkRate;
                dep.edgeIx_ = static_cast<u32>(edge->sinkPortIx());
                dep.memoryStart_ = static_cast<u32>((lowerProd + delayValue) % snkRate);
                dep.memoryEnd_ = static_cast<u32>((upperProd + delayValue) % snkRate);
                dep.firingStart_ = static_cast<u32>(math::floorDiv(lowerProd + delayValue, snkRate));
                dep.firingEnd_ = static_cast<u32>(math::floorDiv(upperProd + delayValue, snkRate));
                return dep;
            }
        }
    }
}

/* === Function(s) definition === */

void spider::pisdf::detail::computeExecDependency(const Edge *edge,
                                                  int64_t lowerCons,
                                                  int64_t upperCons,
                                                  const srless::GraphFiring *handler,
                                                  spider::vector<DependencyInfo> &result) {
    /* == Handle specific cases == */
    const auto *source = edge->source();
    const auto sourceType = source->subtype();
    const auto srcRate = handler->getSourceRate(edge);
    const auto *delay = edge->delay();
    const auto delayValue = delay ? delay->value() : 0;

    if (sourceType == VertexType::DELAY) {
        /* == Case of getter vertex == */
        const auto *delayFromVertex = source->convertTo<pisdf::DelayVertex>()->delay();
        const auto *delayEdge = delayFromVertex->edge();
        const auto *sink = delayEdge->sink();
        const auto snkRate = handler->getSinkRate(delayEdge);
        const auto snkRV = handler->getRV(sink);
        const auto srcRV = handler->getRV(delayEdge->source());
        const auto offset = sink->subtype() == VertexType::OUTPUT ? srcRate * srcRV - snkRate : snkRate * snkRV;
        computeExecDependency(delayEdge, lowerCons + offset, upperCons + offset, handler, result);
    } else if (lowerCons >= delayValue) {
        /* == source only == */
        if (sourceType == VertexType::INPUT) {
            auto dep = createExecDependency(edge, lowerCons, upperCons, srcRate, delayValue, handler);
            const auto *gh = handler->getParent()->handler();
            spider::reserve(result, dep.firingEnd_ - dep.firingStart_ + 1);
            const auto upperLCons = srcRate * handler->firingValue();
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto start = k == dep.firingStart_ ? (lowerCons - delayValue) % srcRate : 0;
                const auto end = k == dep.firingEnd_ ? (upperCons - delayValue) % srcRate : srcRate - 1;
                const auto *upperEdge = source->graph()->inputEdge(source->ix());
                computeExecDependency(upperEdge, upperLCons + start, upperLCons + end, gh, result);
            }
            return;
        } else if (sourceType == VertexType::GRAPH) {
            auto dep = createExecDependency(edge, lowerCons, upperCons, srcRate, delayValue, handler);
            spider::reserve(result, dep.firingEnd_ - dep.firingStart_ + 1);
            const auto *graph = source->convertTo<pisdf::Graph>();
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto *gh = handler->getSubgraphGraphFiring(graph, k);
                if (!gh->isResolved()) {
                    result.emplace_back(unresolved);
                } else {
                    const auto *innerEdge = graph->outputInterface(edge->sourcePortIx())->edge();
                    const auto ifSrcRV = gh->getRV(innerEdge->source());
                    const auto ifSrcRate = gh->getSourceRate(innerEdge);
                    const auto ifDelay = innerEdge->delay() ? innerEdge->delay()->value() : 0u;
                    const auto start = k == dep.firingStart_ ? (lowerCons - delayValue) % srcRate : 0;
                    const auto end = k == dep.firingEnd_ ? (upperCons - delayValue) % srcRate : srcRate - 1;
                    const auto lCons = (ifSrcRV * ifSrcRate - srcRate) + start + ifDelay;
                    const auto uCons = (ifSrcRV * ifSrcRate - srcRate) + end + ifDelay;
                    computeExecDependency(innerEdge, lCons, uCons, gh, result);
                }
            }
            return;
        }
        spider::reserve(result, 1u);
        result.emplace_back(createExecDependency(edge, lowerCons, upperCons, srcRate, delayValue, handler));
    } else if (delay && (upperCons < delayValue)) {
        /* == setter only == */
        const auto *setterEdge = delay->setter()->outputEdge(delay->setterPortIx());
        computeExecDependency(setterEdge, lowerCons, upperCons, handler, result);
    } else if (delay) {
        /* == setter + source == */
        const auto *setterEdge = delay->setter()->outputEdge(delay->setterPortIx());
        computeExecDependency(setterEdge, lowerCons, delayValue - 1, handler, result);
        computeExecDependency(edge, delayValue, upperCons, handler, result);
    } else {
        throwNullptrException();
    }
}


void spider::pisdf::detail::computeConsDependency(const Edge *edge,
                                                  int64_t lowerProd,
                                                  int64_t upperProd,
                                                  const srless::GraphFiring *handler,
                                                  spider::vector<DependencyInfo> &result) {
    /* == Precompute some numerical values == */
    const auto *sink = edge->sink();
    const auto sinkType = sink->subtype();
    const auto snkRate = handler->getSinkRate(edge);
    const auto srcRate = handler->getSourceRate(edge);
    const auto srcRV = handler->getRV(edge->source());
    const auto snkRV = handler->getRV(sink);
    const auto *delay = edge->delay();
    const auto delayValue = delay ? delay->value() : 0;
    const auto totalRate = sinkType == VertexType::OUTPUT ? srcRate * srcRV + delayValue : snkRate * snkRV;
    const auto delayedTotalRate = sinkType == VertexType::OUTPUT ? totalRate : totalRate - delayValue;

    /* == Handle specific cases == */
    if (sinkType == VertexType::DELAY) {
        /* == Case of setter vertex == */
        const auto *nextDelay = sink->convertTo<pisdf::DelayVertex>()->delay();
        const auto offset = nextDelay->value() - delayValue;
        computeConsDependency(nextDelay->edge(), lowerProd - offset, upperProd - offset, handler, result);
    } else if (delay && (lowerProd >= delayedTotalRate)) {
        /* == getter only == */
        const auto *getterEdge = delay->getter()->inputEdge(delay->getterPortIx());
        computeConsDependency(getterEdge, lowerProd - delayedTotalRate, upperProd - delayedTotalRate, handler, result);
    } else if (upperProd < delayedTotalRate) {
        /* == sink only == */
        if (sinkType == VertexType::OUTPUT) {
            auto dep = createConsDependency(edge, lowerProd, upperProd, totalRate, delayValue, handler);
            /* == Now check where we fall == */
            const auto minValidMemWDelay = srcRate * srcRV - snkRate;
            const auto minValidMemWODelay = minValidMemWDelay + delayValue;
            const auto parentLProd = snkRate * handler->firingValue();
            const auto *upperEdge = sink->graph()->outputEdge(sink->ix());
            const auto *gh = handler->getParent()->handler();
            if (dep.memoryEnd_ < minValidMemWDelay) {
                /* == void dependency == */
                return;
            } else if ((dep.memoryStart_ >= minValidMemWODelay) ||
                       (!delayValue && (dep.memoryEnd_ >= minValidMemWODelay))) {
                /* == forward dependency == */
                lowerProd = parentLProd + std::max(int64_t{ 0 }, int64_t{ dep.memoryStart_ - minValidMemWODelay });
                upperProd = parentLProd + (dep.memoryEnd_ - minValidMemWODelay);
                computeConsDependency(upperEdge, lowerProd, upperProd, gh, result);
                return;
            } else if (delay && dep.memoryEnd_ < minValidMemWODelay) {
                /* == getter only == */
                lowerProd = std::max(int64_t{ 0 }, int64_t{ dep.memoryStart_ - minValidMemWDelay });
                upperProd = dep.memoryEnd_ - minValidMemWDelay;
                const auto *getterEdge = delay->getter()->inputEdge(delay->getterPortIx());
                computeConsDependency(getterEdge, lowerProd, upperProd, handler, result);
                return;
            } else if (delay) {
                /* == mix of getter and interface == */
                const auto *getterEdge = delay->getter()->inputEdge(delay->getterPortIx());
                const auto getterLowerProd = dep.memoryStart_ - minValidMemWDelay;
                /* == Getter dependencies, same level as current actor == */
                computeConsDependency(getterEdge, getterLowerProd, delayValue - 1, handler, result);
                /* == Sink dependencies, one level up of the one of current actor == */
                lowerProd = parentLProd + std::max(int64_t{ 0 }, int64_t{ dep.memoryStart_ - minValidMemWODelay });
                upperProd = parentLProd + (dep.memoryEnd_ - minValidMemWODelay);
                computeConsDependency(upperEdge, lowerProd, upperProd, gh, result);
                return;
            } else {
                throwSpiderException("unexpected behavior.");
            }
        } else if (sinkType == VertexType::GRAPH) {
            auto dep = createConsDependency(edge, lowerProd, upperProd, snkRate, delayValue, handler);
            spider::reserve(result, dep.firingEnd_ - dep.firingStart_ + 1);
            const auto *graph = sink->convertTo<pisdf::Graph>();
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto *gh = handler->getSubgraphGraphFiring(graph, k);
                if (!gh->isResolved()) {
                    result.emplace_back(unresolved);
                } else {
                    const auto *interface = graph->inputInterface(edge->sinkPortIx());
                    const auto *innerEdge = interface->edge();
                    const auto ifSrcRate = gh->getSourceRate(innerEdge);
                    const auto ifSnkRV = gh->getRV(innerEdge->sink());
                    const auto ifSnkRate = gh->getSinkRate(innerEdge);
                    const auto adjustedSnkRate = ifSnkRate * ifSnkRV;
                    const auto fullRepCount = adjustedSnkRate / ifSrcRate;
                    const auto lProd = k == dep.firingStart_ ? dep.memoryStart_ % ifSrcRate : 0;
                    const auto uProd = k == dep.firingEnd_ ? dep.memoryEnd_ % ifSrcRate : ifSrcRate - 1;
                    spider::reserve(result, static_cast<size_t>(fullRepCount));
                    for (auto i = 0; i < fullRepCount; ++i) {
                        const auto offset = i * snkRate;
                        computeConsDependency(innerEdge, lProd + offset, uProd + offset, gh, result);
                    }
                    if ((ifSrcRate * fullRepCount) != adjustedSnkRate) {
                        const auto lp = lProd + fullRepCount * snkRate;
                        if (lp < adjustedSnkRate) {
                            const auto up = std::min(uProd + fullRepCount * snkRate, adjustedSnkRate - 1);
                            computeConsDependency(innerEdge, lp, up, gh, result);
                        }
                    }
                }
            }
            return;
        }
        spider::reserve(result, 1u);
        result.emplace_back(createConsDependency(edge, lowerProd, upperProd, snkRate, delayValue, handler));
    } else if (delay) {
        /* == sink + getter == */
        const auto *getterEdge = delay->getter()->inputEdge(delay->getterPortIx());
        computeConsDependency(edge, lowerProd, totalRate - delayValue - 1, handler, result);
        computeConsDependency(getterEdge, 0, upperProd - delayedTotalRate, handler, result);
    } else {
        throwSpiderException("unexpected behavior.");
    }
}
