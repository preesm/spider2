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

/* === Function(s) definition === */

int64_t spider::pisdf::detail::computeSrcRate(const Edge *edge, const srless::FiringHandler *handler) {
    if (edge->source()->subtype() == pisdf::VertexType::INPUT) {
        return edge->sinkRateExpression().evaluate(handler->getParams()) * handler->getRV(edge->sink());
    }
    return edge->sourceRateExpression().evaluate(handler->getParams());
}

spider::pisdf::DependencyIterator
spider::pisdf::detail::computeExecDependencyImpl(const Edge *edge,
                                                 int64_t lowerCons,
                                                 int64_t upperCons,
                                                 const srless::FiringHandler *handler) {
    if (edge->source()->subtype() == VertexType::DELAY) {
        /* == Case of getter vertex == */
        const auto *delay = edge->source()->convertTo<pisdf::DelayVertex>()->delay();
        edge = delay->edge();
        const auto offset = edge->sinkRateExpression().evaluate(handler->getParams()) * handler->getRV(edge->sink());
        lowerCons += offset;
        upperCons += offset;
    }
    const auto *delay = edge->delay();
    const auto srcRate = computeSrcRate(edge, handler);
    const auto setRate = delay ? delay->setterRate(handler->getParams()) : 0;
    const auto delayValue = delay ? delay->value() : 0;
    if (lowerCons >= delayValue) {
        /* == source only == */
        UniqueDependency dep{ };
        dep.info_.vertex_ = edge->source();
        dep.info_.handler_ = handler;
        dep.info_.rate_ = srcRate;
        dep.info_.edgeIx_ = static_cast<u32>(edge->sourcePortIx());
        dep.info_.memoryStart_ = static_cast<u32>((lowerCons - delayValue) % srcRate);
        dep.info_.memoryEnd_ = static_cast<u32>((upperCons - delayValue - 1) % srcRate);
        dep.info_.firingStart_ = static_cast<u32>(math::floorDiv(lowerCons - delayValue, srcRate));
        dep.info_.firingEnd_ = static_cast<u32>(math::floorDiv(upperCons - delayValue - 1, srcRate));
        return DependencyIterator{ dep };
    } else if (upperCons <= delayValue) {
#ifndef NDEBUG
        if (!delay) {
            throwNullptrException();
        }
#endif
        /* == setter only == */
        UniqueDependency dep{ };
        dep.info_.vertex_ = delay->setter();
        dep.info_.handler_ = handler;
        dep.info_.rate_ = setRate;
        dep.info_.edgeIx_ = static_cast<u32>(delay->setterPortIx());
        dep.info_.memoryStart_ = static_cast<u32>(lowerCons % setRate);
        dep.info_.memoryEnd_ = static_cast<u32>((upperCons - 1) % setRate);
        dep.info_.firingStart_ = static_cast<u32>(math::floorDiv(lowerCons, setRate));
        dep.info_.firingEnd_ = static_cast<u32>(math::floorDiv(upperCons - 1, setRate));
        return DependencyIterator{ dep };
    } else {
#ifndef NDEBUG
        if (!delay) {
            throwNullptrException();
        }
#endif
        /* == setter + source only == */
        DualDependency dep{ };
        /* == set info == */
        dep.infos_[0].vertex_ = delay->setter();
        dep.infos_[0].handler_ = handler;
        dep.infos_[0].rate_ = setRate;
        dep.infos_[0].edgeIx_ = static_cast<u32>(delay->setterPortIx());
        dep.infos_[0].memoryStart_ = static_cast<u32>(lowerCons % setRate);
        dep.infos_[0].memoryEnd_ = static_cast<u32>(setRate - 1);
        dep.infos_[0].firingStart_ = 0u;
        dep.infos_[0].firingEnd_ = handler->getRV(delay->setter()) - 1;
        /* == src info == */
        dep.infos_[1].vertex_ = edge->source();
        dep.infos_[1].handler_ = handler;
        dep.infos_[1].rate_ = srcRate;
        dep.infos_[1].edgeIx_ = static_cast<u32>(edge->sourcePortIx());
        dep.infos_[1].memoryStart_ = 0u;
        dep.infos_[1].memoryEnd_ = static_cast<u32>((upperCons - delayValue - 1) % srcRate);
        dep.infos_[1].firingStart_ = 0u;
        dep.infos_[1].firingEnd_ = static_cast<u32>(math::floorDiv(upperCons - delayValue - 1, srcRate));
        return DependencyIterator{ dep };
    }
}

