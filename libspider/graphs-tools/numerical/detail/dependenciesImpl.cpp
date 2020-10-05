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
#include <graphs-tools/helper/pisdf-helper.h>

/* === Static variable(s) === */

namespace spider {
    namespace pisdf {
        static ExecDependencyInfo unresolved = { nullptr, nullptr, -1,
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

/* === Execution dependencies === */

spider::pisdf::DependencyIterator spider::pisdf::detail::computeExecDependencyImpl(const Edge *edge,
                                                                                   int64_t lowerCons,
                                                                                   int64_t upperCons,
                                                                                   int64_t srcRate,
                                                                                   const srless::FiringHandler *handler) {
    /* == Compute numerical values needed for the dependency computation == */
    const auto *delay = edge->delay();
    const auto delayValue = delay ? delay->value() : 0;

    /* == Actual dependency computation == */
    if (lowerCons >= delayValue) {
        /* == source only == */
        return DependencyIterator{ createExecDependency(edge, lowerCons, upperCons, srcRate, delayValue, handler) };
    } else if (delay && (upperCons < delayValue)) {
        /* == setter only == */
        edge = delay->setter()->outputEdge(delay->setterPortIx());
        return computeExecDependency(edge, lowerCons, upperCons, handler);
    } else if (delay) {
        /* == setter + source == */
        const auto *setterEdge = delay->setter()->outputEdge(delay->setterPortIx());
        const auto setDeps = computeExecDependency(setterEdge, lowerCons, delayValue - 1, handler);
        const auto srcDeps = computeExecDependency(edge, delayValue, upperCons, handler);
        const auto nSetDeps = setDeps.count();
        const auto nSrcDeps = srcDeps.count();
        if (nSetDeps + nSrcDeps > 2) {
            auto dependencies = factory::vector<ExecDependencyInfo>(StackID::TRANSFO);
            dependencies.reserve(static_cast<size_t>(nSetDeps + nSrcDeps));
            std::move(std::begin(setDeps), std::end(setDeps), std::back_inserter(dependencies));
            std::move(std::begin(srcDeps), std::end(srcDeps), std::back_inserter(dependencies));
            return DependencyIterator{ MultipleDependency{ std::move(dependencies) }};
        } else {
            return DependencyIterator{ DualDependency{{ *(setDeps.begin()), *(srcDeps.begin()) }}};
        }
    } else {
        throwSpiderException("unexpected behavior.");
    }
}

spider::pisdf::DependencyIterator spider::pisdf::detail::computeInputExecDependency(const Edge *edge,
                                                                                    int64_t lowerCons,
                                                                                    int64_t upperCons,
                                                                                    const srless::FiringHandler *handler) {
    const auto *source = edge->source();
    const auto *delay = edge->delay();
    const auto delayValue = delay ? delay->value() : 0;
    const auto srcRate = edge->sourceRateExpression().evaluate(handler->getParams());
    auto deps = computeExecDependencyImpl(edge, lowerCons, upperCons, srcRate, handler);
    auto result = factory::vector<ExecDependencyInfo>(StackID::TRANSFO);
    result.reserve(static_cast<size_t>(deps.count()));
    for (auto &dep : deps) {
        const auto isInterface = dep.vertex_ == source;
        if (isInterface) {
            const auto parentLowerCons = srcRate * handler->firingValue();
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto start = k == dep.firingStart_ ? (lowerCons - delayValue) % srcRate : 0;
                const auto end = k == dep.firingEnd_ ? (upperCons - delayValue) % srcRate : srcRate - 1;
                const auto *gh = handler->getParent()->handler();
                const auto *upperEdge = source->graph()->inputEdge(source->ix());
                auto ifDeps = computeExecDependency(upperEdge, parentLowerCons + start, parentLowerCons + end, gh);
                std::move(std::begin(ifDeps), std::end(ifDeps), std::back_inserter(result));
            }
        } else {
            result.emplace_back(dep);
        }
    }
    return DependencyIterator{ MultipleDependency{ std::move(result) }};
}

spider::pisdf::DependencyIterator spider::pisdf::detail::computeGraphExecDependency(const Edge *edge,
                                                                                    int64_t lowerCons,
                                                                                    int64_t upperCons,
                                                                                    const srless::FiringHandler *handler) {
    const auto *source = edge->source();
    const auto *delay = edge->delay();
    const auto delayValue = delay ? delay->value() : 0;
    const auto srcRate = edge->sourceRateExpression().evaluate(handler->getParams());
    const auto graphDeps = computeExecDependencyImpl(edge, lowerCons, upperCons, srcRate, handler);
    auto result = factory::vector<ExecDependencyInfo>(StackID::TRANSFO);
    for (auto &dep : graphDeps) {
        if (dep.vertex_ == source) {
            const auto *graph = source->convertTo<pisdf::Graph>();
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto *gh = handler->getChildFiring(graph, k);
                if (!gh->isResolved()) {
                    result.push_back(unresolved);
                } else {
                    edge = graph->outputInterface(edge->sourcePortIx())->edge();
                    const auto ifSrcRV = gh->getRV(edge->source());
                    const auto ifSrcRate = edge->sourceRateExpression().evaluate(gh->getParams());
                    const auto ifDelay = edge->delay() ? edge->delay()->value() : 0u;
                    const auto start = k == dep.firingStart_ ? (lowerCons - delayValue) % srcRate : 0;
                    const auto end = k == dep.firingEnd_ ? (upperCons - delayValue) % srcRate : srcRate - 1;
                    const auto lCons = (ifSrcRV * ifSrcRate - srcRate) + start % srcRate + ifDelay;
                    const auto uCons = (ifSrcRV * ifSrcRate - srcRate) + end % srcRate + ifDelay;
                    const auto deps = computeExecDependency(edge, lCons, uCons, gh);
                    std::move(std::begin(deps), std::end(deps), std::back_inserter(result));
                }
            }
        } else {
            result.emplace_back(dep);
        }
    }
    return DependencyIterator{ MultipleDependency{ std::move(result) }};
}

spider::pisdf::DependencyIterator spider::pisdf::detail::computeExecDependency(const Edge *edge,
                                                                               int64_t lowerCons,
                                                                               int64_t upperCons,
                                                                               const srless::FiringHandler *handler) {
    /* == Handle specific cases == */
    const auto sourceType = edge->source()->subtype();
    if (sourceType == VertexType::INPUT) {
        return computeInputExecDependency(edge, lowerCons, upperCons, handler);
    } else if (sourceType == VertexType::DELAY) {
        /* == Case of getter vertex == */
        const auto *delayFromVertex = edge->source()->convertTo<pisdf::DelayVertex>()->delay();
        edge = delayFromVertex->edge();
        const auto snkRate = edge->sinkRateExpression().evaluate(handler->getParams());
        const auto offset = snkRate * handler->getRV(edge->sink());
        lowerCons += offset;
        upperCons += offset;
        return computeExecDependency(edge, lowerCons, upperCons, handler);
    } else if (sourceType == VertexType::GRAPH) {
        return computeGraphExecDependency(edge, lowerCons, upperCons, handler);
    } else {
        const auto srcRate = edge->sourceRateExpression().evaluate(handler->getParams());
        return computeExecDependencyImpl(edge, lowerCons, upperCons, srcRate, handler);
    }
}

/* === Consummer dependencies === */

spider::pisdf::DependencyIterator spider::pisdf::detail::computeConsDependencyImpl(const Edge *edge,
                                                                                   int64_t lowerProd,
                                                                                   int64_t upperProd,
                                                                                   int64_t snkRate,
                                                                                   const srless::FiringHandler *handler) {
    /* == Actual computation of dependencies == */
    const auto *delay = edge->delay();
    const auto delayValue = delay ? delay->value() : 0;
    const auto snkTotRate = edge->sinkRateExpression().evaluate(handler->getParams()) * handler->getRV(edge->sink());
    const auto delayedSnkRate = snkTotRate - delayValue;
    if (delay && (lowerProd >= delayedSnkRate)) {
        /* == getter only == */
        edge = delay->getter()->inputEdge(delay->getterPortIx());
        lowerProd -= delayedSnkRate;
        upperProd -= delayedSnkRate;
        return computeConsDependency(edge, lowerProd, upperProd, handler);
    } else if (upperProd < delayedSnkRate) {
        /* == sink only == */
        return DependencyIterator{ createConsDependency(edge, lowerProd, upperProd, snkRate, delayValue, handler) };
    } else if (delay) {
        /* == sink + getter == */
        const auto *getterEdge = delay->getter()->inputEdge(delay->getterPortIx());
        const auto snkDeps = computeConsDependency(edge, lowerProd, snkTotRate - delayValue - 1, handler);
        const auto getDeps = computeConsDependency(getterEdge, 0, upperProd - delayedSnkRate, handler);
        const auto nSnkDeps = snkDeps.count();
        const auto nGetDeps = getDeps.count();
        if (nGetDeps + nSnkDeps > 2) {
            auto dependencies = factory::vector<ExecDependencyInfo>(StackID::TRANSFO);
            dependencies.reserve(static_cast<size_t>(nGetDeps + nSnkDeps));
            std::move(std::begin(snkDeps), std::end(snkDeps), std::back_inserter(dependencies));
            std::move(std::begin(getDeps), std::end(getDeps), std::back_inserter(dependencies));
            return DependencyIterator{ MultipleDependency{ std::move(dependencies) }};
        } else {
            return DependencyIterator{ DualDependency{{ *(snkDeps.begin()), *(getDeps.begin()) }}};
        }
    } else {
        throwSpiderException("unexpected behavior.");
    }
}


spider::pisdf::DependencyIterator spider::pisdf::detail::computeGraphConsDependency(const Edge *edge,
                                                                                    int64_t lowerProd,
                                                                                    int64_t upperProd,
                                                                                    const srless::FiringHandler *handler) {
    const auto *sink = edge->sink();
    const auto snkRate = edge->sinkRateExpression().evaluate(handler->getParams());
    const auto graphDeps = computeConsDependencyImpl(edge, lowerProd, upperProd, snkRate, handler);
    auto result = factory::vector<ExecDependencyInfo>(StackID::TRANSFO);
    for (auto &dep : graphDeps) {
        if (dep.vertex_ == sink) {
            const auto *graph = sink->convertTo<pisdf::Graph>();
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto *gh = handler->getChildFiring(graph, k);
                if (!gh->isResolved()) {
                    result.push_back(unresolved);
                } else {
                    const auto *interface = graph->inputInterface(edge->sinkPortIx());
                    edge = interface->edge();
                    const auto ifSrcRate = edge->sourceRateExpression().evaluate(handler->getParams());
                    const auto ifSnkRV = gh->getRV(edge->sink());
                    const auto ifSnkRate = edge->sinkRateExpression().evaluate(handler->getParams());
                    const auto adjustedSnkRate = ifSnkRate * ifSnkRV;
                    const auto fullRepCount = adjustedSnkRate / ifSrcRate;
                    const auto updatedLowerProd = k == dep.firingStart_ ? dep.memoryStart_ : 0;
                    const auto updatedUpperProd = k == dep.firingEnd_ ? dep.memoryEnd_ : ifSrcRate - 1;
                    for (auto i = 0; i < fullRepCount; ++i) {
                        const auto lp = updatedLowerProd + i * snkRate;
                        const auto up = updatedUpperProd + i * snkRate;
                        const auto deps = computeConsDependency(edge, lp, up, gh);
                        std::move(std::begin(deps), std::end(deps), std::back_inserter(result));
                    }
                    if ((ifSrcRate * fullRepCount) != adjustedSnkRate) {
                        const auto lp = updatedLowerProd + fullRepCount * snkRate;
                        if (lp < adjustedSnkRate) {
                            const auto up = std::min(updatedUpperProd + fullRepCount * snkRate, adjustedSnkRate - 1);
                            const auto deps = computeConsDependency(edge, lp, up, gh);
                            std::move(std::begin(deps), std::end(deps), std::back_inserter(result));
                        }
                    }
                    lowerProd -= ifSrcRate;
                    upperProd -= ifSrcRate;
                }
            }
        } else {
            result.emplace_back(dep);
        }
    }
    return DependencyIterator{ MultipleDependency{ std::move(result) }};
}

