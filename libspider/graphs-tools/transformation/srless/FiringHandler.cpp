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
    children_ = spider::array<GraphHandler *>(graph->subgraphCount(), StackID::TRANSFO);
    taskIxRegister_ = spider::array<u32 *>(graph->vertexCount(), StackID::TRANSFO);
    std::fill(std::begin(taskIxRegister_), std::end(taskIxRegister_), nullptr);
    std::fill(std::begin(children_), std::end(children_), nullptr);
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

void spider::srless::FiringHandler::resolveBRV() {
    /* == update dependent params == */
    for (const auto &param : params_) {
        if (param->type() == pisdf::ParamType::DYNAMIC_DEPENDANT) {
            param->setValue(param->value(params_));
        }
    }
    spider::brv::compute(parent_->graph(), params_);
    for (const auto &vertex : parent_->graph()->vertices()) {
        const auto ix = vertex->ix();
        const auto rvValue = vertex->repetitionValue();
        brv_.at(ix) = static_cast<u32>(rvValue);
        auto &currentPointer = taskIxRegister_.at(ix);
        deallocate(currentPointer);
        currentPointer = spider::allocate<u32, StackID::TRANSFO>(rvValue);
        std::fill(currentPointer, std::next(currentPointer, static_cast<long>(rvValue)), SIZE_MAX);
    }
    /* == creates children == */
    for (const auto &subgraph : parent_->graph()->subgraphs()) {
        destroy(children_.at(subgraph->subIx()));
        children_.at(subgraph->subIx()) = spider::make<GraphHandler>(subgraph, params_, subgraph->repetitionValue(),
                                                                     this);
    }
    resolved_ = true;
}

u32 spider::srless::FiringHandler::getRV(const spider::pisdf::Vertex *vertex) const {
#ifndef NDEBUG
    if (vertex->graph() != parent_->graph()) {
        throwSpiderException("vertex does not belong to the correct graph.");
    }
#endif
    return brv_.at(vertex->ix());
}

spider::vector<spider::srless::ExecDependency>
spider::srless::FiringHandler::computeExecDependenciesByFiring(const pisdf::Vertex *vertex, u32 vertexFiring) const {
    auto dependencies = factory::vector<ExecDependency>(StackID::TRANSFO);
    dependencies.reserve(vertex->inputEdgeCount());
    for (const auto *edge : vertex->inputEdgeVector()) {
        dependencies.emplace_back(
                computeExecDependenciesByEdge(vertex, vertexFiring, static_cast<u32>(edge->sinkPortIx())));
    }
    return dependencies;
}

