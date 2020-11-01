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
#include <graphs-tools/numerical/detail/dependenciesImpl.h>

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
    brvArray_ = spider::make_unique(make_n<u32, StackID::TRANSFO>(graph->vertexCount(), UINT32_MAX));
    subgraphHandlers_ = spider::make_unique(make_n<GraphHandler *, StackID::TRANSFO>(graph->subgraphCount(), nullptr));
    ratesArray_ = spider::make_unique(make_n<EdgeRate, StackID::TRANSFO>(graph->edgeCount(), { 0, 0 }));
    taskIxRegister_ = spider::make_unique(make_n<u32 *, StackID::TRANSFO>(graph->vertexCount(), nullptr));
    edgeAllocArray_ = spider::make_unique(make_n<FifoAlloc *, StackID::TRANSFO>(graph->edgeCount(), nullptr));
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
    for (const auto &edge : parent_->graph()->edges()) {
        deallocate(edgeAllocArray_[edge->ix()]);
    }
}

void spider::pisdf::GraphFiring::resolveBRV() {
    if (resolved_) {
        return;
    }
    /* == Compute BRV == */
    spider::brv::compute(parent_->graph(), params_);
    /* == Save RV values into the array == */
    for (const auto &vertex : parent_->graph()->vertices()) {
        updateFromRV(vertex.get(), vertex->repetitionValue());
    }
    /* == creates subgraph handlers == */
    createOrUpdateSubgraphHandlers();
    /* == Save the rates == */
    for (const auto &edge : parent_->graph()->edges()) {
        const auto ix = edge->ix();
        ratesArray_[ix].srcRate_ = edge->sourceRateValue();
        ratesArray_[ix].snkRate_ = edge->sinkRateValue();
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
    for (const auto &vertex : parent_->graph()->vertices()) {
        updateFromRV(vertex.get(), srcFiring->brvArray_[vertex->ix()]);
    }
    /* == creates subgraph handlers == */
    createOrUpdateSubgraphHandlers();
    /* == Copy rates == */
    memcpy(ratesArray_.get(), srcFiring->ratesArray_.get(), parent_->graph()->edgeCount() * sizeof(EdgeRate));
    resolved_ = true;
}

void spider::pisdf::GraphFiring::clear() {
    for (const auto &vertex : parent_->graph()->vertices()) {
        const auto ix = vertex->ix();
        const auto rvValue = brvArray_[ix];
        if (rvValue != UINT32_MAX) {
            std::fill(taskIxRegister_[ix], taskIxRegister_[ix] + rvValue, UINT32_MAX);
            for (const auto *edge : vertex->outputEdges()) {
                for (u32 k = 0; k < rvValue; ++k) {
                    setEdgeAddress(SIZE_MAX, edge, k);
                    setEdgeOffset(0, edge, k);
                }
            }
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

spider::vector<spider::pisdf::DependencyIterator>
spider::pisdf::GraphFiring::computeExecDependencies(const Vertex *vertex, u32 firing) const {
    auto result = factory::vector<DependencyIterator>(StackID::SCHEDULE);
    if (vertex->inputEdgeCount()) {
        spider::reserve(result, vertex->inputEdgeCount());
        for (const auto *edge : vertex->inputEdges()) {
            result.emplace_back(computeExecDependency(vertex, firing, edge->sinkPortIx()));
        }
    }
    return result;
}

spider::pisdf::DependencyIterator
spider::pisdf::GraphFiring::computeExecDependency(const Vertex *vertex, u32 firing, size_t edgeIx, i32 *count) const {
#ifndef NDEBUG
    if (!vertex) {
        throwNullptrException();
    }
#endif
    const auto *edge = vertex->inputEdge(edgeIx);
    const auto snkRate = getSnkRate(edge);
    if (!snkRate) {
        return DependencyIterator{{{ nullptr, nullptr, 0, 0, 0, 0, 0, 0 }}};
    }
    auto result = factory::vector<DependencyInfo>(StackID::TRANSFO);
    spider::reserve(result, 20);
    auto depCount = pisdf::detail::computeExecDependency(edge, snkRate * firing, snkRate * (firing + 1) - 1, this, &result);
    if (count) {
        *count = depCount;
    }
    return DependencyIterator{ std::move(result) };
}

spider::vector<spider::pisdf::DependencyIterator>
spider::pisdf::GraphFiring::computeConsDependencies(const Vertex *vertex, u32 firing) const {
    auto result = factory::vector<pisdf::DependencyIterator>(StackID::SCHEDULE);
    if (vertex->outputEdgeCount()) {
        spider::reserve(result, vertex->outputEdgeCount());
        for (const auto *edge : vertex->outputEdges()) {
            result.emplace_back(computeConsDependency(vertex, firing, edge->sourcePortIx()));
        }
    }
    return result;
}

spider::pisdf::DependencyIterator
spider::pisdf::GraphFiring::computeConsDependency(const Vertex *vertex, u32 firing, size_t edgeIx, i32 *count) const {
#ifndef NDEBUG
    if (!vertex) {
        throwNullptrException();
    }
#endif
    const auto *edge = vertex->outputEdge(edgeIx);
    const auto srcRate = getSrcRate(edge);
    if (!srcRate) {
        return DependencyIterator{{{ nullptr, nullptr, 0, 0, 0, 0, 0, 0 }}};
    }
    auto result = factory::vector<DependencyInfo>(StackID::TRANSFO);
    auto depCount = detail::computeConsDependency(edge, srcRate * firing, srcRate * (firing + 1) - 1, this, &result);
    if (count) {
        *count = depCount;
    }
    return DependencyIterator{ std::move(result) };
}

spider::array_handle<spider::pisdf::GraphHandler *> spider::pisdf::GraphFiring::subgraphFirings() const {
    return make_handle(subgraphHandlers_.get(), parent_->graph()->subgraphCount());
}

spider::array_handle<spider::pisdf::GraphHandler *> spider::pisdf::GraphFiring::subgraphHandlers() {
    return make_handle(subgraphHandlers_.get(), parent_->graph()->subgraphCount());
}

int64_t spider::pisdf::GraphFiring::getSrcRate(const Edge *edge) const {
#ifndef NDEBUG
    if (edge->graph() != parent_->graph()) {
        throwSpiderException("edge does not belong to this graph.");
    }
#endif
    // TODO:: add possibility to switch off this optim with compiler flag
    return ratesArray_[edge->ix()].srcRate_;
}

int64_t spider::pisdf::GraphFiring::getSnkRate(const Edge *edge) const {
#ifndef NDEBUG
    if (edge->graph() != parent_->graph()) {
        throwSpiderException("edge does not belong to this graph.");
    }
#endif
    // TODO:: add possibility to switch off this optim with compiler flag
    return ratesArray_[edge->ix()].snkRate_;
}

u32 spider::pisdf::GraphFiring::getRV(const Vertex *vertex) const {
#ifndef NDEBUG
    if (vertex->graph() != parent_->graph()) {
        throwSpiderException("vertex does not belong to the correct graph.");
    }
#endif
    if (vertex->subtype() == pisdf::VertexType::INPUT || vertex->subtype() == pisdf::VertexType::OUTPUT) {
        return 1;
    }
    return brvArray_[vertex->ix()];
}

u32 spider::pisdf::GraphFiring::getTaskIx(const Vertex *vertex, u32 firing) const {
#ifndef NDEBUG
    if (firing >= getRV(vertex)) {
        throwSpiderException("invalid vertex firing.");
    }
#endif
    return taskIxRegister_[vertex->ix()][firing];
}

const spider::pisdf::GraphFiring *
spider::pisdf::GraphFiring::getSubgraphGraphFiring(const Graph *subgraph, u32 firing) const {
#ifndef NDEBUG
    if (subgraph->graph() != parent_->graph()) {
        throwSpiderException("subgraph does not belong to this graph.");
    }
#endif
    return subgraphHandlers_[subgraph->subIx()]->firings()[firing];
}

const spider::pisdf::Vertex *spider::pisdf::GraphFiring::vertex(size_t ix) const {
    return parent_->graph()->vertex(ix);
}

spider::pisdf::Vertex *spider::pisdf::GraphFiring::vertex(size_t ix) {
    return parent_->graph()->vertex(ix);
}

size_t spider::pisdf::GraphFiring::getEdgeAddress(const Edge *edge, u32 firing) const {
    return edgeAllocArray_[edge->ix()][firing].address_;
}

u32 spider::pisdf::GraphFiring::getEdgeOffset(const Edge *edge, u32 firing) const {
    return edgeAllocArray_[edge->ix()][firing].offset_;
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

void spider::pisdf::GraphFiring::setTaskIx(const pisdf::Vertex *vertex, u32 firing, u32 taskIx) {
#ifndef NDEBUG
    if (firing >= getRV(vertex)) {
        throwSpiderException("invalid vertex firing.");
    }
#endif
    taskIxRegister_[vertex->ix()][firing] = taskIx;
}

void spider::pisdf::GraphFiring::setEdgeAddress(size_t value, const pisdf::Edge *edge, u32 firing) {
    edgeAllocArray_[edge->ix()][firing].address_ = value;
}

void spider::pisdf::GraphFiring::setEdgeOffset(u32 value, const pisdf::Edge *edge, u32 firing) {
    edgeAllocArray_[edge->ix()][firing].offset_ = value;
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

void spider::pisdf::GraphFiring::updateFromRV(const pisdf::Vertex *vertex, u32 rvValue) {
    const auto ix = vertex->ix();
    if (brvArray_[ix] != rvValue) {
        brvArray_[ix] = rvValue;
        deallocate(taskIxRegister_[ix]);
        taskIxRegister_[ix] = spider::make_n<u32, StackID::TRANSFO>(rvValue, UINT32_MAX);
        for (const auto *edge : vertex->outputEdges()) {
            deallocate(edgeAllocArray_[edge->ix()]);
            edgeAllocArray_[edge->ix()] = spider::make_n<FifoAlloc, StackID::TRANSFO>(rvValue, { SIZE_MAX, 0 });
        }
    } else {
        /* == reset values == */
        std::fill(taskIxRegister_[ix], taskIxRegister_[ix] + rvValue, UINT32_MAX);
    }
}

void spider::pisdf::GraphFiring::createOrUpdateSubgraphHandlers() {
    for (const auto &subgraph : parent_->graph()->subgraphs()) {
        const auto ix = subgraph->ix();
        const auto rvValue = brvArray_[ix];
        auto &currentGraphHandler = subgraphHandlers_[subgraph->subIx()];
        if (!currentGraphHandler || (rvValue != currentGraphHandler->repetitionCount())) {
            destroy(currentGraphHandler);
            currentGraphHandler = spider::make<GraphHandler>(subgraph, subgraph->params(), rvValue, this);
        } else {
            /* == Resolve every child == */
            currentGraphHandler->resolveFirings();
        }
    }
}
