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

#include <graphs-tools/transformation/srless/FiringHandler.h>
#include <graphs-tools/transformation/srless/GraphHandler.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs-tools/numerical/dependencies.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/DelayVertex.h>

/* === Static function === */

namespace spider {
    namespace srless {
        namespace detail {
            ExecDependencyInfo dummyInfo{ nullptr, nullptr, SIZE_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX, UINT32_MAX,
                                          UINT32_MAX };
        }
    }
}

/* === Method(s) implementation === */

spider::srless::FiringHandler::FiringHandler(const GraphHandler *parent,
                                             const spider::vector<std::shared_ptr<pisdf::Param>> &params,
                                             u32 firing) :
        params_{ factory::vector<std::shared_ptr<pisdf::Param>>(StackID::TRANSFO) },
        parent_{ parent },
        ix_{ SIZE_MAX },
        firing_{ firing },
        resolved_{ false } {
    if (!parent) {
        throwNullptrException();
    }
    const auto *graph = parent->graph();
    brv_ = spider::array<u32>(graph->vertexCount(), UINT32_MAX, StackID::TRANSFO);
    children_ = spider::array<GraphHandler *>(graph->subgraphCount(), nullptr, StackID::TRANSFO);
    taskIxRegister_ = spider::array<u32 *>(graph->vertexCount() + graph->inputEdgeCount(), nullptr, StackID::TRANSFO);
    /* == copy parameters == */
    params_.reserve(params.size());
    for (const auto &param : params) {
        params_.emplace_back(copyParameter(param, params));
    }
}

spider::srless::FiringHandler::~FiringHandler() {
    for (auto &ptr : taskIxRegister_) {
        deallocate(ptr);
    }
    for (auto &child : children_) {
        destroy(child);
    }
}

spider::srless::ExecDependency
spider::srless::FiringHandler::computeExecDependency(const pisdf::Vertex *vertex,
                                                     u32 firing,
                                                     u32 edgeIx) const {
    const auto *edge = vertex->inputEdge(edgeIx);
    const auto snkRate = edge->sinkRateExpression().evaluate(params_);
    if (!snkRate) {
        return { detail::dummyInfo, detail::dummyInfo };
    }
    const auto *source = edge->source();
    if (source->subtype() == pisdf::VertexType::INPUT) {
        if (!isInputInterfaceTransparent(source->ix())) {
            return computeExecDependency(edge, snkRate * firing, snkRate * (firing + 1));
        } else {
            const auto ix = static_cast<u32>(source->ix());
            auto res = parent_->handler()->computeExecDependency(parent_->graph(), firing_, ix);
            res.first_.memoryStart_ = static_cast<u32>(snkRate * firing) % static_cast<u32>(res.first_.rate_);
            if (res.second_.vertex_) {
                res.second_.memoryEnd_ =
                        static_cast<u32>(snkRate * (firing + 1) - 1) % static_cast<u32>(res.second_.rate_);
            } else {
                res.first_.memoryEnd_ =
                        static_cast<u32>(snkRate * (firing + 1) - 1) % static_cast<u32>(res.first_.rate_);
            }
            return res;
        }
    } else if (source->subtype() == pisdf::VertexType::DELAY) {
        const auto *delay = edge->source()->convertTo<pisdf::DelayVertex>()->delay();
        const auto *delayEdge = delay->edge();
        const auto getterOffset = delayEdge->sinkRateExpression().evaluate(params_) * getRV(delayEdge->sink());
        const auto lowerCons = snkRate * firing + getterOffset;
        const auto upperCons = snkRate * (firing + 1u) + getterOffset;
        return computeExecDependency(delayEdge, lowerCons, upperCons);
    }
    return computeExecDependency(edge, snkRate * firing, snkRate * (firing + 1));
}


