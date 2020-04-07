/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
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

#include <graphs-tools/transformation/srdagless/SRLessHandler.h>
#include <graphs/pisdf/DynamicParam.h>
#include <graphs/pisdf/InHeritedParam.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs-tools/numerical/dependencies.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::srdagless::SRLessHandler::SRLessHandler(spider::pisdf::Graph *graph, const SRLessHandler *parentHandler) :
        graph_{ graph },
        parentHandler_{ parentHandler },
        params_{ factory::vector<std::shared_ptr<pisdf::Param>>(graph->paramCount(), StackID::TRANSFO) },
        subHandlers_{ factory::vector<spider::unique_ptr<SRLessHandler>>(StackID::TRANSFO) },
        vertexDependencies_{ factory::vector<spider::vector<VertexDependencies>>(StackID::TRANSFO) } {
    for (const auto &param : graph_->params()) {
        const auto ix = param->ix();
        if (param->dynamic()) {
            if (param->type() == pisdf::ParamType::DYNAMIC) {
                params_[ix] = spider::make_shared<pisdf::DynamicParam, StackID::PISDF>(param->name(),
                                                                                       param->expression());
                params_[ix]->setIx(ix);
            } else if (parentHandler_) {
                params_[ix] = parentHandler_->params_[param->ix()];
            } else {
                throwNullptrException();
            }
        } else {
            params_[ix] = param;
        }
    }
}

void spider::srdagless::SRLessHandler::resolveStatic() {
    subHandlers_.clear();
    /* == Compute RV == */
    brv::compute(graph_, params_);
    /* == Evaluate if there are dynamic subgraphs or not == */
    for (auto &subgraph : graph_->subgraphs()) {
        if (subgraph->configVertexCount()) {
            for (u32 i = 0; i < subgraph->repetitionValue(); ++i) {
                auto *subHandler = make<SRLessHandler>(subgraph, this);
                // TODO: clear previous and copy config vertices
                // TODO: update config vertices
                subHandlers_.emplace_back(subHandler);
            }
        } else {
            subHandlers_.emplace_back(make<SRLessHandler>(subgraph, this));
        }
    }
    /* == Replace interfaces (if needed) == */
    // TODO: add repeat / tail actors when rates are not transparent, copy dependencies when needed.
    /* == Compute the dependencies == */
    for (const auto &vertex : graph_->vertices()) {
        if (vertex->subtype() == pisdf::VertexType::DELAY) {
            continue;
        }
        computeDependencies(vertex.get());
    }
}

const spider::vector<spider::VertexDependencies> &
spider::srdagless::SRLessHandler::getVertexDependencies(const pisdf::Vertex *vertex) const {
    return vertexDependencies_.at(vertex->ix());
}

const spider::vector<std::shared_ptr<spider::pisdf::Param>> &spider::srdagless::SRLessHandler::getParameters() const {
    return params_;
}

/* === Private method(s) === */

void spider::srdagless::SRLessHandler::computeDependencies(const spider::pisdf::Vertex *vertex) {
    auto firingDependencies = factory::vector<VertexDependencies>(StackID::TRANSFO);
    for (u32 i = 0; i < vertex->repetitionValue(); ++i) {
        auto firingDependency = factory::vector<ExecDependency>(StackID::TRANSFO);
        for (const auto &edge : vertex->inputEdgeVector()) {
            if (edge->source()->subtype() == pisdf::VertexType::DELAY) {
                computeGetterDependency(edge, i, firingDependency);
            } else if (edge->delay()) {
                computeDelayedDependency(edge, i, firingDependency);
            } else {
                computeDependency(edge, i, firingDependency);
            }
        }
        firingDependencies.emplace_back(std::move(firingDependency));
    }
    vertexDependencies_.emplace_back(std::move(firingDependencies));
}

void spider::srdagless::SRLessHandler::computeDependency(const pisdf::Edge *edge,
                                                         u32 firing,
                                                         spider::vector<spider::ExecDependency> &firingDependency) {
    const auto sinkRate = edge->sinkRateExpression().evaluate(params_);
    const auto sourceRate = edge->sourceRateExpression().evaluate(params_);
    const auto memoryStart = static_cast<u32>((firing * sinkRate) % sourceRate);
    const auto memoryEnd = static_cast<u32>(((firing + 1) * sinkRate - 1) % sourceRate);
    const auto depMin = static_cast<u32>(pisdf::computeConsLowerDep(sinkRate, sourceRate, firing, 0));
    const auto depMax = static_cast<u32>(pisdf::computeConsUpperDep(sinkRate, sourceRate, firing, 0));
    firingDependency.push_back({ edge->source(), memoryStart, memoryEnd, depMin, depMax });
}

