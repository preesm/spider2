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
#include <graphs-tools/numerical/dependencies.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs-tools/helper/pisdf-helper.h>

/* === Static function === */

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

//spider::srless::ExecDependency
//spider::srless::FiringHandler::computeConsDependenciesByEdge(const pisdf::Vertex *vertex,
//                                                             u32 firing,
//                                                             u32 edgeIx) const {
//    const auto *edge = vertex->outputEdge(edgeIx);
//    const auto srcRate = edge->sourceRateExpression().evaluate(params_);
//    if (!srcRate) {
//        return { detail::dummyInfo, detail::dummyInfo };
//    }
//    if (edge->sink()->subtype() == pisdf::VertexType::DELAY) {
//        const auto *delay = edge->sink()->convertTo<pisdf::DelayVertex>()->delay();
//        const auto lowerProd = srcRate * firing - delay->value();
//        const auto upperProd = srcRate * (firing + 1u) - delay->value();
//        return computeConsDependency(delay->edge(), lowerProd, upperProd);
//    } else {
//        return computeConsDependency(edge, srcRate * firing, srcRate * (firing + 1));
//    }
//}

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
        if (!pisdf::isInterfaceTransparent(interface.get(), this)) {
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
    if (vertex->subtype() == pisdf::VertexType::INPUT) {
        return taskIxRegister_.at(vertex->ix() + parent_->graph()->vertexCount())[0u];
    }
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

//spider::srless::ExecDependency spider::srless::FiringHandler::computeConsDependency(const pisdf::Edge *edge,
//                                                                                    int64_t lowerProd,
//                                                                                    int64_t upperProd) const {
//    const auto *delay = edge->delay();
//    const auto snkRate = edge->sinkRateExpression().evaluate(params_);
//    const auto getRate = delay ? delay->getterRate(params_) : 0;
//    const auto snkTotRate = snkRate * getRV(edge->sink());
//    const auto delayValue = delay ? delay->value() : 0;
//    auto result = ExecDependency{ detail::dummyInfo, detail::dummyInfo };
//    if (lowerProd + delayValue >= snkTotRate) {
//#ifndef NDEBUG
//        if (!delay) {
//            throwNullptrException();
//        }
//#endif
//        /* == getter only == */
//        result.first_.vertex_ = delay->getter();
//        result.first_.handler_ = this;
//        result.first_.rate_ = static_cast<size_t>(getRate);
//        result.first_.edgeIx_ = static_cast<u32>(delay->getterPortIx());
//        result.first_.memoryStart_ = static_cast<u32>((lowerProd + delayValue - snkTotRate) % getRate);
//        result.first_.memoryEnd_ = static_cast<u32>((upperProd + delayValue - snkTotRate - 1) % getRate);
//        result.first_.firingStart_ = static_cast<u32>(math::floorDiv(lowerProd + delayValue - snkTotRate, getRate));
//        result.first_.firingEnd_ = static_cast<u32>(math::floorDiv(upperProd + delayValue - 1 - snkTotRate, getRate));
//    } else if (upperProd + delayValue <= snkTotRate) {
//        /* == sink only == */
//        result.first_.vertex_ = edge->sink();
//        result.first_.handler_ = this;
//        result.first_.rate_ = static_cast<size_t>(snkRate);
//        result.first_.edgeIx_ = static_cast<u32>(edge->sinkPortIx());
//        result.first_.memoryStart_ = static_cast<u32>((lowerProd + delayValue) % snkRate);
//        result.first_.memoryEnd_ = static_cast<u32>((upperProd + delayValue - 1) % snkRate);
//        result.first_.firingStart_ = static_cast<u32>(math::floorDiv(lowerProd + delayValue, snkRate));
//        result.first_.firingEnd_ = static_cast<u32>(math::floorDiv(upperProd + delayValue - 1, snkRate));
//    } else {
//#ifndef NDEBUG
//        if (!delay) {
//            throwNullptrException();
//        }
//#endif
//        /* == sink + getter == */
//        /* == snk info == */
//        result.first_.vertex_ = edge->sink();
//        result.first_.handler_ = this;
//        result.first_.rate_ = static_cast<size_t>(snkRate);
//        result.first_.edgeIx_ = static_cast<u32>(edge->sinkPortIx());
//        result.first_.memoryStart_ = static_cast<u32>((lowerProd + delayValue) % snkRate);
//        result.first_.memoryEnd_ = static_cast<u32>(snkRate - 1);
//        result.first_.firingStart_ = static_cast<u32>(math::floorDiv(lowerProd + delayValue, snkRate));
//        result.first_.firingEnd_ = getRV(edge->sink()) - 1;
//        /* == get info == */
//        result.second_.vertex_ = delay->getter();
//        result.second_.handler_ = this;
//        result.second_.rate_ = static_cast<size_t>(getRate);
//        result.second_.edgeIx_ = static_cast<u32>(delay->getterPortIx());
//        result.second_.memoryStart_ = 0u;
//        result.second_.memoryEnd_ = static_cast<u32>((upperProd + delayValue - snkTotRate - 1) % getRate);
//        result.second_.firingStart_ = 0u;
//        result.second_.firingEnd_ = static_cast<u32>(math::floorDiv(upperProd + delayValue - 1 - snkTotRate, getRate));
//    }
//    return result;
//}