spider::srless::ExecDependency
spider::srless::FiringHandler::computeExecDependenciesByEdge(const pisdf::Vertex *vertex,
                                                             u32 firing,
                                                             u32 edgeIx) const {
    const auto *edge = vertex->inputEdge(edgeIx);
    const auto snkRate = edge->sinkRateExpression().evaluate(params_);
    if (!snkRate) {
        return { detail::dummyInfo, detail::dummyInfo };
    }
    const auto *source = edge->source();
    if (vertex->subtype() == pisdf::VertexType::OUTPUT) {
        const auto srcRate = edge->sourceRateExpression().evaluate(params_);
        const auto srcPortIx = static_cast<u32>(edge->sourcePortIx());
        const auto srcRV = getRV(edge->source());
        const auto memStart = static_cast<u32>(snkRate % srcRate);
        const auto memEnd = static_cast<u32>(srcRate - 1);
        const auto depMin = static_cast<u32>(srcRV - math::ceilDiv(snkRate, srcRate));
        return {{ edge->source(), this, static_cast<size_t>(srcRate), srcPortIx, memStart, memEnd, depMin, srcRV - 1 },
                detail::dummyInfo };
    } else if (source->subtype() == pisdf::VertexType::INPUT) {
        const auto *parentHandler = parent_->handler();
        const auto *graph = parent_->graph();
        auto res = parentHandler->computeExecDependenciesByEdge(graph, firing_, static_cast<u32>(edge->sinkPortIx()));
        res.first_.memoryStart_ = static_cast<u32>(snkRate * firing) % static_cast<u32>(res.first_.rate_);
        if (res.second_.vertex_) {
            res.second_.memoryEnd_ =
                    static_cast<u32>(snkRate * (firing + 1) - 1) % static_cast<u32>(res.second_.rate_);
        } else {
            res.first_.memoryEnd_ =
                    static_cast<u32>(snkRate * (firing + 1) - 1) % static_cast<u32>(res.first_.rate_);
        }
        return res;
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
spider::srless::FiringHandler::computeHExecDependenciesByEdge(const pisdf::Vertex *vertex,
                                                              u32 firing,
                                                              u32 edgeIx) const {
    auto dependencies = factory::vector<ExecDependency>(StackID::TRANSFO);
    const auto *edge = vertex->inputEdge(edgeIx);
    const auto snkRate = edge->sinkRateExpression().evaluate(params_);
    if (!snkRate) {
        dependencies.push_back({ detail::dummyInfo, detail::dummyInfo });
    } else {
        auto dep = computeExecDependenciesByEdge(vertex, firing, edgeIx);
        if (!dep.first_.vertex_->hierarchical()) {
//            if (dep.second_.vertex_) {
//                if (!dep.second_.vertex_->hierarchical()) {
            dependencies.emplace_back(dep);
//                } else {
//                    dependencies.push_back({ dep.first_, detail::dummyInfo });
//                    const auto *graph = dep.second_.vertex_->convertTo<pisdf::Graph>();
//                    for (auto k = dep.second_.firingStart_; k < dep.second_.firingEnd_; ++k) {
//                        const auto *handler = &(children_[graph->subIx()]->firings()[k]);
//                        auto res = handler->computeHExecDependenciesByEdge(graph->outputInterface(dep.second_.edgeIx_),
//                                                                           k,
//                                                                           dep.second_.edgeIx_);
//                        for (auto &v : res) {
//                            dependencies.emplace_back(v);
//                        }
//                    }
//                    dependencies[1u].first_.memoryStart_ = static_cast<u32>(snkRate) %
//                                                           static_cast<u32>(dependencies[1u].first_.rate_);
//                    dependencies.back().first_.memoryEnd_ = static_cast<u32>(snkRate * (firing + 1) - 1) %
//                                                            static_cast<u32>(dependencies.back().first_.rate_);
//                }
//            }
        } else {
            const auto *graph = dep.first_.vertex_->convertTo<pisdf::Graph>();
            for (auto k = dep.first_.firingStart_; k <= dep.first_.firingEnd_; ++k) {
                const auto *handler = &(children_[graph->subIx()]->firings()[k]);
                if (!handler->isResolved()) {
                    dependencies.push_back({ detail::dummyInfo, detail::dummyInfo });
                    return dependencies;
                }
                const auto *interface = graph->outputInterface(dep.first_.edgeIx_);
                if (interface->opposite()->hierarchical()) {
                    spider::append(handler->computeHExecDependenciesByEdge(interface, 0, 0), dependencies);
                } else {
                    dependencies.emplace_back(handler->computeExecDependenciesByEdge(interface, 0, 0));
                }
            }
            dependencies[0u].first_.memoryStart_ = static_cast<u32>(snkRate * firing) %
                                                   static_cast<u32>(dependencies[0u].first_.rate_);
            dependencies.back().first_.memoryEnd_ = static_cast<u32>(snkRate * (firing + 1) - 1) %
                                                    static_cast<u32>(dependencies.back().first_.rate_);
        }
    }
    return dependencies;
}

spider::srless::ExecDependency
spider::srless::FiringHandler::computeConsDependenciesByEdge(const pisdf::Vertex *vertex,
                                                             u32 firing,
                                                             u32 edgeIx) const {
    return computeCons(vertex->outputEdge(edgeIx), firing);
}

void spider::srless::FiringHandler::registerTaskIx(const pisdf::Vertex *vertex, u32 vertexFiring, u32 taskIx) {
#ifndef NDEBUG
    if (vertexFiring >= getRV(vertex)) {
        throwSpiderException("invalid vertex firing.");
    }
#endif
    taskIxRegister_.at(vertex->ix())[vertexFiring] = taskIx;
}

u32 spider::srless::FiringHandler::getTaskIx(const spider::pisdf::Vertex *vertex, u32 vertexFiring) const {
#ifndef NDEBUG
    if (vertexFiring >= getRV(vertex)) {
        throwSpiderException("invalid vertex firing.");
    }
#endif
    return taskIxRegister_.at(vertex->ix())[vertexFiring];
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
            const auto &parentParam = parentParams[param->parent()->ix()];
            newParam = spider::make_shared<pisdf::Param, StackID::PISDF>(param->name(),
                                                                         parentParam->value(parentParams));
        } else {
            newParam = spider::make_shared<pisdf::Param, StackID::PISDF>(*param);
        }
        newParam->setIx(param->ix());
        return newParam;
    }
    return param;
}

spider::srless::ExecDependency spider::srless::FiringHandler::computeExecDependency(const pisdf::Edge *edge,
                                                                                    int64_t lowerCons,
                                                                                    int64_t upperCons) const {
    const auto *delay = edge->delay();
    const auto srcRate = edge->sourceRateExpression().evaluate(params_);
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

spider::srless::ExecDependency
spider::srless::FiringHandler::computeCons(const spider::pisdf::Edge *edge, u32 firing) const {
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

const spider::srless::FiringHandler *
spider::srless::FiringHandler::getChildFiring(const pisdf::Graph *subgraph, u32 firing) const {
#ifndef NDEBUG
    if (subgraph->graph() != parent_->graph()) {
        throwSpiderException("subgraph does not belong to this graph.");
    }
#endif
    return &(children_[subgraph->subIx()]->firings()[firing]);
}