void spider::srdagless::SRLessHandler::computeDelayedDependency(const pisdf::Edge *edge,
                                                                u32 firing,
                                                                spider::vector<ExecDependency> &firingDependency) {
    auto *delay = edge->delay();
    auto *source = edge->source();
    const auto delayValue = delay->value();
    const auto sinkRate = edge->sinkRateExpression().evaluate(params_);
    const auto sourceRate = edge->sourceRateExpression().evaluate(params_);
    const auto lowerCons = sinkRate * firing;
    const auto upperCons = sinkRate * (firing + 1);
    if ((delayValue + 1) > upperCons) {
        const auto *delayEdge = delay->vertex()->inputEdge(0);
        const auto setterRate = delayEdge->sourceRateExpression().evaluate(params_);
        const auto memoryStart = static_cast<u32>(lowerCons % setterRate);
        const auto memoryEnd = static_cast<u32>((upperCons - 1) % setterRate);
        const auto depMin = static_cast<u32>(math::floorDiv(lowerCons, setterRate));
        const auto depMax = static_cast<u32>(math::floorDiv(upperCons - 1, setterRate));
        firingDependency.push_back({ delayEdge->source(), memoryStart, memoryEnd, depMin, depMax });
    } else if (delayValue > lowerCons) {
        const auto *delayEdge = delay->vertex()->inputEdge(0);
        auto *setter = delayEdge->source();
        const auto setterRate = delayEdge->sourceRateExpression().evaluate(params_);
        const auto memoryStart = static_cast<u32>(lowerCons % setterRate);
        const auto memorySetterEnd = static_cast<u32>(setterRate - 1);
        const auto memoryEnd = static_cast<u32>((upperCons - 1) % sourceRate);
        const auto depMin = static_cast<u32>(math::floorDiv(lowerCons - delayValue, setterRate));
        const auto depSetterMax = setter->repetitionValue() - 1;
        const auto depMax = static_cast<u32>(math::floorDiv(upperCons - delayValue - 1, sourceRate));
        firingDependency.push_back({ delayEdge->source(), memoryStart, memorySetterEnd, depMin, depSetterMax });
        firingDependency.push_back({ source, 0u, memoryEnd, 0, depMax });
    } else {
        const auto memoryStart = static_cast<u32>(lowerCons % sourceRate);
        const auto memoryEnd = static_cast<u32>((upperCons - 1) % sourceRate);
        const auto depMin = static_cast<u32>(math::floorDiv(lowerCons - delayValue, sourceRate));
        const auto depMax = static_cast<u32>(math::floorDiv(upperCons - delayValue - 1, sourceRate));
        firingDependency.push_back({ source, memoryStart, memoryEnd, depMin, depMax });
    }
}

void spider::srdagless::SRLessHandler::computeGetterDependency(const pisdf::Edge *edge,
                                                               u32 firing,
                                                               spider::vector<spider::ExecDependency> &firingDependency) {
    const auto *delay = edge->source()->convertTo<pisdf::DelayVertex>()->delay();
    const auto *delayEdge = delay->edge();
    const auto originalSourceRate = delayEdge->sourceRateExpression().evaluate(params_);
    const auto originalSinkRate = delayEdge->sinkRateExpression().evaluate(params_);
    const auto offset = delay->value() + delayEdge->sink()->repetitionValue() * originalSinkRate;
    const auto sourceRV = delayEdge->source()->repetitionValue();
    const auto sinkRate = edge->sinkRateExpression().evaluate(params_);
    const auto memoryStart = static_cast<u32>((offset + firing * sinkRate) % originalSourceRate);
    const auto memoryEnd = static_cast<u32>((offset + (firing + 1) * sinkRate - 1) % originalSourceRate);
    const auto depMin = sourceRV - static_cast<u32>(math::ceilDiv(firing * sinkRate, originalSourceRate));
    const auto depMax = sourceRV - static_cast<u32>(math::ceilDiv((firing + 1) * sinkRate, originalSourceRate));
    firingDependency.push_back({ delayEdge->source(), memoryStart, memoryEnd, depMin, depMax });

}


