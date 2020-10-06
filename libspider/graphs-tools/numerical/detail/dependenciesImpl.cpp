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
#include <graphs-tools/transformation/srless/FiringHandler.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/DelayVertex.h>

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
            UniqueDependency createExecDependency(const Edge *edge,
                                                  int64_t lowerCons,
                                                  int64_t upperCons,
                                                  int64_t srcRate,
                                                  int64_t delayValue,
                                                  const srless::FiringHandler *handler) {
                UniqueDependency dep{ };
                dep.info_.vertex_ = edge->source();
                dep.info_.handler_ = handler;
                dep.info_.rate_ = srcRate;
                dep.info_.edgeIx_ = static_cast<u32>(edge->sourcePortIx());
                dep.info_.memoryStart_ = static_cast<u32>((lowerCons - delayValue) % srcRate);
                dep.info_.memoryEnd_ = static_cast<u32>((upperCons - delayValue) % srcRate);
                dep.info_.firingStart_ = static_cast<u32>(math::floorDiv(lowerCons - delayValue, srcRate));
                dep.info_.firingEnd_ = static_cast<u32>(math::floorDiv(upperCons - delayValue, srcRate));
                return dep;
            }

            UniqueDependency createConsDependency(const Edge *edge,
                                                  int64_t lowerProd,
                                                  int64_t upperProd,
                                                  int64_t snkRate,
                                                  int64_t delayValue,
                                                  const srless::FiringHandler *handler) {
                UniqueDependency dep{ };
                dep.info_.vertex_ = edge->sink();
                dep.info_.handler_ = handler;
                dep.info_.rate_ = snkRate;
                dep.info_.edgeIx_ = static_cast<u32>(edge->sinkPortIx());
                dep.info_.memoryStart_ = static_cast<u32>((lowerProd + delayValue) % snkRate);
                dep.info_.memoryEnd_ = static_cast<u32>((upperProd + delayValue) % snkRate);
                dep.info_.firingStart_ = static_cast<u32>(math::floorDiv(lowerProd + delayValue, snkRate));
                dep.info_.firingEnd_ = static_cast<u32>(math::floorDiv(upperProd + delayValue, snkRate));
                return dep;
            }
        }
    }
}

/* === Function(s) definition === */

spider::pisdf::DependencyIterator spider::pisdf::detail::computeExecDependency(const Edge *edge,
                                                                               int64_t lowerCons,
                                                                               int64_t upperCons,
                                                                               const srless::FiringHandler *handler) {
    /* == Handle specific cases == */
    const auto sourceType = edge->source()->subtype();
    const auto srcRate = edge->sourceRateExpression().evaluate(handler->getParams());
    const auto *delay = edge->delay();
    const auto delayValue = delay ? delay->value() : 0;
    if (sourceType == VertexType::DELAY) {
        /* == Case of getter vertex == */
        const auto *delayFromVertex = edge->source()->convertTo<pisdf::DelayVertex>()->delay();
        const auto *delayEdge = delayFromVertex->edge();
        const auto *sink = delayEdge->sink();
        const auto snkRate = delayEdge->sinkRateExpression().evaluate(handler->getParams());
        const auto snkRV = handler->getRV(sink);
        const auto srcRV = handler->getRV(delayEdge->source());
        const auto offset = sink->subtype() == VertexType::OUTPUT ? srcRate * srcRV - snkRate : snkRate * snkRV;
        return computeExecDependency(delayEdge, lowerCons + offset, upperCons + offset, handler);
    } else if (lowerCons >= delayValue) {
        /* == source only == */
        if (sourceType == VertexType::INPUT) {
            auto dep = createExecDependency(edge, lowerCons, upperCons, srcRate, delayValue, handler).info_;
            auto result = factory::vector<DependencyInfo>(StackID::TRANSFO);
            result.reserve(dep.firingEnd_ - dep.firingStart_ + 1);
            const auto upperLCons = srcRate * handler->firingValue();
            const auto *gh = handler->getParent()->handler();
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto start = k == dep.firingStart_ ? (lowerCons - delayValue) % srcRate : 0;
                const auto end = k == dep.firingEnd_ ? (upperCons - delayValue) % srcRate : srcRate - 1;
                const auto *upperEdge = edge->source()->graph()->inputEdge(edge->source()->ix());
                auto deps = computeExecDependency(upperEdge, upperLCons + start, upperLCons + end, gh);
                std::move(std::begin(deps), std::end(deps), std::back_inserter(result));
            }
            return DependencyIterator{ MultipleDependency{ std::move(result) }};
        } else if (sourceType == VertexType::GRAPH) {
            auto dep = createExecDependency(edge, lowerCons, upperCons, srcRate, delayValue, handler).info_;
            auto result = factory::vector<DependencyInfo>(StackID::TRANSFO);
            result.reserve(dep.firingEnd_ - dep.firingStart_ + 1);
            const auto *graph = edge->source()->convertTo<pisdf::Graph>();
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto *gh = handler->getChildFiring(graph, dep.firingStart_);
                if (!gh->isResolved()) {
                    result.push_back(unresolved);
                } else {
                    const auto *innerEdge = graph->outputInterface(edge->sourcePortIx())->edge();
                    const auto ifSrcRV = gh->getRV(innerEdge->source());
                    const auto ifSrcRate = innerEdge->sourceRateExpression().evaluate(gh->getParams());
                    const auto ifDelay = innerEdge->delay() ? innerEdge->delay()->value() : 0u;
                    const auto start = k == dep.firingStart_ ? (lowerCons - delayValue) % srcRate : 0;
                    const auto end = k == dep.firingEnd_ ? (upperCons - delayValue) % srcRate : srcRate - 1;
                    const auto lCons = (ifSrcRV * ifSrcRate - srcRate) + start + ifDelay;
                    const auto uCons = (ifSrcRV * ifSrcRate - srcRate) + end + ifDelay;
                    auto deps = computeExecDependency(innerEdge, lCons, uCons, gh);
                    std::move(std::begin(deps), std::end(deps), std::back_inserter(result));
                }
            }
            return DependencyIterator{ MultipleDependency{ std::move(result) }};
        }
        return DependencyIterator{ createExecDependency(edge, lowerCons, upperCons, srcRate, delayValue, handler) };
    } else if (delay && (upperCons < delayValue)) {
        /* == setter only == */
        const auto *setterEdge = delay->setter()->outputEdge(delay->setterPortIx());
        return computeExecDependency(setterEdge, lowerCons, upperCons, handler);
    } else if (delay) {
        /* == setter + source == */
        const auto *setterEdge = delay->setter()->outputEdge(delay->setterPortIx());
        const auto setDeps = computeExecDependency(setterEdge, lowerCons, delayValue - 1, handler);
        const auto srcDeps = computeExecDependency(edge, delayValue, upperCons, handler);
        auto result = factory::vector<DependencyInfo>(StackID::TRANSFO);
        result.reserve(static_cast<size_t>(setDeps.count() + srcDeps.count()));
        std::move(std::begin(setDeps), std::end(setDeps), std::back_inserter(result));
        std::move(std::begin(srcDeps), std::end(srcDeps), std::back_inserter(result));
        return DependencyIterator{ MultipleDependency{ std::move(result) }};
    } else {
        throwNullptrException();
    }
}