spider::vector<spider::srless::ExecDependency>
spider::srless::FiringHandler::computeRelaxedExecDependency(const pisdf::Vertex *vertex,
                                                            u32 firing,
                                                            u32 edgeIx) const {
    auto dependencies = factory::vector<ExecDependency>(StackID::TRANSFO);
    auto *edge = vertex->inputEdge(edgeIx);
    auto snkRate = edge->sinkRateExpression().evaluate(params_);
    if (!snkRate) {
        dependencies.push_back({ detail::dummyInfo, detail::dummyInfo });
    } else if (!edge->source()->hierarchical()) {
        dependencies.emplace_back(computeExecDependency(vertex, firing, edgeIx));
    } else {
        const auto *source = edge->source();
        const auto *graph = source->convertTo<pisdf::Graph>();
        auto dep = computeExecDependency(vertex, firing, edgeIx);
        if (dep.first_.vertex_ == source) {
            const auto srcRate = edge->sourceRateExpression().evaluate(params_);
            const auto delayValue = edge->delay() ? edge->delay()->value() : 0;
            const auto lowerCons = (dep.first_.firingStart_ + 1) * srcRate + delayValue - snkRate * firing;
            const auto upperCons = (dep.first_.firingEnd_ + 1) * srcRate + delayValue - snkRate * (firing + 1) + 1;
            const auto *nextEdge = graph->outputInterface(edge->sourcePortIx())->edge();
            const auto *lowHandler = getChildFiring(graph, dep.first_.firingStart_);
            if (dep.first_.firingStart_ == dep.first_.firingEnd_) {
                spider::append(dependencies,
                               lowHandler->computeRelaxedExecDependency(nextEdge, lowerCons, upperCons, 0, 0, 0, 0));
            } else {
                spider::append(dependencies, lowHandler->computeRelFirstExecDependency(nextEdge, lowerCons, 0, 0, 0));
                for (auto k = dep.first_.firingStart_ + 1; k < dep.first_.firingEnd_; ++k) {
                    spider::append(dependencies, getChildFiring(graph, k)->computeRelMidExecDependency(nextEdge));
                }
                const auto *upHandler = getChildFiring(graph, dep.first_.firingEnd_);
                spider::append(dependencies, upHandler->computeRelLastExecDependency(nextEdge, upperCons, 0, 0, 0));
            }
        } else if (dep.second_.vertex_ == source) {
        }
    }
    return dependencies;
}

spider::srless::ExecDependency
spider::srless::FiringHandler::computeConsDependenciesByEdge(const pisdf::Vertex *vertex,
                                                             u32 firing,
                                                             u32 edgeIx) const {
    const auto *edge = vertex->outputEdge(edgeIx);
    const auto srcRate = edge->sourceRateExpression().evaluate(params_);
    if (!srcRate) {
        return { detail::dummyInfo, detail::dummyInfo };
    }
    if (edge->sink()->subtype() == pisdf::VertexType::DELAY) {
        const auto *delay = edge->sink()->convertTo<pisdf::DelayVertex>()->delay();
        const auto lowerProd = srcRate * firing - delay->value();
        const auto upperProd = srcRate * (firing + 1u) - delay->value();
        return computeConsDependency(delay->edge(), lowerProd, upperProd);
    } else {
        return computeConsDependency(edge, srcRate * firing, srcRate * (firing + 1));
    }
}

bool spider::srless::FiringHandler::isInputInterfaceTransparent(size_t ix) const {
#ifndef NDEBUG
    if (ix >= parent_->graph()->inputEdgeCount()) {
        throwSpiderException("invalid input interface ix");
    }
#endif
    const auto *edge = parent_->graph()->inputInterface(ix)->edge();
    const auto srcRate = edge->sourceRateExpression().evaluate(params_);
    const auto snkRate = edge->sinkRateExpression().evaluate(params_);
    return ((snkRate * getRV(edge->sink())) % srcRate) == 0;
}

void spider::srless::FiringHandler::registerTaskIx(const pisdf::Vertex *vertex, u32 firing, u32 taskIx) {
#ifndef NDEBUG
    if (firing >= getRV(vertex)) {
        throwSpiderException("invalid vertex firing.");
    }
#endif
//    if (vertex->subtype() == pisdf::VertexType::INPUT) {
//        taskIxRegister_.at(vertex->ix() + parent_->graph()->vertexCount())[0u] = taskIx;
//    }
    taskIxRegister_.at(vertex->ix())[firing] = taskIx;
}

