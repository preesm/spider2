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

#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>
#include <graphs-tools/numerical/dependencies.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs-tools/helper/pisdf-helper.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::pisdf::GraphFiring::GraphFiring(const GraphHandler *parent,
                                        const spider::vector<std::shared_ptr<pisdf::Param>> &params,
                                        u32 firing) :
        params_{ factory::vector<std::shared_ptr<pisdf::Param>>(StackID::TRANSFO) },
        parent_{ parent },
        firing_{ firing },
        resolved_{ false } {
    if (!parent) {
        throwNullptrException();
    }
    const auto *graph = parent->graph();
    brv_ = spider::make_unique(make_n<u32, StackID::TRANSFO>(graph->vertexCount(), UINT32_MAX));
    taskIxRegister_ = spider::make_unique(make_n<u32 *, StackID::TRANSFO>(graph->vertexCount(), nullptr));
    subgraphHandlers_ = spider::make_unique(make_n<GraphHandler *, StackID::TRANSFO>(graph->subgraphCount(), nullptr));
    rates_ = spider::make_unique(make_n<EdgeRate, StackID::TRANSFO>(graph->edgeCount(), { 0, 0 }));
    edgeAllocAddress_ = spider::make_unique(make_n<size_t, StackID::SCHEDULE>(graph->edgeCount(), SIZE_MAX));
    /* == copy parameters == */
    params_.reserve(params.size());
    dynamicParamCount_ = 0;
    for (const auto &param : params) {
        dynamicParamCount_ += param->type() == pisdf::ParamType::DYNAMIC;
        params_.emplace_back(copyParameter(param));
    }
}

spider::pisdf::GraphFiring::~GraphFiring() {
    for (auto &ptr : make_handle(taskIxRegister_.get(), parent_->graph()->vertexCount())) {
        deallocate(ptr);
    }
    for (auto &child : subgraphHandlers()) {
        destroy(child);
    }
}

void spider::pisdf::GraphFiring::registerTaskIx(const pisdf::Vertex *vertex, u32 firing, u32 taskIx) {
#ifndef NDEBUG
    if (firing >= getRV(vertex)) {
        throwSpiderException("invalid vertex firing.");
    }
#endif
    taskIxRegister_.get()[vertex->ix()][firing] = taskIx;
}

void spider::pisdf::GraphFiring::resolveBRV() {
    if (resolved_) {
        return;
    }
    /* == Compute BRV == */
    spider::brv::compute(parent_->graph(), params_);
    /* == Save RV values into the array == */
    for (const auto &vertex : parent_->graph()->vertices()) {
        const auto ix = vertex->ix();
        const auto rvValue = vertex->repetitionValue();
        if (brv_.get()[ix] != rvValue) {
            brv_.get()[ix] = rvValue;
            deallocate(taskIxRegister_.get()[ix]);
            taskIxRegister_.get()[ix] = spider::make_n<u32, StackID::TRANSFO>(rvValue, UINT32_MAX);
        } else {
            /* == reset values == */
            std::fill(taskIxRegister_.get()[ix], taskIxRegister_.get()[ix] + rvValue, UINT32_MAX);
        }
    }
    std::fill(edgeAllocAddress_.get(), edgeAllocAddress_.get() + parent_->graph()->edgeCount(), SIZE_MAX);
    /* == creates subgraph handlers == */
    createOrUpdateSubgraphHandlers();
    /* == Save the rates == */
    for (const auto &edge : parent_->graph()->edges()) {
        const auto ix = edge->ix();
        rates_.get()[ix].srcRate_ = edge->sourceRateValue();
        rates_.get()[ix].snkRate_ = edge->sinkRateValue();
    }
    resolved_ = true;
}