spider::pisdf::DependencyIterator
spider::pisdf::detail::computeRelMidExecDependency(const Edge *edge, const srless::FiringHandler *handler) {
    const auto *source = edge->source();
    const auto snkRate = edge->sinkRateExpression().evaluate(handler->getParams());
    const auto srcRate = edge->sourceRateExpression().evaluate(handler->getParams());
    const auto srcRV = handler->getRV(source);
    const auto depMin = static_cast<u32>(srcRV - math::ceilDiv(snkRate, srcRate));
    const auto depMax = static_cast<u32>(srcRV - math::ceilDiv(snkRate, srcRate));
    const auto srcPortIx = static_cast<u32>(edge->sourcePortIx());
    const auto memStart = static_cast<u32>((srcRate * srcRV - snkRate) % srcRate);
    const auto memEnd = static_cast<u32>(srcRate - 1);
    if (!source->hierarchical()) {
        UniqueDependency dep{ source, handler, srcRate, srcPortIx, memStart, memEnd, depMin, depMax };
        return DependencyIterator{ dep };
    } else {
        auto result = factory::vector<ExecDependencyInfo>(StackID::TRANSFO);
        const auto *graph = source->convertTo<pisdf::Graph>();
        edge = graph->outputInterface(srcPortIx)->edge();
        for (auto k = depMin; k <= depMax; ++k) {
            const auto dependencies = computeRelMidExecDependency(edge, handler->getChildFiring(graph, k));
            std::move(std::begin(dependencies), std::end(dependencies), std::back_inserter(result));
        }
        return DependencyIterator(MultipleDependency(std::move(result)));
    }
}

spider::pisdf::DependencyIterator
spider::pisdf::detail::computeRelFirstExecDependency(const Edge *edge,
                                                     int64_t lowerCons,
                                                     int64_t prevSrcRate,
                                                     u32 prevSrcRV,
                                                     u32 prevDep,
                                                     const srless::FiringHandler *handler) {
    const auto *source = edge->source();
    /* == Fetching P_n and q_{p_n} == */
    const auto srcRate = edge->sourceRateExpression().evaluate(handler->getParams());
    const auto srcRV = handler->getRV(edge->source());
    /* == Updating C^{0,n}_{a|j,k} == */
    lowerCons = lowerCons - (prevSrcRV - (prevDep + 1)) * prevSrcRate;
    /* == Compute boundaries == */
    const auto snkRate = edge->sinkRateExpression().evaluate(handler->getParams());
    const auto depMin = static_cast<u32>(srcRV - math::ceilDiv(lowerCons, srcRate));
    if (!source->hierarchical()) {
        const auto srcPortIx = static_cast<u32>(edge->sourcePortIx());
        const auto memStart = static_cast<u32>((srcRate * srcRV - snkRate) % srcRate);
        const auto memEnd = static_cast<u32>(srcRate - 1);
        UniqueDependency dep{ source, handler, srcRate, srcPortIx, memStart, memEnd, depMin, srcRV - 1 };
        return DependencyIterator{ dep };
    } else {
        auto result = factory::vector<ExecDependencyInfo>(StackID::TRANSFO);
        const auto *graph = source->convertTo<pisdf::Graph>();
        edge = graph->outputInterface(edge->sourcePortIx())->edge();
        const auto *srcHandler = handler->getChildFiring(graph, depMin);
        const auto lowDeps = computeRelFirstExecDependency(edge, lowerCons, srcRate, srcRV, depMin, srcHandler);
        std::move(std::begin(lowDeps), std::end(lowDeps), std::back_inserter(result));
        for (auto k = depMin + 1; k <= srcRV - 1; ++k) {
            const auto dependencies = computeRelMidExecDependency(edge, handler->getChildFiring(graph, k));
            std::move(std::begin(dependencies), std::end(dependencies), std::back_inserter(result));
        }
        return DependencyIterator(MultipleDependency(std::move(result)));
    }
}