void spider::srless::FiringHandler::registerTaskIx(const pisdf::Interface *interface, u32 taskIx) {
#ifndef NDEBUG
    if (interface->subtype() != pisdf::VertexType::INPUT) {
        throwSpiderException("invalid interface type.");
    }
#endif
    taskIxRegister_.at(interface->ix() + parent_->graph()->vertexCount())[0u] = taskIx;
}

void spider::srless::FiringHandler::resolveBRV() {
    /* == update dependent params == */
    for (const auto &param : params_) {
        if (param->type() == pisdf::ParamType::DYNAMIC_DEPENDANT) {
            param->setValue(param->value(params_));
        }
    }
    /* == Compute BRV == */
    spider::brv::compute(parent_->graph(), params_);
    /* == Save RV values into the array == */
    for (const auto &vertex : parent_->graph()->vertices()) {
        const auto ix = vertex->ix();
        const auto rvValue = vertex->repetitionValue();
        if (brv_.at(ix) != rvValue) {
            brv_[ix] = static_cast<u32>(rvValue);
            deallocate(taskIxRegister_.at(ix));
            taskIxRegister_[ix] = spider::allocate<u32, StackID::TRANSFO>(rvValue);
        }
        std::fill(taskIxRegister_.at(ix), std::next(taskIxRegister_.at(ix), rvValue), UINT32_MAX);
    }
    /* == Check input interfaces == */
    for (const auto &interface : parent_->graph()->inputInterfaceVector()) {
        const auto ix = interface->ix() + parent_->graph()->vertexCount();
        if (!isInputInterfaceTransparent(interface->ix())) {
            if (!taskIxRegister_[ix]) {
                taskIxRegister_[ix] = spider::allocate<u32, StackID::TRANSFO>(1u);
            }
            taskIxRegister_[ix][0u] = UINT32_MAX;
        } else if (taskIxRegister_[ix]) {
            deallocate(taskIxRegister_[ix]);
            taskIxRegister_[ix] = nullptr;
        }
    }
    /* == creates children == */
    for (const auto &subgraph : parent_->graph()->subgraphs()) {
        destroy(children_.at(subgraph->subIx()));
        children_.at(subgraph->subIx()) = spider::make<GraphHandler>(subgraph, params_, subgraph->repetitionValue(),
                                                                     this);
    }
    resolved_ = true;
}

void spider::srless::FiringHandler::clear() {
    for (const auto &vertex : parent_->graph()->vertices()) {
        const auto ix = vertex->ix();
        brv_.at(ix) = UINT32_MAX;
        deallocate(taskIxRegister_.at(ix));
        taskIxRegister_[ix] = nullptr;
    }
    for (const auto &subgraph : parent_->graph()->subgraphs()) {
        destroy(children_.at(subgraph->subIx()));
    }
    resolved_ = false;
}

u32 spider::srless::FiringHandler::getRV(const spider::pisdf::Vertex *vertex) const {
#ifndef NDEBUG
    if (vertex->graph() != parent_->graph()) {
        throwSpiderException("vertex does not belong to the correct graph.");
    }
#endif
    if (vertex->subtype() == pisdf::VertexType::INPUT || vertex->subtype() == pisdf::VertexType::OUTPUT) {
        return 1u;
    }
    return brv_.at(vertex->ix());
}

u32 spider::srless::FiringHandler::getTaskIx(const spider::pisdf::Vertex *vertex, u32 vertexFiring) const {
#ifndef NDEBUG
    if (vertexFiring >= getRV(vertex)) {
        throwSpiderException("invalid vertex firing.");
    }
#endif
//    if (vertex->subtype() == pisdf::VertexType::INPUT) {
//        return taskIxRegister_.at(vertex->ix() + parent_->graph()->vertexCount())[0u];
//    }
    return taskIxRegister_.at(vertex->ix())[vertexFiring];
}

u32 spider::srless::FiringHandler::getTaskIx(const spider::pisdf::Interface *interface) const {
#ifndef NDEBUG
    if (interface->subtype() != pisdf::VertexType::INPUT) {
        throwSpiderException("invalid interface type.");
    }
#endif
    return taskIxRegister_.at(interface->ix() + parent_->graph()->vertexCount())[0u];
}

