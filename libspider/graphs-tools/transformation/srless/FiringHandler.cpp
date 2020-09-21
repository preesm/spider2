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
        children_{ factory::vector<spider::unique_ptr<GraphHandler>>(StackID::TRANSFO) },
        params_{ factory::vector<std::shared_ptr<pisdf::Param>>(StackID::TRANSFO) },
        brv_{ factory::vector<u32>(StackID::TRANSFO) },
        parent_{ parent },
        ix_{ SIZE_MAX },
        firing_{ firing },
        resolved_{ false } {
    const auto *graph = parent->graph();
    brv_.resize(graph->vertexCount(), UINT32_MAX);
    children_.reserve(graph->subgraphCount());
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
        spider::set_at(brv_, vertex->ix(), static_cast<u32>(vertex->repetitionValue()));
    }
    /* == creates children == */
    children_.clear();
    for (const auto &subgraph : parent_->graph()->subgraphs()) {
        children_.emplace_back(spider::make_unique<GraphHandler, StackID::TRANSFO>(subgraph, params_,
                                                                                   subgraph->repetitionValue()));
    }
    resolved_ = true;
}

u32 spider::srless::FiringHandler::getRV(const spider::pisdf::Vertex *vertex) const {
#ifndef NDEBUG
    if (vertex->graph() != parent_->graph()) {
        throwSpiderException("vertex does not belong to the correct graph.");
    }
#endif
    return spider::get_at(brv_, vertex->ix());
}

spider::vector<spider::srless::ExecDependency>
spider::srless::FiringHandler::computeDependencies(const pisdf::Vertex *vertex, u32 vertexFiring) const {
    auto dependencies = factory::vector<ExecDependency>(StackID::TRANSFO);
    for (const auto *edge : vertex->inputEdgeVector()) {
        const auto sinkRate = edge->sinkRateExpression().evaluate(params_);
        const auto sourceRate = edge->sourceRateExpression().evaluate(params_);
        const auto *source = edge->source();
        if (source->subtype() == pisdf::VertexType::DELAY) {
            const auto *delay = source->convertTo<pisdf::DelayVertex>()->delay();
            const auto *delayEdge = delay->edge();
            const auto originalSourceRate = delayEdge->sourceRateExpression().evaluate(params_);
            const auto originalSinkRate = delayEdge->sinkRateExpression().evaluate(params_);
            const auto offset = delayEdge->sink()->repetitionValue() * originalSinkRate - delay->value();
            const auto sourceRV = delayEdge->source()->repetitionValue();
            const auto memoryStart = static_cast<u32>((offset + vertexFiring * sinkRate) % originalSourceRate);
            const auto memoryEnd = static_cast<u32>((offset + (vertexFiring + 1) * sinkRate - 1) % originalSourceRate);
            const auto depMin = sourceRV - static_cast<u32>(math::ceilDiv(delay->value() - (vertexFiring * sinkRate),
                                                                          originalSourceRate));
            const auto depMax =
                    sourceRV - static_cast<u32>(math::ceilDiv(delay->value() - (vertexFiring + 1) * sinkRate + 1,
                                                              originalSourceRate));
            dependencies.push_back({ delayEdge->source(), sinkRate, memoryStart, memoryEnd, depMin, depMax });
        } else if (edge->delay()) {
            const auto *delay = edge->delay();
            const auto delayValue = delay->value();
            const auto lowerCons = sinkRate * vertexFiring;
            const auto upperCons = sinkRate * (vertexFiring + 1u);
            if ((delayValue + 1) > upperCons) {
                const auto *delayEdge = delay->vertex()->inputEdge(0);
                const auto setterRate = delayEdge->sourceRateExpression().evaluate(params_);
                const auto memoryStart = static_cast<u32>(lowerCons % setterRate);
                const auto memoryEnd = static_cast<u32>((upperCons - 1) % setterRate);
                const auto depMin = static_cast<u32>(math::floorDiv(lowerCons, setterRate));
                const auto depMax = static_cast<u32>(math::floorDiv(upperCons - 1, setterRate));
                dependencies.push_back({ delayEdge->source(), sinkRate, memoryStart, memoryEnd, depMin, depMax });
            } else if (delayValue > lowerCons) {
                const auto *delayEdge = delay->vertex()->inputEdge(0);
                const auto *setter = delayEdge->source();
                const auto setterRate = delayEdge->sourceRateExpression().evaluate(params_);
                const auto memoryStart = static_cast<u32>(lowerCons % setterRate);
                const auto memorySetterEnd = static_cast<u32>(setterRate - 1);
                const auto memoryEnd = static_cast<u32>((upperCons - 1) % sourceRate);
                const auto depMin = static_cast<u32>(math::floorDiv(lowerCons - delayValue, setterRate));
                const auto depSetterMax = setter->repetitionValue() - 1;
                const auto depMax = static_cast<u32>(math::floorDiv(upperCons - delayValue - 1, sourceRate));
                dependencies.push_back({ setter, sinkRate, memoryStart, memorySetterEnd, depMin, depSetterMax });
                dependencies.push_back({ source, sinkRate, 0u, memoryEnd, 0, depMax });
            } else {
                const auto memoryStart = static_cast<u32>(lowerCons % sourceRate);
                const auto memoryEnd = static_cast<u32>((upperCons - 1) % sourceRate);
                const auto depMin = static_cast<u32>(math::floorDiv(lowerCons - delayValue, sourceRate));
                const auto depMax = static_cast<u32>(math::floorDiv(upperCons - delayValue - 1, sourceRate));
                dependencies.push_back({ source, sinkRate, memoryStart, memoryEnd, depMin, depMax });
            }
        } else {
            const auto memoryStart = static_cast<u32>((vertexFiring * sinkRate) % sourceRate);
            const auto memoryEnd = static_cast<u32>(((vertexFiring + 1) * sinkRate - 1) % sourceRate);
            const auto depMin = static_cast<u32>(pisdf::computeConsLowerDep(sinkRate, sourceRate, vertexFiring, 0));
            const auto depMax = static_cast<u32>(pisdf::computeConsUpperDep(sinkRate, sourceRate, vertexFiring, 0));
            dependencies.push_back({ edge->source(), sinkRate, memoryStart, memoryEnd, depMin, depMax });
        }
    }
    return dependencies;
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