spider::pisdf::DependencyIterator
spider::pisdf::detail::computeRelLastExecDependency(const Edge *edge,
                                                    int64_t upperCons,
                                                    int64_t prevSrcRate,
                                                    u32 prevSrcRV,
                                                    u32 prevDep,
                                                    const srless::FiringHandler *handler) {
    const auto *source = edge->source();
    /* == Fetching P_n and q_{p_n} == */
    const auto srcRate = edge->sourceRateExpression().evaluate(handler->getParams());
    const auto srcRV = handler->getRV(edge->source());
    /* == Updating C^{1,n}_{a|j,k} == */
    upperCons = upperCons - (prevSrcRV - (prevDep + 1)) * prevSrcRate;
    /* == Compute boundaries == */
    const auto snkRate = edge->sinkRateExpression().evaluate(handler->getParams());
    const auto depMin = static_cast<u32>(srcRV - math::ceilDiv(snkRate, srcRate));
    const auto depMax = static_cast<u32>(srcRV - math::ceilDiv(upperCons, srcRate));
    if (!source->hierarchical()) {
        const auto srcPortIx = static_cast<u32>(edge->sourcePortIx());
        const auto memStart = static_cast<u32>((srcRate * srcRV - snkRate) % srcRate);
        const auto memEnd = static_cast<u32>((srcRate * srcRV - upperCons) % srcRate);
        UniqueDependency dep{ source, handler, srcRate, srcPortIx, memStart, memEnd, depMin, depMax };
        return DependencyIterator{ dep };
    } else {
        auto result = factory::vector<ExecDependencyInfo>(StackID::TRANSFO);
        const auto *graph = source->convertTo<pisdf::Graph>();
        edge = graph->outputInterface(edge->sourcePortIx())->edge();
        for (auto k = depMin; k < depMax; ++k) {
            const auto dependencies = computeRelMidExecDependency(edge, handler->getChildFiring(graph, k));
            std::move(std::begin(dependencies), std::end(dependencies), std::back_inserter(result));
        }
        handler = handler->getChildFiring(graph, depMax);
        const auto upDeps = computeRelLastExecDependency(edge, upperCons, srcRate, srcRV, depMax, handler);
        std::move(std::begin(upDeps), std::end(upDeps), std::back_inserter(result));
        return DependencyIterator(MultipleDependency(std::move(result)));
    }
}

spider::pisdf::DependencyIterator
spider::pisdf::detail::computeRelaxedExecDependencyImpl(const Edge *edge,
                                                        int64_t lowerCons,
                                                        int64_t upperCons,
                                                        int64_t prevSrcRate,
                                                        u32 prevSrcRV,
                                                        u32 prevLowDep, u32 prevUpDep,
                                                        const srless::FiringHandler *handler) {
    const auto *source = edge->source();
    /* == Fetching P_n and q_{p_n} == */
    const auto srcRate = edge->sourceRateExpression().evaluate(handler->getParams());
    const auto srcRV = handler->getRV(source);
    /* == Updating C^{0,n}_{a|j,k} == */
    lowerCons = lowerCons - (prevSrcRV - (prevLowDep + 1)) * prevSrcRate;
    /* == Updating C^{1,n}_{a|j,k} == */
    upperCons = upperCons - (prevSrcRV - (prevUpDep + 1)) * prevSrcRate;
    /* == Compute boundaries == */
    const auto depMin = static_cast<u32>(srcRV - math::ceilDiv(lowerCons, srcRate));
    const auto depMax = static_cast<u32>(srcRV - math::ceilDiv(upperCons, srcRate));
    if (!source->hierarchical()) {
        const auto srcPortIx = static_cast<u32>(edge->sourcePortIx());
        const auto memStart = static_cast<u32>((srcRate * srcRV - lowerCons) % srcRate);
        const auto memEnd = static_cast<u32>((srcRate * srcRV - upperCons) % srcRate);
        UniqueDependency dep{ source, handler, srcRate, srcPortIx, memStart, memEnd, depMin, depMax };
        return DependencyIterator{ dep };
    } else {
        const auto *graph = source->convertTo<pisdf::Graph>();
        edge = graph->outputInterface(edge->sourcePortIx())->edge();
        if (depMin == depMax) {
            const auto *h = handler->getChildFiring(graph, depMin);
            return computeRelaxedExecDependencyImpl(edge, lowerCons, upperCons, srcRate, srcRV, depMin, depMax, h);
        } else {
            auto result = factory::vector<ExecDependencyInfo>(StackID::TRANSFO);
            /* == Compute lower part of the dependencies == */
            const auto *srcH = handler->getChildFiring(graph, depMin);
            const auto lowDeps = computeRelFirstExecDependency(edge, lowerCons, srcRate, srcRV, depMin, srcH);
            std::move(std::begin(lowDeps), std::end(lowDeps), std::back_inserter(result));
            /* == Compute middle part of the dependencies == */
            for (auto k = depMin + 1; k < depMax; ++k) {
                const auto midDeps = computeRelMidExecDependency(edge, handler->getChildFiring(graph, k));
                std::move(std::begin(midDeps), std::end(midDeps), std::back_inserter(result));
            }
            /* == Compute upper part of the dependencies == */
            srcH = handler->getChildFiring(graph, depMax);
            const auto upDeps = computeRelLastExecDependency(edge, upperCons, srcRate, srcRV, depMax, srcH);
            std::move(std::begin(upDeps), std::end(upDeps), std::back_inserter(result));
            return DependencyIterator(MultipleDependency(std::move(result)));
        }
    }
}