const spider::srless::FiringHandler *
spider::srless::FiringHandler::getChildFiring(const pisdf::Graph *subgraph, u32 firing) const {
#ifndef NDEBUG
    if (subgraph->graph() != parent_->graph()) {
        throwSpiderException("subgraph does not belong to this graph.");
    }
#endif
    return &(children_[subgraph->subIx()]->firings()[firing]);
}

int64_t spider::srless::FiringHandler::getParamValue(size_t ix) {
    return spider::get_at(params_, ix)->value(params_);
}

void spider::srless::FiringHandler::setParamValue(size_t ix, int64_t value) {
    spider::get_at(params_, ix)->setValue(value);
}

/* === Private method(s) implementation === */

std::shared_ptr<spider::pisdf::Param>
spider::srless::FiringHandler::copyParameter(const std::shared_ptr<pisdf::Param> &param,
                                             const spider::vector<std::shared_ptr<pisdf::Param>> &parentParams) {
    if (param->dynamic()) {
        std::shared_ptr<pisdf::Param> newParam;
        if (param->type() == pisdf::ParamType::INHERITED) {
            const auto &parent = parentParams[param->parent()->ix()];
            newParam = spider::make_shared<pisdf::Param, StackID::PISDF>(param->name(), parent->value(parentParams));
        } else {
            newParam = spider::make_shared<pisdf::Param, StackID::PISDF>(*param);
        }
        newParam->setIx(param->ix());
        return newParam;
    }
    return param;
}

int64_t spider::srless::FiringHandler::computeSrcRate(const spider::pisdf::Edge *edge) const {
    if (edge->source()->subtype() == pisdf::VertexType::INPUT) {
        return edge->sinkRateExpression().evaluate(params_) * getRV(edge->sink());
    }
    return edge->sourceRateExpression().evaluate(params_);
}

spider::srless::ExecDependency spider::srless::FiringHandler::computeExecDependency(const pisdf::Edge *edge,
                                                                                    int64_t lowerCons,
                                                                                    int64_t upperCons) const {
    const auto *delay = edge->delay();
    const auto srcRate = computeSrcRate(edge);
    const auto setRate = delay ? delay->setterRate(params_) : 0;
    const auto delayValue = delay ? delay->value() : 0;
    auto result = ExecDependency{ detail::dummyInfo, detail::dummyInfo };
    if (lowerCons >= delayValue) {
        /* == source only == */
        result.first_.vertex_ = edge->source();
        result.first_.handler_ = this;
        result.first_.rate_ = static_cast<size_t>(srcRate);
        result.first_.edgeIx_ = static_cast<u32>(edge->sourcePortIx());
        result.first_.memoryStart_ = static_cast<u32>((lowerCons - delayValue) % srcRate);
        result.first_.memoryEnd_ = static_cast<u32>((upperCons - delayValue - 1) % srcRate);
        result.first_.firingStart_ = static_cast<u32>(math::floorDiv(lowerCons - delayValue, srcRate));
        result.first_.firingEnd_ = static_cast<u32>(math::floorDiv(upperCons - delayValue - 1, srcRate));
    } else if (upperCons <= delayValue) {
#ifndef NDEBUG
        if (!delay) {
            throwNullptrException();
        }
#endif
        /* == setter only == */
        result.first_.vertex_ = delay->setter();
        result.first_.handler_ = this;
        result.first_.rate_ = static_cast<size_t>(setRate);
        result.first_.edgeIx_ = static_cast<u32>(delay->setterPortIx());
        result.first_.memoryStart_ = static_cast<u32>(lowerCons % setRate);
        result.first_.memoryEnd_ = static_cast<u32>((upperCons - 1) % setRate);
        result.first_.firingStart_ = static_cast<u32>(math::floorDiv(lowerCons, setRate));
        result.first_.firingEnd_ = static_cast<u32>(math::floorDiv(upperCons - 1, setRate));
    } else {
#ifndef NDEBUG
        if (!delay) {
            throwNullptrException();
        }
#endif
        /* == setter + source only == */
        /* == set info == */
        result.first_.vertex_ = delay->setter();
        result.first_.handler_ = this;
        result.first_.rate_ = static_cast<size_t>(setRate);
        result.first_.edgeIx_ = static_cast<u32>(delay->setterPortIx());
        result.first_.memoryStart_ = static_cast<u32>(lowerCons % setRate);
        result.first_.memoryEnd_ = static_cast<u32>(setRate - 1);
        result.first_.firingStart_ = 0u;
        result.first_.firingEnd_ = getRV(delay->setter()) - 1;
        /* == src info == */
        result.second_.vertex_ = edge->source();
        result.second_.handler_ = this;
        result.second_.rate_ = static_cast<size_t>(srcRate);
        result.second_.edgeIx_ = static_cast<u32>(edge->sourcePortIx());
        result.second_.memoryStart_ = 0u;
        result.second_.memoryEnd_ = static_cast<u32>((upperCons - delayValue - 1) % srcRate);
        result.second_.firingStart_ = 0u;
        result.second_.firingEnd_ = static_cast<u32>(math::floorDiv(upperCons - delayValue - 1, srcRate));
    }
    return result;
}

