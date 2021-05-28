/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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
#include <graphs-tools/transformation/pisdf/GraphAlloc.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/DelayVertex.h>

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
    ratesArray_ = spider::make_unique(make_n<EdgeRate, StackID::TRANSFO>(graph->edgeCount(), { 0, 0 }));
    /* == copy parameters == */
    params_.reserve(params.size());
    dynamicParamCount_ = 0;
    for (const auto &param : params) {
        dynamicParamCount_ += param->type() == pisdf::ParamType::DYNAMIC;
        params_.emplace_back(copyParameter(param));
    }
    depsCountArray_ = spider::make_unique(make_n<u32 *, StackID::TRANSFO>(graph->vertexCount(), nullptr));
    subgraphHandlers_ = spider::make_unique(make_n<GraphHandler *, StackID::TRANSFO>(graph->subgraphCount(), nullptr));
    alloc_ = spider::make_unique(make<GraphAlloc, StackID::SCHEDULE>(parent->graph()));
}

spider::pisdf::GraphFiring::~GraphFiring() {
    for (const auto &vertex : parent_->graph()->vertices()) {
        deallocate(depsCountArray_[vertex->ix()]);
    }
    for (auto &child : subgraphHandlers()) {
        destroy(child);
    }
    alloc_->clear(parent_->graph());
}

void spider::pisdf::GraphFiring::resolveBRV() {
    if (resolved_) {
        return;
    }
    if (parent_->isStatic() && firing_) {
        return;
    }
    resolveDynamicDependentParams();
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
    /* == do other firings == */
    if (parent_->isStatic()) {
        const auto parentRV = parent_->repetitionCount();
        const auto vertexCount = parent_->graph()->vertexCount();
        const auto edgeCount = parent_->graph()->edgeCount();
        for (u32 k = 1; k < parentRV; ++k) {
            auto *graphFiring = parent_->firing(k);
            memcpy(graphFiring->brvArray_.get(), brvArray_.get(), vertexCount * sizeof(u32));
            memcpy(graphFiring->ratesArray_.get(), ratesArray_.get(), edgeCount * sizeof(EdgeRate));
            graphFiring->resolveDynamicDependentParams();
            graphFiring->createOrUpdateSubgraphHandlers();
            graphFiring->resolved_ = true;
        }
    }
}

void spider::pisdf::GraphFiring::clear() {
    const auto *brv = brvArray_.get();
    alloc_->reset(parent_->graph(), brv);
    for (auto &graphHandler : subgraphHandlers()) {
        if (graphHandler) {
            graphHandler->clear();
        }
    }
    paramResolvedCount_ = 0;
    resolved_ = parent_->isStatic();
}

spider::array_view<spider::pisdf::GraphHandler *> spider::pisdf::GraphFiring::subgraphFirings() const {
    return make_view(subgraphHandlers_.get(), parent_->graph()->subgraphCount());
}

spider::array_view<spider::pisdf::GraphHandler *> spider::pisdf::GraphFiring::subgraphHandlers() {
    return make_view(subgraphHandlers_.get(), parent_->graph()->subgraphCount());
}

int64_t spider::pisdf::GraphFiring::getSrcRate(const Edge *edge) const {
#ifndef NDEBUG
    if (edge->graph() != parent_->graph()) {
        throwSpiderException("edge does not belong to this graph.");
    }
#endif
    return ratesArray_[edge->ix()].srcRate_;
}

