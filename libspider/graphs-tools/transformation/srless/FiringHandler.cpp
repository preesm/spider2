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
    taskIxRegister_ = spider::array<u32 *>(graph->vertexCount(), nullptr, StackID::TRANSFO);
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

void spider::srless::FiringHandler::registerTaskIx(const pisdf::Vertex *vertex, u32 firing, u32 taskIx) {
#ifndef NDEBUG
    if (firing >= getRV(vertex)) {
        throwSpiderException("invalid vertex firing.");
    }
#endif
    taskIxRegister_.at(vertex->ix())[firing] = taskIx;
}

void spider::srless::FiringHandler::resolveBRV() {
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
    /* == creates children == */
    for (const auto &subgraph : parent_->graph()->subgraphs()) {
        const auto ix = subgraph->ix();
        const auto rvValue = brv_.at(ix);
        auto &currentGraphHandler = children_.at(subgraph->subIx());
        if (!currentGraphHandler || (rvValue != currentGraphHandler->repetitionCount())) {
            destroy(currentGraphHandler);
            currentGraphHandler = spider::make<GraphHandler>(subgraph, params_, subgraph->repetitionValue(), this);
        }
    }
    resolved_ = true;
}

void spider::srless::FiringHandler::clear() {
    for (const auto &vertex : parent_->graph()->vertices()) {
        const auto ix = vertex->ix();
        const auto rvValue = brv_.at(ix);
        if (rvValue != UINT32_MAX) {
            std::fill(taskIxRegister_.at(ix), std::next(taskIxRegister_.at(ix), brv_.at(ix)), UINT32_MAX);
        }
    }
    for (auto &graphHandler : children_) {
        if (graphHandler) {
            graphHandler->clear();
        }
    }
    resolved_ = false;
}

u32 spider::srless::FiringHandler::getRV(const spider::pisdf::Vertex *vertex) const {
#ifndef NDEBUG
    if (vertex->graph() != parent_->graph()) {
        throwSpiderException("vertex does not belong to the correct graph.");
    }
#endif
    return brv_.at(vertex->ix());
}

u32 spider::srless::FiringHandler::getTaskIx(const spider::pisdf::Vertex *vertex, u32 vertexFiring) const {
#ifndef NDEBUG
    if (vertexFiring >= getRV(vertex)) {
        throwSpiderException("invalid vertex firing.");
    }
#endif
    return taskIxRegister_.at(vertex->ix())[vertexFiring];
}

const spider::srless::FiringHandler *
spider::srless::FiringHandler::getChildFiring(const pisdf::Graph *subgraph, u32 firing) const {
#ifndef NDEBUG
    if (subgraph->graph() != parent_->graph()) {
        throwSpiderException("subgraph does not belong to this graph.");
    }
#endif
    return children_[subgraph->subIx()]->firings()[firing];
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
            newParam = spider::make_shared<pisdf::Param, StackID::PISDF>(param->name(), parent);
        } else {
            newParam = spider::make_shared<pisdf::Param, StackID::PISDF>(*param);
        }
        newParam->setIx(param->ix());
        return newParam;
    }
    return param;
}