spider::srless::ExecDependency spider::srless::FiringHandler::computeConsDependency(const pisdf::Edge *edge,
                                                                                    int64_t lowerProd,
                                                                                    int64_t upperProd) const {
    const auto *delay = edge->delay();
    const auto snkRate = edge->sinkRateExpression().evaluate(params_);
    const auto getRate = delay ? delay->getterRate(params_) : 0;
    const auto snkTotRate = snkRate * getRV(edge->sink());
    const auto delayValue = delay ? delay->value() : 0;
    auto result = ExecDependency{ detail::dummyInfo, detail::dummyInfo };
    if (lowerProd + delayValue >= snkTotRate) {
#ifndef NDEBUG
        if (!delay) {
            throwNullptrException();
        }
#endif
        /* == getter only == */
        result.first_.vertex_ = delay->getter();
        result.first_.handler_ = this;
        result.first_.rate_ = static_cast<size_t>(getRate);
        result.first_.edgeIx_ = static_cast<u32>(delay->getterPortIx());
        result.first_.memoryStart_ = static_cast<u32>((lowerProd + delayValue - snkTotRate) % getRate);
        result.first_.memoryEnd_ = static_cast<u32>((upperProd + delayValue - snkTotRate - 1) % getRate);
        result.first_.firingStart_ = static_cast<u32>(math::floorDiv(lowerProd + delayValue - snkTotRate, getRate));
        result.first_.firingEnd_ = static_cast<u32>(math::floorDiv(upperProd + delayValue - 1 - snkTotRate, getRate));
    } else if (upperProd + delayValue <= snkTotRate) {
        /* == sink only == */
        result.first_.vertex_ = edge->sink();
        result.first_.handler_ = this;
        result.first_.rate_ = static_cast<size_t>(snkRate);
        result.first_.edgeIx_ = static_cast<u32>(edge->sinkPortIx());
        result.first_.memoryStart_ = static_cast<u32>((lowerProd + delayValue) % snkRate);
        result.first_.memoryEnd_ = static_cast<u32>((upperProd + delayValue - 1) % snkRate);
        result.first_.firingStart_ = static_cast<u32>(math::floorDiv(lowerProd + delayValue, snkRate));
        result.first_.firingEnd_ = static_cast<u32>(math::floorDiv(upperProd + delayValue - 1, snkRate));
    } else {
#ifndef NDEBUG
        if (!delay) {
            throwNullptrException();
        }
#endif
        /* == sink + getter == */
        /* == snk info == */
        result.first_.vertex_ = edge->sink();
        result.first_.handler_ = this;
        result.first_.rate_ = static_cast<size_t>(snkRate);
        result.first_.edgeIx_ = static_cast<u32>(edge->sinkPortIx());
        result.first_.memoryStart_ = static_cast<u32>((lowerProd + delayValue) % snkRate);
        result.first_.memoryEnd_ = static_cast<u32>(snkRate - 1);
        result.first_.firingStart_ = static_cast<u32>(math::floorDiv(lowerProd + delayValue, snkRate));
        result.first_.firingEnd_ = getRV(edge->sink()) - 1;
        /* == get info == */
        result.second_.vertex_ = delay->getter();
        result.second_.handler_ = this;
        result.second_.rate_ = static_cast<size_t>(getRate);
        result.second_.edgeIx_ = static_cast<u32>(delay->getterPortIx());
        result.second_.memoryStart_ = 0u;
        result.second_.memoryEnd_ = static_cast<u32>((upperProd + delayValue - snkTotRate - 1) % getRate);
        result.second_.firingStart_ = 0u;
        result.second_.firingEnd_ = static_cast<u32>(math::floorDiv(upperProd + delayValue - 1 - snkTotRate, getRate));
    }
    return result;
}