spider::pisdf::DependencyIterator spider::pisdf::detail::computeConsDependency(const Edge *edge,
                                                                               int64_t lowerProd,
                                                                               int64_t upperProd,
                                                                               const srless::FiringHandler *handler) {
    /* == Precompute some numerical values == */
    const auto sinkType = edge->sink()->subtype();
    const auto snkRate = edge->sinkRateExpression().evaluate(handler->getParams());
    const auto srcRate = edge->sourceRateExpression().evaluate(handler->getParams());
    const auto srcRV = handler->getRV(edge->source());
    const auto snkRV = handler->getRV(edge->sink());
    const auto *delay = edge->delay();
    const auto delayValue = delay ? delay->value() : 0;
    const auto totalRate = sinkType == VertexType::OUTPUT ? srcRate * srcRV + delayValue : snkRate * snkRV;
    const auto delayedTotalRate = sinkType == VertexType::OUTPUT ? totalRate : totalRate - delayValue;

    /* == Handle specific cases == */
    if (sinkType == VertexType::DELAY) {
        /* == Case of setter vertex == */
        const auto *nextDelay = edge->sink()->convertTo<pisdf::DelayVertex>()->delay();
        const auto offset = nextDelay->value() - delayValue;
        return computeConsDependency(nextDelay->edge(), lowerProd - offset, upperProd - offset, handler);
    } else if (delay && (lowerProd >= delayedTotalRate)) {
        /* == getter only == */
        const auto *getterEdge = delay->getter()->inputEdge(delay->getterPortIx());
        return computeConsDependency(getterEdge, lowerProd - delayedTotalRate, upperProd - delayedTotalRate, handler);
    } else if (upperProd < delayedTotalRate) {
        /* == sink only == */
        if (sinkType == VertexType::OUTPUT) {
            auto dep = createConsDependency(edge, lowerProd, upperProd, totalRate, delayValue, handler).info_;
            /* == Now check where we fall == */
            const auto minValidMemWDelay = srcRate * srcRV - snkRate;
            const auto minValidMemWODelay = minValidMemWDelay + delayValue;
            const auto parentLProd = snkRate * handler->firingValue();
            const auto *upperEdge = edge->sink()->graph()->outputEdge(edge->sink()->ix());
            const auto *gh = handler->getParent()->handler();
            if (dep.memoryEnd_ < minValidMemWDelay) {
                /* == void dependency == */
                return DependencyIterator{ VoidDependency{ }};
            } else if ((dep.memoryStart_ >= minValidMemWODelay) ||
                       (!delayValue && (dep.memoryEnd_ >= minValidMemWODelay))) {
                /* == forward dependency == */
                lowerProd = parentLProd + std::max(0l, dep.memoryStart_ - minValidMemWODelay);
                upperProd = parentLProd + (dep.memoryEnd_ - minValidMemWODelay);
                return computeConsDependency(upperEdge, lowerProd, upperProd, gh);
            } else if (delay && dep.memoryEnd_ < minValidMemWODelay) {
                /* == getter only == */
                lowerProd = std::max(0l, dep.memoryStart_ - minValidMemWDelay);
                upperProd = dep.memoryEnd_ - minValidMemWDelay;
                const auto *getterEdge = delay->getter()->inputEdge(delay->getterPortIx());
                return computeConsDependency(getterEdge, lowerProd, upperProd, handler);
            } else if (delay) {
                /* == mix of getter and interface == */
                const auto *getterEdge = delay->getter()->inputEdge(delay->getterPortIx());
                const auto getterLowerProd = dep.memoryStart_ - minValidMemWDelay;
                /* == Getter dependencies, same level as current actor == */
                const auto getDeps = computeConsDependency(getterEdge, getterLowerProd, delayValue - 1, handler);
                /* == Sink dependencies, one level up of the one of current actor == */
                lowerProd = parentLProd + std::max(0l, dep.memoryStart_ - minValidMemWODelay);
                upperProd = parentLProd + (dep.memoryEnd_ - minValidMemWODelay);
                const auto snkDeps = computeConsDependency(upperEdge, lowerProd, upperProd, gh);
                /* == Return a compound dependency iterator == */
                auto result = factory::vector<DependencyInfo>(StackID::TRANSFO);
                result.reserve(static_cast<size_t>(getDeps.count() + snkDeps.count()));
                std::move(std::begin(getDeps), std::end(getDeps), std::back_inserter(result));
                std::move(std::begin(snkDeps), std::end(snkDeps), std::back_inserter(result));
                return DependencyIterator{ MultipleDependency{ std::move(result) }};
            } else {
                throwSpiderException("unexpected behavior.");
            }
        } else if (sinkType == VertexType::GRAPH) {
            auto dep = createConsDependency(edge, lowerProd, upperProd, snkRate, delayValue, handler).info_;
            auto result = factory::vector<DependencyInfo>(StackID::TRANSFO);
            result.reserve(dep.firingEnd_ - dep.firingStart_ + 1);
            const auto *graph = edge->sink()->convertTo<pisdf::Graph>();
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto *gh = handler->getChildFiring(graph, k);
                if (!gh->isResolved()) {
                    result.emplace_back(unresolved);
                } else {
                    const auto *interface = graph->inputInterface(edge->sinkPortIx());
                    const auto *innerEdge = interface->edge();
                    const auto ifSrcRate = innerEdge->sourceRateExpression().evaluate(handler->getParams());
                    const auto ifSnkRV = gh->getRV(innerEdge->sink());
                    const auto ifSnkRate = innerEdge->sinkRateExpression().evaluate(handler->getParams());
                    const auto adjustedSnkRate = ifSnkRate * ifSnkRV;
                    const auto fullRepCount = adjustedSnkRate / ifSrcRate;
                    const auto lProd = k == dep.firingStart_ ? dep.memoryStart_ % ifSrcRate : 0;
                    const auto uProd = k == dep.firingEnd_ ? dep.memoryEnd_ % ifSrcRate : ifSrcRate - 1;
                    for (auto i = 0; i < fullRepCount; ++i) {
                        const auto offset = i * snkRate;
                        const auto deps = computeConsDependency(innerEdge, lProd + offset, uProd + offset, gh);
                        std::move(std::begin(deps), std::end(deps), std::back_inserter(result));
                    }
                    if ((ifSrcRate * fullRepCount) != adjustedSnkRate) {
                        const auto lp = lProd + fullRepCount * snkRate;
                        if (lp < adjustedSnkRate) {
                            const auto up = std::min(uProd + fullRepCount * snkRate, adjustedSnkRate - 1);
                            const auto deps = computeConsDependency(innerEdge, lp, up, gh);
                            std::move(std::begin(deps), std::end(deps), std::back_inserter(result));
                        }
                    }
                }
            }
            return DependencyIterator{ MultipleDependency{ std::move(result) }};
        }
        return DependencyIterator{ createConsDependency(edge, lowerProd, upperProd, snkRate, delayValue, handler) };
    } else if (delay) {
        /* == sink + getter == */
        const auto *getterEdge = delay->getter()->inputEdge(delay->getterPortIx());
        const auto snkDeps = computeConsDependency(edge, lowerProd, totalRate - delayValue - 1, handler);
        const auto getDeps = computeConsDependency(getterEdge, 0, upperProd - delayedTotalRate, handler);
        auto result = factory::vector<DependencyInfo>(StackID::TRANSFO);
        result.reserve(static_cast<size_t>(getDeps.count() + snkDeps.count()));
        std::move(std::begin(snkDeps), std::end(snkDeps), std::back_inserter(result));
        std::move(std::begin(getDeps), std::end(getDeps), std::back_inserter(result));
        return DependencyIterator{ MultipleDependency{ std::move(result) }};
    } else {
        throwSpiderException("unexpected behavior.");
    }
}