spider::pisdf::DependencyIterator spider::pisdf::detail::computeConsDependency(const Edge *edge,
                                                                               int64_t lowerProd,
                                                                               int64_t upperProd,
                                                                               const srless::FiringHandler *handler) {
    /* == Handle specific cases == */
    const auto *sink = edge->sink();
    const auto sinkType = sink->subtype();
    if (sinkType == VertexType::OUTPUT) {
        const auto snkRate = edge->sinkRateExpression().evaluate(handler->getParams());
        const auto srcRate = edge->sourceRateExpression().evaluate(handler->getParams());
        const auto srcRV = handler->getRV(edge->source());
        edge = sink->graph()->outputEdge(sink->ix());
        handler = handler->getParent()->handler();
        if ((srcRate * srcRV) == snkRate) {
            return computeConsDependency(edge, lowerProd, upperProd, handler);
        } else if (lowerProd >= (srcRate * srcRV - snkRate)) {
            return computeConsDependency(edge, lowerProd % snkRate, upperProd % snkRate, handler);
        } else if (upperProd >= (srcRate * srcRV - snkRate)) {
            return computeConsDependency(edge, 0, upperProd % snkRate, handler);
        }
        return DependencyIterator{ VoidDependency{ }};
    } else if (sinkType == VertexType::DELAY) {
        /* == Case of setter vertex == */
        const auto *delay = edge->sink()->convertTo<pisdf::DelayVertex>()->delay();
        const auto currentDelayValue = edge->delay() ? edge->delay()->value() : 0;
        const auto delayValue = delay->value() - currentDelayValue;
        edge = delay->edge();
        return computeConsDependency(edge, lowerProd - delayValue, upperProd - delayValue, handler);
    } else if (sinkType == VertexType::GRAPH) {
        return computeGraphConsDependency(edge, lowerProd, upperProd, handler);
    } else {
        const auto snkRate = edge->sinkRateExpression().evaluate(handler->getParams());
        return computeConsDependencyImpl(edge, lowerProd, upperProd, snkRate, handler);
    }
}
