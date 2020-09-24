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
    children_ = spider::array<spider::unique_ptr<GraphHandler>>(graph->subgraphCount(), StackID::TRANSFO);
    taskIxRegister_ = spider::array<spider::unique_ptr<u32>>(graph->vertexCount(), StackID::TRANSFO);
    /* == copy parameters == */
    params_.reserve(params.size());
    for (const auto &param : params) {
        params_.emplace_back(copyParameter(param, params));
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
        currentPointer.release();
        currentPointer = spider::make_unique(spider::allocate<u32>(rvValue));
        std::fill(currentPointer.get(), std::next(currentPointer.get(), static_cast<long>(rvValue)), SIZE_MAX);
    }
    /* == creates children == */
    for (const auto &subgraph : parent_->graph()->subgraphs()) {
        children_.at(subgraph->subIx()) = spider::make_unique<GraphHandler, StackID::TRANSFO>(subgraph, params_,
                                                                                              subgraph->repetitionValue());
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
        compute(edge, vertexFiring, dependencies);
    }
    return dependencies;
}

spider::vector<spider::srless::ExecDependency>
spider::srless::FiringHandler::computeExecDependenciesByEdge(const pisdf::Vertex *vertex,
                                                             u32 vertexFiring,
                                                             u32 edgeIx) const {
    auto dependencies = factory::vector<ExecDependency>(StackID::TRANSFO);
    dependencies.reserve(2u);
    const auto *edge = vertex->inputEdge(edgeIx);
    compute(edge, vertexFiring, dependencies);
    return dependencies;
}

void spider::srless::FiringHandler::registerTaskIx(const pisdf::Vertex *vertex, u32 vertexFiring, u32 taskIx) {
#ifndef NDEBUG
    if (vertexFiring >= getRV(vertex)) {
        throwSpiderException("invalid vertex firing.");
    }
#endif
    taskIxRegister_.at(vertex->ix()).get()[vertexFiring] = taskIx;
}

