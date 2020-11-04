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
    if (!firing || !parent->isStatic()) {
        /* == small optimization to avoid allocating same information for static graphs == */
        brvArray_ = spider::make_unique(make_n<u32, StackID::TRANSFO>(graph->vertexCount(), UINT32_MAX));
        ratesArray_ = spider::make_unique(make_n<EdgeRate, StackID::TRANSFO>(graph->edgeCount(), { 0, 0 }));
    }
    /* == copy parameters == */
    params_.reserve(params.size());
    dynamicParamCount_ = 0;
    for (const auto &param : params) {
        dynamicParamCount_ += param->type() == pisdf::ParamType::DYNAMIC;
        params_.emplace_back(copyParameter(param));
    }
    subgraphHandlers_ = spider::make_unique(make_n<GraphHandler *, StackID::TRANSFO>(graph->subgraphCount(), nullptr));
    alloc_ = spider::make_unique(make<GraphAlloc, StackID::SCHEDULE>(parent->graph()));
}

spider::pisdf::GraphFiring::~GraphFiring() {
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
        for (u32 k = 1; k < parentRV; ++k) {
            auto *graphFiring = const_cast<GraphFiring *>(parent_->firing(k));
            graphFiring->createOrUpdateSubgraphHandlers();
            graphFiring->resolved_ = true;
        }
    }
}

void spider::pisdf::GraphFiring::clear() {
    const auto *brv = brvArray_.get();
    if (firing_ && parent_->isStatic()) {
        brv = parent_->firing(0)->brvArray_.get();
    }
    alloc_->reset(parent_->graph(), brv);
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

int64_t spider::pisdf::GraphFiring::getSrcRate(const Edge *edge) const {
#ifndef NDEBUG
    if (edge->graph() != parent_->graph()) {
        throwSpiderException("edge does not belong to this graph.");
    }
#endif
    if (!firing_ || !parent_->isStatic()) {
        return ratesArray_[edge->ix()].srcRate_;
    } else {
        return parent_->firing(0)->ratesArray_[edge->ix()].srcRate_;
    }
}

int64_t spider::pisdf::GraphFiring::getSnkRate(const Edge *edge) const {
#ifndef NDEBUG
    if (edge->graph() != parent_->graph()) {
        throwSpiderException("edge does not belong to this graph.");
    }
#endif
    if (!firing_ || !parent_->isStatic()) {
        return ratesArray_[edge->ix()].snkRate_;
    } else {
        return parent_->firing(0)->ratesArray_[edge->ix()].snkRate_;
    }
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
    if (!firing_ || !parent_->isStatic()) {
        return brvArray_[vertex->ix()];
    } else {
        return parent_->firing(0)->brvArray_[vertex->ix()];
    }
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

/* === Private method(s) implementation === */

void spider::pisdf::GraphFiring::resolveDynamicDependentParams() {
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
    if (brvArray_[ix] != rv) {
        brvArray_[ix] = rv;
        alloc_->initialize(vertex, rv);
        if (parent_->isStatic()) {
            const auto parentRV = parent_->repetitionCount();
            for (u32 k = 1; k < parentRV; ++k) {
                parent_->firing(k)->alloc_->initialize(vertex, rv);
            }
        }
    } else {
        /* == reset values == */
        alloc_->reset(vertex, rv);
        if (parent_->isStatic()) {
            const auto parentRV = parent_->repetitionCount();
            for (u32 k = 1; k < parentRV; ++k) {
                parent_->firing(k)->alloc_->reset(vertex, rv);
            }
        }
    }
}

void spider::pisdf::GraphFiring::createOrUpdateSubgraphHandlers() {
    for (const auto &subgraph : parent_->graph()->subgraphs()) {
        const auto rvValue = getRV(subgraph);
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