void spider::pisdf::GraphFiring::apply(const GraphFiring *srcFiring) {
#ifndef NDEBUG
    if (!srcFiring) {
        throwNullptrException();
    } else if (srcFiring->parent_ != parent_) {
        throwSpiderException("expected a firing from the same parent_.");
    } else if (!srcFiring->resolved_) {
        throwSpiderException("expected a resolved firing.");
    }
#endif
    if (resolved_ || srcFiring == this) {
        return;
    }
    const auto *srcFiringBRV = srcFiring->brv_.get();
    auto *thisBRV = brv_.get();
    for (size_t ix = 0; ix < parent_->graph()->vertexCount(); ++ix) {
        const auto rvValue = srcFiringBRV[ix];
        if (thisBRV[ix] != rvValue) {
            thisBRV[ix] = rvValue;
            deallocate(taskIxRegister_.get()[ix]);
            taskIxRegister_.get()[ix] = spider::make_n<u32, StackID::TRANSFO>(rvValue, UINT32_MAX);
        } else {
            /* == reset values == */
            std::fill(taskIxRegister_.get()[ix], taskIxRegister_.get()[ix] + rvValue, UINT32_MAX);
        }
    }
    std::fill(edgeAllocAddress_.get(), edgeAllocAddress_.get() + parent_->graph()->edgeCount(), SIZE_MAX);
    /* == creates subgraph handlers == */
    createOrUpdateSubgraphHandlers();
    /* == Copy rates == */
    memcpy(rates_.get(), srcFiring->rates_.get(), parent_->graph()->edgeCount() * sizeof(EdgeRate));
    resolved_ = true;
}

void spider::pisdf::GraphFiring::clear() {
    for (const auto &vertex : parent_->graph()->vertices()) {
        const auto ix = vertex->ix();
        const auto rvValue = brv_.get()[ix];
        if (rvValue != UINT32_MAX) {
            std::fill(taskIxRegister_.get()[ix], taskIxRegister_.get()[ix] + brv_.get()[ix], UINT32_MAX);
        }
    }
    for (auto &graphHandler : subgraphHandlers()) {
        if (graphHandler) {
            graphHandler->clear();
        }
    }
    paramResolvedCount_ = 0;
    resolved_ = parent_->isStatic();
}

spider::array_handle<spider::pisdf::GraphHandler *> spider::pisdf::GraphFiring::subgraphFirings() const {
    return make_handle(subgraphHandlers_.get(), parent_->graph()->subgraphCount());
}

spider::array_handle<spider::pisdf::GraphHandler *> spider::pisdf::GraphFiring::subgraphHandlers() {
    return make_handle(subgraphHandlers_.get(), parent_->graph()->subgraphCount());
}

int64_t spider::pisdf::GraphFiring::getSourceRate(const pisdf::Edge *edge) const {
#ifndef NDEBUG
    if (edge->graph() != parent_->graph()) {
        throwSpiderException("edge does not belong to this graph.");
    }
#endif
    // TODO:: add possibility to switch off this optim with compiler flag
    return rates_.get()[edge->ix()].srcRate_;
}

int64_t spider::pisdf::GraphFiring::getSinkRate(const pisdf::Edge *edge) const {
#ifndef NDEBUG
    if (edge->graph() != parent_->graph()) {
        throwSpiderException("edge does not belong to this graph.");
    }
#endif
    // TODO:: add possibility to switch off this optim with compiler flag
    return rates_.get()[edge->ix()].snkRate_;
}

u32 spider::pisdf::GraphFiring::getRV(const spider::pisdf::Vertex *vertex) const {
#ifndef NDEBUG
    if (vertex->graph() != parent_->graph()) {
        throwSpiderException("vertex does not belong to the correct graph.");
    }
#endif
    if (vertex->subtype() == pisdf::VertexType::INPUT || vertex->subtype() == pisdf::VertexType::OUTPUT) {
        return 1;
    }
    return brv_.get()[vertex->ix()];
}