int64_t spider::pisdf::GraphFiring::getSnkRate(const Edge *edge) const {
#ifndef NDEBUG
    if (edge->graph() != parent_->graph()) {
        throwSpiderException("edge does not belong to this graph.");
    }
#endif
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

const spider::pisdf::GraphFiring *
spider::pisdf::GraphFiring::getSubgraphGraphFiring(const Graph *subgraph, u32 firing) const {
#ifndef NDEBUG
    if (subgraph->graph() != parent_->graph()) {
        throwSpiderException("subgraph does not belong to this graph.");
    }
#endif
    return subgraphHandlers_[subgraph->subIx()]->firing(firing);
}

const spider::vector<std::shared_ptr<spider::pisdf::Param>> &spider::pisdf::GraphFiring::getParams() const {
    return params_;
}

const spider::pisdf::Vertex *spider::pisdf::GraphFiring::vertex(size_t ix) const {
    return parent_->graph()->vertex(ix);
}

spider::pisdf::Vertex *spider::pisdf::GraphFiring::vertex(size_t ix) {
    return parent_->graph()->vertex(ix);
}

spider::sched::PiSDFTask *spider::pisdf::GraphFiring::getTask(const Vertex *vertex) const {
#ifndef NDEBUG
    if (vertex->graph() != parent_->graph()) {
        throwSpiderException("vertex do not belong to this graph.");
    }
#endif
    return alloc_->getTask(vertex);
}

u32 spider::pisdf::GraphFiring::getTaskIx(const Vertex *vertex, u32 firing) const {
    return alloc_->getTaskIx(static_cast<u32>(vertex->ix()), firing);
}

u32 spider::pisdf::GraphFiring::getTaskIx(u32 vertexIx, u32 firing) const {
    return alloc_->getTaskIx(vertexIx, firing);
}

const u32 *spider::pisdf::GraphFiring::getTaskIndexes(const Vertex *vertex) const {
    return alloc_->getTaskIndexes(static_cast<u32>(vertex->ix()));
}

const u32 *spider::pisdf::GraphFiring::getTaskIndexes(u32 vertexIx) const {
    return alloc_->getTaskIndexes(vertexIx);
}

size_t spider::pisdf::GraphFiring::getEdgeAddress(const Edge *edge, u32 firing) const {
    const auto type = edge->source()->subtype();
    if (type != VertexType::FORK && type != VertexType::DUPLICATE) {
        return alloc_->getEdgeAddress(edge, 0) + static_cast<size_t>(getSrcRate(edge) * firing);
    }
    return alloc_->getEdgeAddress(edge, firing);
}

u32 spider::pisdf::GraphFiring::getEdgeOffset(const Edge *edge, u32 firing) const {
    const auto type = edge->source()->subtype();
    if (type != VertexType::FORK && type != VertexType::DUPLICATE) {
        return alloc_->getEdgeOffset(edge, 0);
    }
    return alloc_->getEdgeOffset(edge, firing);
}

u32 spider::pisdf::GraphFiring::getEdgeDepCount(const Vertex *vertex, const Edge *edge, u32 firing) const {
    const auto offset = firing * vertex->inputEdgeCount();
    return depsCountArray_[vertex->ix()][offset + edge->sinkPortIx()];
}

void spider::pisdf::GraphFiring::setParamValue(size_t ix, int64_t value) {
    spider::get_at(params_, ix)->setValue(value);
    paramResolvedCount_++;
    if (paramResolvedCount_ == dynamicParamCount_) {
        resolveDynamicDependentParams();
        for (auto *subHandler : subgraphHandlers()) {
            subHandler->resolveFirings();
        }
    }
}

void spider::pisdf::GraphFiring::setTaskIx(const Vertex *vertex, u32 firing, u32 taskIx) {
    alloc_->setTaskIx(vertex, firing, taskIx);
}

void spider::pisdf::GraphFiring::setEdgeAddress(size_t value, const Edge *edge, u32 firing) {
    const auto type = edge->source()->subtype();
    if (type != VertexType::FORK && type != VertexType::DUPLICATE) {
        if (alloc_->getEdgeAddress(edge, 0) == SIZE_MAX) {
            alloc_->setEdgeAddress(value, edge, 0);
        }
    } else {
        alloc_->setEdgeAddress(value, edge, firing);
    }
}

void spider::pisdf::GraphFiring::setEdgeOffset(u32 value, const Edge *edge, u32 firing) {
    const auto type = edge->source()->subtype();
    if (type != VertexType::FORK && type != VertexType::DUPLICATE) {
        alloc_->setEdgeOffset(value, edge, 0);
    } else {
        alloc_->setEdgeOffset(value, edge, firing);
    }
}

void spider::pisdf::GraphFiring::setEdgeDepCount(const Vertex *vertex, const Edge *edge, u32 firing, u32 value) {
    const auto offset = firing * vertex->inputEdgeCount();
    if (vertex->subtype() != pisdf::VertexType::EXTERN_OUT) {
        depsCountArray_[vertex->ix()][offset + edge->sinkPortIx()] = value;
    } else {
        depsCountArray_[vertex->ix()][offset + edge->sinkPortIx()] = 1;
    }
}

/* === Private method(s) implementation === */

void spider::pisdf::GraphFiring::resolveDynamicDependentParams() const {
    /* == Resolve dynamic dependent parameters == */
    for (auto &param : params_) {
        if (param->type() == pisdf::ParamType::DYNAMIC_DEPENDANT) {
            param->value(params_);
        }
    }
}

std::shared_ptr<spider::pisdf::Param>
spider::pisdf::GraphFiring::copyParameter(const std::shared_ptr<pisdf::Param> &param) {
    if (param->type() == ParamType::DYNAMIC || param->type() == ParamType::DYNAMIC_DEPENDANT) {
        return spider::make_shared<Param, StackID::PISDF>(*param);
    } else if (param->type() == ParamType::INHERITED) {
        const auto *parentHandler = parent_->base();
        auto paramParentIx = param->parent() ? param->parent()->ix() : throwNullptrException();
        const auto *parent = &parentHandler->getParams()[paramParentIx];
        while (parent && ((*parent)->type() == ParamType::INHERITED)) {
            parentHandler = parentHandler->parent_->base();
            paramParentIx = (*parent)->parent() ? (*parent)->parent()->ix() : throwNullptrException();
            parent = &parentHandler->getParams()[paramParentIx];
        }
        auto newParam = spider::make_shared<Param, StackID::PISDF>(param->name(), *parent);
        newParam->setIx(param->ix());
        return newParam;
    }
    return param;
}

void spider::pisdf::GraphFiring::updateFromRV(const pisdf::Vertex *vertex, u32 rv) {
    const auto ix = vertex->ix();
    const auto count = rv * vertex->inputEdgeCount();
    if (brvArray_[ix] != rv) {
        brvArray_[ix] = rv;
        alloc_->initialize(this, vertex, rv);
        deallocate(depsCountArray_[ix]);
        depsCountArray_[ix] = make_n<u32, StackID::TRANSFO>(count, 0);
        if (parent_->isStatic()) {
            const auto parentRV = parent_->repetitionCount();
            for (u32 k = 1; k < parentRV; ++k) {
                auto *graphFiring = parent_->firing(k);
                graphFiring->alloc_->initialize(graphFiring, vertex, rv);
                deallocate(graphFiring->depsCountArray_[ix]);
                graphFiring->depsCountArray_[ix] = make_n<u32, StackID::TRANSFO>(count, 0);
            }
        }
    } else {
        /* == reset values == */
        std::fill(depsCountArray_[ix], depsCountArray_[ix] + count, 0);
        alloc_->reset(vertex, rv);
        if (parent_->isStatic()) {
            const auto parentRV = parent_->repetitionCount();
            for (u32 k = 1; k < parentRV; ++k) {
                auto *graphFiring = parent_->firing(k);
                graphFiring->alloc_->reset(vertex, rv);
                std::fill(graphFiring->depsCountArray_[ix], graphFiring->depsCountArray_[ix] + count, 0);
            }
        }
    }
}

void spider::pisdf::GraphFiring::createOrUpdateSubgraphHandlers() {
    for (const auto &subgraph : parent_->graph()->subgraphs()) {
        const auto rv = getRV(subgraph);
        auto &currentGraphHandler = subgraphHandlers_[subgraph->subIx()];
        if (!currentGraphHandler || (rv != currentGraphHandler->repetitionCount())) {
            destroy(currentGraphHandler);
            currentGraphHandler = spider::make<GraphHandler>(subgraph, subgraph->params(), rv, this);
        } else {
            /* == Resolve every child == */
            currentGraphHandler->resolveFirings();
        }
    }
}