spider::pisdf::DependencyIterator
spider::pisdf::detail::computeConsDependencyImpl(const Edge *edge,
                                                 int64_t lowerProd,
                                                 int64_t upperProd,
                                                 const srless::FiringHandler *handler) {
    if (edge->sink()->subtype() == pisdf::VertexType::DELAY) {
        const auto *delay = edge->sink()->convertTo<pisdf::DelayVertex>()->delay();
        lowerProd -= delay->value();
        upperProd -= delay->value();
        edge = delay->edge();
    }
    const auto *delay = edge->delay();
    const auto snkRate = edge->sinkRateExpression().evaluate(handler->getParams());
    const auto getRate = delay ? delay->getterRate(handler->getParams()) : 0;
    const auto snkTotRate = snkRate * handler->getRV(edge->sink());
    const auto delayValue = delay ? delay->value() : 0;
    if (lowerProd + delayValue >= snkTotRate) {
#ifndef NDEBUG
        if (!delay) {
            throwNullptrException();
        }
#endif
        /* == getter only == */
        UniqueDependency dep{ };
        dep.info_.vertex_ = delay->getter();
        dep.info_.handler_ = handler;
        dep.info_.rate_ = getRate;
        dep.info_.edgeIx_ = static_cast<u32>(delay->getterPortIx());
        dep.info_.memoryStart_ = static_cast<u32>((lowerProd + delayValue - snkTotRate) % getRate);
        dep.info_.memoryEnd_ = static_cast<u32>((upperProd + delayValue - snkTotRate - 1) % getRate);
        dep.info_.firingStart_ = static_cast<u32>(math::floorDiv(lowerProd + delayValue - snkTotRate, getRate));
        dep.info_.firingEnd_ = static_cast<u32>(math::floorDiv(upperProd + delayValue - 1 - snkTotRate, getRate));
        return DependencyIterator{ dep };
    } else if (upperProd + delayValue <= snkTotRate) {
        /* == sink only == */
        UniqueDependency dep{ };
        dep.info_.vertex_ = edge->sink();
        dep.info_.handler_ = handler;
        dep.info_.rate_ = snkRate;
        dep.info_.edgeIx_ = static_cast<u32>(edge->sinkPortIx());
        dep.info_.memoryStart_ = static_cast<u32>((lowerProd + delayValue) % snkRate);
        dep.info_.memoryEnd_ = static_cast<u32>((upperProd + delayValue - 1) % snkRate);
        dep.info_.firingStart_ = static_cast<u32>(math::floorDiv(lowerProd + delayValue, snkRate));
        dep.info_.firingEnd_ = static_cast<u32>(math::floorDiv(upperProd + delayValue - 1, snkRate));
        return DependencyIterator{ dep };
    } else {
#ifndef NDEBUG
        if (!delay) {
            throwNullptrException();
        }
#endif
        /* == sink + getter == */
        DualDependency dep{ };
        /* == snk info == */
        dep.infos_[0].vertex_ = edge->sink();
        dep.infos_[0].handler_ = handler;
        dep.infos_[0].rate_ = snkRate;
        dep.infos_[0].edgeIx_ = static_cast<u32>(edge->sinkPortIx());
        dep.infos_[0].memoryStart_ = static_cast<u32>((lowerProd + delayValue) % snkRate);
        dep.infos_[0].memoryEnd_ = static_cast<u32>(snkRate - 1);
        dep.infos_[0].firingStart_ = static_cast<u32>(math::floorDiv(lowerProd + delayValue, snkRate));
        dep.infos_[0].firingEnd_ = handler->getRV(edge->sink()) - 1;
        /* == get info == */
        dep.infos_[1].vertex_ = delay->getter();
        dep.infos_[1].handler_ = handler;
        dep.infos_[1].rate_ = getRate;
        dep.infos_[1].edgeIx_ = static_cast<u32>(delay->getterPortIx());
        dep.infos_[1].memoryStart_ = 0u;
        dep.infos_[1].memoryEnd_ = static_cast<u32>((upperProd + delayValue - snkTotRate - 1) % getRate);
        dep.infos_[1].firingStart_ = 0u;
        dep.infos_[1].firingEnd_ = static_cast<u32>(math::floorDiv(upperProd + delayValue - 1 - snkTotRate, getRate));
        return DependencyIterator{ dep };
    }
}