spider::vector<spider::srless::ExecDependency>
spider::srless::FiringHandler::computeRelaxedExecDependency(const spider::pisdf::Edge *edge,
                                                            int64_t lowerCons,
                                                            int64_t upperCons,
                                                            int64_t prevSrcRate,
                                                            u32 prevSrcRV,
                                                            u32 prevLowDep,
                                                            u32 prevUpDep) const {
    auto dependencies = factory::vector<ExecDependency>(StackID::TRANSFO);
    const auto *source = edge->source();
    /* == Fetching P_n and q_{p_n} == */
    const auto srcRate = edge->sourceRateExpression().evaluate(params_);
    const auto srcRV = getRV(source);
    /* == Updating C^{0,n}_{a|j,k} == */
    lowerCons = lowerCons - (prevSrcRV - (prevLowDep + 1)) * prevSrcRate;
    const auto depMin = static_cast<u32>(srcRV - math::ceilDiv(lowerCons, srcRate));
    /* == Updating C^{1,n}_{a|j,k} == */
    upperCons = upperCons - (prevSrcRV - (prevUpDep + 1)) * prevSrcRate;
    const auto depMax = static_cast<u32>(srcRV - math::ceilDiv(upperCons, srcRate));
    if (source->hierarchical()) {
        const auto *graph = source->convertTo<pisdf::Graph>();
        const auto *nextEdge = graph->outputInterface(edge->sourcePortIx())->edge();
        if (depMax == depMin) {
            spider::append(dependencies,
                           getChildFiring(graph, depMin)->computeRelaxedExecDependency(nextEdge,
                                                                                       lowerCons,
                                                                                       upperCons,
                                                                                       srcRate,
                                                                                       srcRV,
                                                                                       depMin,
                                                                                       depMax));
        } else {
            const auto *lowHandler = getChildFiring(graph, depMin);
            spider::append(dependencies,
                           lowHandler->computeRelFirstExecDependency(nextEdge, lowerCons, srcRate, srcRV, depMin));
            for (auto k = depMin + 1; k < depMax; ++k) {
                spider::append(dependencies, getChildFiring(graph, k)->computeRelMidExecDependency(nextEdge));
            }
            const auto *upHandler = getChildFiring(graph, depMax);
            spider::append(dependencies,
                           upHandler->computeRelLastExecDependency(nextEdge, upperCons, srcRate, srcRV, depMax));
        }
    } else {
        const auto srcPortIx = static_cast<u32>(edge->sourcePortIx());
        const auto memStart = static_cast<u32>((srcRate * srcRV - lowerCons) % srcRate);
        const auto memEnd = static_cast<u32>((srcRate * srcRV - upperCons) % srcRate);
        dependencies.push_back(
                {{ edge->source(), this, static_cast<size_t>(srcRate), srcPortIx, memStart, memEnd, depMin, depMax },
                 detail::dummyInfo });
    }
    return dependencies;
}