u32 spider::srless::FiringHandler::getTaskIx(const spider::pisdf::Vertex *vertex, u32 vertexFiring) const {
#ifndef NDEBUG
    if (vertexFiring >= getRV(vertex)) {
        throwSpiderException("invalid vertex firing.");
    }
#endif
    return taskIxRegister_.at(vertex->ix()).get()[vertexFiring];
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

void spider::srless::FiringHandler::compute(const spider::pisdf::Edge *edge,
                                            u32 firing,
                                            spider::vector<spider::srless::ExecDependency> &dependencies) const {
    const auto snkRate = edge->sinkRateExpression().evaluate(params_);
    const auto srcRate = edge->sourceRateExpression().evaluate(params_);
    if (!snkRate) {
        dependencies.push_back({ nullptr, nullptr, UINT32_MAX, UINT32_MAX, MemoryDependency{ }});
    }
    const auto *source = edge->source();
    if (source->subtype() == pisdf::VertexType::DELAY) {
        const auto *delay = source->convertTo<pisdf::DelayVertex>()->delay();
        const auto *delayEdge = delay->edge();
        const auto delaySrcRate = delayEdge->sourceRateExpression().evaluate(params_);
        const auto delaySnkRate = delayEdge->sinkRateExpression().evaluate(params_);
        const auto srcRV = delayEdge->source()->repetitionValue();
        const auto depMin = srcRV - math::ceilDiv(delay->value() - (firing * snkRate), delaySrcRate);
        const auto depMax = srcRV - math::ceilDiv(delay->value() - (firing + 1) * snkRate + 1, delaySrcRate);
        const auto offset = delayEdge->sink()->repetitionValue() * delaySnkRate - delay->value();
        const auto memoryStart = static_cast<u32>((offset + firing * snkRate) % delaySrcRate);
        const auto memoryEnd = static_cast<u32>((offset + (firing + 1) * snkRate - 1) % delaySrcRate);
        dependencies.push_back({ delayEdge->source(), this, static_cast<u32>(depMin), static_cast<u32>(depMax),
                                 MemoryDependency{ static_cast<size_t>(delaySrcRate),
                                                   static_cast<u32>(delayEdge->sourcePortIx()),
                                                   memoryStart,
                                                   memoryEnd }});
    } else if (edge->delay()) {
        const auto *delay = edge->delay();
        const auto delayValue = delay->value();
        const auto lowerCons = snkRate * firing;
        const auto upperCons = snkRate * (firing + 1u);
        if ((delayValue + 1) > upperCons) {
            const auto *delayEdge = delay->vertex()->inputEdge(0);
            const auto setterRate = delayEdge->sourceRateExpression().evaluate(params_);
            const auto depMin = static_cast<u32>(math::floorDiv(lowerCons, setterRate));
            const auto depMax = static_cast<u32>(math::floorDiv(upperCons - 1, setterRate));
            const auto memoryStart = static_cast<u32>(lowerCons % setterRate);
            const auto memoryEnd = static_cast<u32>((upperCons - 1) % setterRate);
            dependencies.push_back({ delayEdge->source(), this, depMin, depMax,
                                     MemoryDependency{ static_cast<size_t>(setterRate),
                                                       static_cast<u32>(delayEdge->sourcePortIx()),
                                                       memoryStart,
                                                       memoryEnd }});
        } else if (delayValue > lowerCons) {
            const auto *delayEdge = delay->vertex()->inputEdge(0);
            const auto *setter = delayEdge->source();
            /* == Set dependency on setter == */
            const auto setterRate = delayEdge->sourceRateExpression().evaluate(params_);
            const auto depMin = static_cast<u32>(math::floorDiv(lowerCons - delayValue, setterRate));
            const auto memoryStart = static_cast<u32>(lowerCons % setterRate);
            const auto memorySetterEnd = static_cast<u32>(setterRate - 1);
            dependencies.push_back({ setter, this, depMin, setter->repetitionValue() - 1,
                                     MemoryDependency{ static_cast<size_t>(setterRate),
                                                       static_cast<u32>(delayEdge->sourcePortIx()),
                                                       memoryStart,
                                                       memorySetterEnd }});
            /* == Set dependency on original source == */
            const auto depMax = static_cast<u32>(math::floorDiv(upperCons - delayValue - 1, srcRate));
            const auto memoryEnd = static_cast<u32>((upperCons - 1) % srcRate);
            dependencies.push_back({ source, this, 0u, depMax,
                                     MemoryDependency{ static_cast<size_t>(srcRate),
                                                       static_cast<u32>(edge->sourcePortIx()),
                                                       0u,
                                                       memoryEnd }});
        } else {
            const auto depMin = static_cast<u32>(math::floorDiv(lowerCons - delayValue, srcRate));
            const auto depMax = static_cast<u32>(math::floorDiv(upperCons - delayValue - 1, srcRate));
            const auto memoryStart = static_cast<u32>(lowerCons % srcRate);
            const auto memoryEnd = static_cast<u32>((upperCons - 1) % srcRate);
            dependencies.push_back({ source, this, depMin, depMax,
                                     MemoryDependency{ static_cast<size_t>(srcRate),
                                                       static_cast<u32>(edge->sourcePortIx()),
                                                       memoryStart,
                                                       memoryEnd }});
        }
    } else {
        const auto depMin = static_cast<u32>(pisdf::computeConsLowerDep(snkRate, srcRate, firing, 0));
        const auto depMax = static_cast<u32>(pisdf::computeConsUpperDep(snkRate, srcRate, firing, 0));
        const auto memoryStart = static_cast<u32>((firing * snkRate) % srcRate);
        const auto memoryEnd = static_cast<u32>(((firing + 1) * snkRate - 1) % srcRate);
        dependencies.push_back({ edge->source(), this, depMin, depMax,
                                 MemoryDependency{ static_cast<size_t>(srcRate),
                                                   static_cast<u32>(edge->sourcePortIx()),
                                                   memoryStart,
                                                   memoryEnd }});
    }
}
