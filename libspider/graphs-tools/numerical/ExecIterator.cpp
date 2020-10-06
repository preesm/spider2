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

#include <graphs-tools/numerical/ExecIterator.h>
#include <graphs-tools/transformation/srless/GraphHandler.h>
#include <graphs-tools/transformation/srless/FiringHandler.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/DelayVertex.h>

/* === Static function === */

namespace spider {
    namespace pisdf {

        static ExecDependencyInfo unresolved2 = { nullptr, nullptr, -1,
                                                  UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX };

        ExecIterator
        make_iterator(const Vertex *vertex, u32 firing, size_t edgeIx, const srless::FiringHandler *handler) {
#ifndef NDEBUG
            if (!handler || !vertex) {
                throwNullptrException();
            }
#endif
            const auto *edge = vertex->inputEdge(edgeIx);
            const auto snkRate = edge->sinkRateExpression().evaluate(handler->getParams());
            return ExecIterator{ edge, snkRate * firing, snkRate * (firing + 1) - 1, handler };
        }

    }
}

/* === Method(s) implementation === */

spider::pisdf::ExecIterator::ExecIterator(const Edge *edge,
                                          int64_t lowerCons,
                                          int64_t upperCons,
                                          const srless::FiringHandler *handler) :
        deps_{ factory::vector<spider::unique_ptr<ExecIterator>>(StackID::TRANSFO) } {
#ifndef NDEBUG
    if (!handler || !edge) {
        throwNullptrException();
    }
#endif
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
        int64_t offset = 0;
        if (sink->subtype() == VertexType::OUTPUT) {
            const auto totSrcRate = srcRate * handler->getRV(delayEdge->source());
            offset = totSrcRate - snkRate;
        } else {
            offset = snkRate * handler->getRV(delayEdge->sink());
        }
        const auto lCons = lowerCons + offset;
        const auto uCons = upperCons + offset;
        deps_.emplace_back(spider::make<ExecIterator, StackID::TRANSFO>(delayEdge, lCons, uCons, handler));
    } else if (lowerCons >= delayValue) {
        /* == source only == */
        if (sourceType == VertexType::INPUT) {
            auto dep = createExecDependency(edge, lowerCons, upperCons, srcRate, delayValue, handler);
            deps_.reserve(dep.firingEnd_ - dep.firingStart_ + 1);
            const auto upperLCons = srcRate * handler->firingValue();
            const auto *gh = handler->getParent()->handler();
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto start = k == dep.firingStart_ ? (lowerCons - delayValue) % srcRate : 0;
                const auto end = k == dep.firingEnd_ ? (upperCons - delayValue) % srcRate : srcRate - 1;
                const auto *upperEdge = edge->source()->graph()->inputEdge(edge->source()->ix());
                const auto lCons = upperLCons + start;
                const auto uCons = upperLCons + end;
                deps_.emplace_back(spider::make<ExecIterator, StackID::TRANSFO>(upperEdge, lCons, uCons, gh));
            }
        } else if (sourceType == VertexType::GRAPH) {
            auto dep = createExecDependency(edge, lowerCons, upperCons, srcRate, delayValue, handler);
            deps_.reserve(dep.firingEnd_ - dep.firingStart_ + 1);
            const auto *graph = edge->source()->convertTo<pisdf::Graph>();
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto *gh = handler->getChildFiring(graph, dep.firingStart_);
                if (!gh->isResolved()) {
                    info_ = spider::make_unique<ExecDependencyInfo, StackID::TRANSFO>(unresolved2);
                    deps_.clear();
                } else {
                    const auto *innerEdge = graph->outputInterface(edge->sourcePortIx())->edge();
                    const auto ifSrcRV = gh->getRV(innerEdge->source());
                    const auto ifSrcRate = innerEdge->sourceRateExpression().evaluate(gh->getParams());
                    const auto ifDelay = innerEdge->delay() ? innerEdge->delay()->value() : 0u;
                    const auto start = k == dep.firingStart_ ? (lowerCons - delayValue) % srcRate : 0;
                    const auto end = k == dep.firingEnd_ ? (upperCons - delayValue) % srcRate : srcRate - 1;
                    const auto lCons = (ifSrcRV * ifSrcRate - srcRate) + start + ifDelay;
                    const auto uCons = (ifSrcRV * ifSrcRate - srcRate) + end + ifDelay;
                    deps_.emplace_back(spider::make<ExecIterator, StackID::TRANSFO>(innerEdge, lCons, uCons, gh));
                }
            }
        } else {
            auto dep = createExecDependency(edge, lowerCons, upperCons, srcRate, delayValue, handler);
            info_ = spider::make_unique<ExecDependencyInfo, StackID::TRANSFO>(dep);
        }
    } else if (delay && (upperCons < delayValue)) {
        /* == setter only == */
        const auto *setterEdge = delay->setter()->outputEdge(delay->setterPortIx());
        deps_.emplace_back(spider::make<ExecIterator, StackID::TRANSFO>(setterEdge, lowerCons, upperCons, handler));
    } else if (delay) {
        /* == setter + source == */
        const auto *setterEdge = delay->setter()->outputEdge(delay->setterPortIx());
        deps_.emplace_back(
                spider::make<ExecIterator, StackID::TRANSFO>(setterEdge, lowerCons, delayValue - 1, handler));
        deps_.emplace_back(spider::make<ExecIterator, StackID::TRANSFO>(edge, delayValue, upperCons, handler));
    } else {
        throwNullptrException();
    }
}

spider::pisdf::ExecIterator::pointer_t spider::pisdf::ExecIterator::begin() {
    if (deps_.empty()) {
        return info_.get();
    }
    current_ = 0;
    return deps_[0u]->begin();
}

spider::pisdf::ExecIterator::pointer_t spider::pisdf::ExecIterator::operator++(int) {
    if (deps_.empty()) {
        return info_.get() + 1;
    }
    auto next = (*deps_[current_])++;
    if (next == deps_[current_]->end() && (current_ < (deps_.size() - 1))) {
        return deps_[current_++]->begin();
    }
    return next;
}

spider::pisdf::ExecIterator::pointer_t spider::pisdf::ExecIterator::end() {
    if (deps_.empty()) {
        return info_.get() + 1;
    }
    return deps_.back()->end();
}

/* === Private method(s) implementation === */

spider::pisdf::ExecDependencyInfo spider::pisdf::ExecIterator::createExecDependency(const Edge *edge,
                                                                                    int64_t lowerCons,
                                                                                    int64_t upperCons,
                                                                                    int64_t srcRate,
                                                                                    int64_t delayValue,
                                                                                    const srless::FiringHandler *handler) const {
    ExecDependencyInfo dep{ };
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