spider::vector<spider::srless::ExecDependency>
spider::srless::FiringHandler::computeRelFirstExecDependency(const pisdf::Edge *edge,
                                                             int64_t consumption,
                                                             int64_t prevSrcRate,
                                                             u32 prevSrcRV,
                                                             u32 prevDep) const {
    auto dependencies = factory::vector<ExecDependency>(StackID::TRANSFO);
    const auto snkRate = edge->sinkRateExpression().evaluate(params_);
    const auto *source = edge->source();
    /* == Fetching P_n and q_{p_n} == */
    const auto srcRate = edge->sourceRateExpression().evaluate(params_);
    const auto srcRV = getRV(edge->source());
    /* == Updating C^{0,n}_{a|j,k} == */
    consumption = consumption - (prevSrcRV - (prevDep + 1)) * prevSrcRate;
    const auto depMin = static_cast<u32>(srcRV - math::ceilDiv(consumption, srcRate));
    if (source->hierarchical()) {
        const auto *graph = source->convertTo<pisdf::Graph>();
        const auto *srcHandler = getChildFiring(graph, depMin);
        const auto *nextEdge = graph->outputInterface(edge->sourcePortIx())->edge();
        spider::append(dependencies,
                       srcHandler->computeRelFirstExecDependency(nextEdge, consumption, srcRate, srcRV, depMin));
        for (auto k = depMin + 1; k <= srcRV - 1; ++k) {
            spider::append(dependencies, getChildFiring(graph, k)->computeRelMidExecDependency(nextEdge));
        }
    } else {
        const auto srcPortIx = static_cast<u32>(edge->sourcePortIx());
        const auto memStart = static_cast<u32>((srcRate * srcRV - snkRate) % srcRate);
        const auto memEnd = static_cast<u32>(srcRate - 1);
        dependencies.push_back(
                {{ edge->source(), this, static_cast<size_t>(srcRate), srcPortIx, memStart, memEnd, depMin, srcRV - 1 },
                 detail::dummyInfo });
    }
    return dependencies;
}

spider::vector<spider::srless::ExecDependency>
spider::srless::FiringHandler::computeRelMidExecDependency(const spider::pisdf::Edge *edge) const {
    auto dependencies = factory::vector<ExecDependency>(StackID::TRANSFO);
    const auto *source = edge->source();
    if (source->hierarchical()) {
        const auto dep = computeExecDependency(edge->sink(), 0, 0);
        for (auto k = dep.first_.firingStart_; k <= dep.first_.firingEnd_; ++k) {
            const auto *graph = source->convertTo<pisdf::Graph>();
            const auto *srcHandler = getChildFiring(graph, k);
            const auto *nextEdge = graph->outputInterface(edge->sourcePortIx())->edge();
            spider::append(dependencies, srcHandler->computeRelMidExecDependency(nextEdge));
        }
    } else {
        dependencies.emplace_back(computeExecDependency(edge->sink(), 0, 0));
    }
    return dependencies;
}

spider::vector<spider::srless::ExecDependency>
spider::srless::FiringHandler::computeRelLastExecDependency(const pisdf::Edge *edge,
                                                            int64_t consumption,
                                                            int64_t prevSrcRate,
                                                            u32 prevSrcRV,
                                                            u32 prevDep) const {
    auto dependencies = factory::vector<ExecDependency>(StackID::TRANSFO);
    const auto snkRate = edge->sinkRateExpression().evaluate(params_);
    const auto *source = edge->source();
    /* == Fetching P_n and q_{p_n} == */
    const auto srcRate = edge->sourceRateExpression().evaluate(params_);
    const auto srcRV = getRV(edge->source());
    const auto depMin = static_cast<u32>(srcRV - math::ceilDiv(snkRate, srcRate));
    /* == Updating C^{1,n}_{a|j,k} == */
    consumption = consumption - (prevSrcRV - (prevDep + 1)) * prevSrcRate;
    const auto depMax = static_cast<u32>(srcRV - math::ceilDiv(consumption, srcRate));
    if (source->hierarchical()) {
        const auto *graph = source->convertTo<pisdf::Graph>();
        const auto *nextEdge = graph->outputInterface(edge->sourcePortIx())->edge();
        for (auto k = depMin; k < depMax; ++k) {
            spider::append(dependencies, getChildFiring(graph, k)->computeRelMidExecDependency(nextEdge));
        }
        const auto *srcHandler = getChildFiring(graph, depMax);
        spider::append(dependencies,
                       srcHandler->computeRelLastExecDependency(nextEdge, consumption, srcRate, srcRV, depMax));
    } else {
        const auto srcPortIx = static_cast<u32>(edge->sourcePortIx());
        const auto memStart = static_cast<u32>((srcRate * srcRV - snkRate) % srcRate);
        const auto memEnd = static_cast<u32>((srcRate * srcRV - consumption) % srcRate);
        dependencies.push_back(
                {{ edge->source(), this, static_cast<size_t>(srcRate), srcPortIx, memStart, memEnd, depMin, depMax },
                 detail::dummyInfo });
    }
    return dependencies;
}