u32 spider::pisdf::GraphFiring::getTaskIx(const spider::pisdf::Vertex *vertex, u32 firing) const {
#ifndef NDEBUG
    if (firing >= getRV(vertex)) {
        throwSpiderException("invalid vertex firing.");
    }
#endif
    return taskIxRegister_.get()[vertex->ix()][firing];
}

const spider::pisdf::GraphFiring *
spider::pisdf::GraphFiring::getSubgraphGraphFiring(const pisdf::Graph *subgraph, u32 firing) const {
#ifndef NDEBUG
    if (subgraph->graph() != parent_->graph()) {
        throwSpiderException("subgraph does not belong to this graph.");
    }
#endif
    return subgraphHandlers_.get()[subgraph->subIx()]->firings()[firing];
}

const spider::vector<std::shared_ptr<spider::pisdf::Param>> &spider::pisdf::GraphFiring::getParams() const {
    return params_;
}

void spider::pisdf::GraphFiring::setParamValue(size_t ix, int64_t value) {
    spider::get_at(params_, ix)->setValue(value);
    paramResolvedCount_++;
    if (paramResolvedCount_ == dynamicParamCount_) {
        /* == Resolve dynamic dependent parameters == */
        for (auto &param : params_) {
            if (param->type() == pisdf::ParamType::DYNAMIC_DEPENDANT) {
                param->value(params_);
            }
        }
        for (auto *subHandler : subgraphHandlers()) {
            subHandler->resolveFirings();
        }
    }
}

size_t spider::pisdf::GraphFiring::getEdgeAlloc(const pisdf::Edge *edge) const {
    return edgeAllocAddress_.get()[edge->ix()];
}

const spider::pisdf::Vertex *spider::pisdf::GraphFiring::vertex(size_t ix) const {
    return parent_->graph()->vertex(ix);
}

spider::pisdf::Vertex *spider::pisdf::GraphFiring::vertex(size_t ix) {
    return parent_->graph()->vertex(ix);
}

void spider::pisdf::GraphFiring::registerEdgeAlloc(size_t value, const pisdf::Edge *edge) {
    edgeAllocAddress_.get()[edge->ix()] = value;
}

/* === Private method(s) implementation === */

std::shared_ptr<spider::pisdf::Param>
spider::pisdf::GraphFiring::copyParameter(const std::shared_ptr<pisdf::Param> &param) {
    if (param->type() == pisdf::ParamType::DYNAMIC) {
        return spider::make_shared<pisdf::Param, StackID::PISDF>(*param);
    } else if (param->type() == pisdf::ParamType::INHERITED) {
        const auto *parentHandler = parent_->handler();
        auto paramParentIx = param->parent() ? param->parent()->ix() : throwNullptrException();
        const auto *parent = &parentHandler->params_[paramParentIx];
        while (parent && ((*parent)->type() == pisdf::ParamType::INHERITED)) {
            parentHandler = parentHandler->parent_->handler();
            paramParentIx = (*parent)->parent() ? (*parent)->parent()->ix() : throwNullptrException();
            parent = &parentHandler->params_[paramParentIx];
        }
        auto newParam = spider::make_shared<pisdf::Param, StackID::PISDF>(param->name(), *parent);
        newParam->setIx(param->ix());
        return newParam;
    }
    return param;
}

void spider::pisdf::GraphFiring::createOrUpdateSubgraphHandlers() {
    for (const auto &subgraph : parent_->graph()->subgraphs()) {
        const auto ix = subgraph->ix();
        const auto rvValue = brv_.get()[ix];
        auto &currentGraphHandler = subgraphHandlers_.get()[subgraph->subIx()];
        if (!currentGraphHandler || (rvValue != currentGraphHandler->repetitionCount())) {
            destroy(currentGraphHandler);
            currentGraphHandler = spider::make<GraphHandler>(subgraph, subgraph->params(), rvValue, this);
        } else {
            /* == Resolve every child == */
            currentGraphHandler->resolveFirings();
        }
    }
}
