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
#include <graphs-tools/helper/pisdf.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs-tools/numerical/dependencies.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::srdagless::SRLessHandler::SRLessHandler() :
        verticesToSchedule_{ factory::vector<pisdf::Vertex *>(StackID::TRANSFO) } { }

void spider::srdagless::SRLessHandler::computeRV(pisdf::Graph *graph, u32 graphFiring) {
    if (graph->dynamic() && !graph->configVertexCount()) {
        const auto &paramVector = parameters_.at(graph);
        /* == Dynamic graph == */
        brv::compute(graph, paramVector[graphFiring]);
        /* == Copy the repetition vector values == */
        if (graph2RV_.find(graph) == graph2RV_.end()) {
            graph2RV_[graph] = makeRVVector();
        }
        graph2RV_[graph].emplace_back(factory::vector<u32>(StackID::TRANSFO));
        auto &rvVector = graph2RV_[graph][graphFiring];
        for (auto &vertex : graph->vertices()) {
            rvVector[vertex->ix()] = vertex->repetitionValue();
        }
    } else {
        brv::compute(graph);
    }
}

ifast64 spider::srdagless::SRLessHandler::computeConsLowerDep(pisdf::Edge *edge,
                                                              u32 vertexFiring,
                                                              pisdf::Graph *graph,
                                                              u32 graphFiring) const {
    const auto &paramVector = parameters_.at(graph);
    if (!(graph->dynamic() && !graph->configVertexCount())) {
        graphFiring = 0;
    }
    const auto prod = edge->sourceRateExpression().evaluate(paramVector[graphFiring]);
    const auto cons = edge->sinkRateExpression().evaluate(paramVector[graphFiring]);
    const auto delay = edge->delay() ? edge->delay()->value() : 0;
    return pisdf::computeConsLowerDep(cons, prod, vertexFiring, delay);
}

ifast64 spider::srdagless::SRLessHandler::computeConsUpperDep(pisdf::Edge *edge,
                                                              u32 vertexFiring,
                                                              pisdf::Graph *graph,
                                                              u32 graphFiring) const {
    const auto &paramVector = parameters_.at(graph);
    if (!(graph->dynamic() && !graph->configVertexCount())) {
        graphFiring = 0;
    }
    const auto prod = edge->sourceRateExpression().evaluate(paramVector[graphFiring]);
    const auto cons = edge->sinkRateExpression().evaluate(paramVector[graphFiring]);
    const auto delay = edge->delay() ? edge->delay()->value() : 0;
    return pisdf::computeConsUpperDep(cons, prod, vertexFiring, delay);
}

void spider::srdagless::SRLessHandler::copyParameters(pisdf::Graph *graph, u32 graphRepCount, u32 parentFiring) {
    if (parameters_.find(graph) == parameters_.end()) {
        parameters_[graph] = makeParamVector();
    }
    auto &paramVector = parameters_[graph];
    paramVector.reserve(paramVector.size() + graphRepCount);
    for (u32 i = 0; i < graphRepCount; ++i) {
        parameters_[graph].emplace_back(factory::vector<std::shared_ptr<pisdf::Param>>(StackID::TRANSFO));
        for (auto &param : graph->params()) {
            auto p = param;
            if (param->dynamic()) {
                if (param->type() == pisdf::ParamType::DYNAMIC) {
                    p = make_shared<pisdf::DynamicParam, StackID::PISDF>(param->name(), param->expression());
                    p->setIx(param->ix());
                } else {
                    const auto &parentParamVector = parameters_[graph->graph()][parentFiring];
                    const auto &parentParam = parentParamVector[param->parent()->ix()];
                    p = make_shared<pisdf::Param, StackID::PISDF>(param->name(), parentParam->value());
                }
            }
            parameters_[graph].back().emplace_back(std::move(p));
        }
    }
}

u32 spider::srdagless::SRLessHandler::getRepetitionValue(pisdf::Vertex *vertex, u32 graphFiring) const {
    if (!vertex) {
        return 0;
    }
    auto *graph = vertex->graph();
    if (!(graph->dynamic() && !graph->configVertexCount())) {
        /* == Graph is static so we can use vertex value directly == */
        return vertex->repetitionValue();
    }
    const auto &rvVector = graph2RV_.at(graph);
    return rvVector[graphFiring][vertex->ix()];
}

void spider::srdagless::SRLessHandler::setParamValue(pisdf::Param *param, u32 graphFiring, i64 value) {
    if (!param) {
        return;
    }
    auto *graph = param->graph();
    const auto &paramVector = parameters_.at(graph);
    if (!(graph->dynamic() && !graph->configVertexCount())) {
        graphFiring = 0;
    }
    paramVector[graphFiring][param->ix()]->setValue(value);
}

void spider::srdagless::SRLessHandler::createsProductionDependencies(pisdf::Vertex *vertex, u32 graphFiring) {
    const auto &graph = vertex->graph();
    if (prodDependencies_.find(graph) == prodDependencies_.end()) {
        auto vector = factory::vector<DependencyVector>(StackID::TRANSFO);
        vector.reserve(graph->vertexCount());
        for (size_t i = 0; i < graph->vertexCount(); ++i) {
            vector.emplace_back(makeDependencyVector());
        }
        prodDependencies_[graph] = std::move(vector);
    }
    const auto &params = getParameters(graph, graphFiring);
    auto &dependencies = prodDependencies_.at(graph).at(vertex->ix());
    if (!dependencies.empty()) {
        return;
    }
    const auto repetitionValue = getRepetitionValue(vertex, graphFiring);
    for (auto edge : vertex->outputEdgeVector()) {
        auto *sink = edge->sink();
        auto *delay = edge->delay();
        const auto sourceRate = edge->sourceRateExpression().evaluate(params);
        const auto delayValue = delay ? delay->value() : 0;
        if (sink->subtype() == pisdf::VertexType::DELAY) {
            edge = sink->convertTo<pisdf::DelayVertex>()->delay()->edge();
            sink = edge->sink();
        }
        const auto sinkRate = edge->sinkRateExpression().evaluate(params);
        /* == Creates the vector of dependencies == */
        auto currentEdgeDependencies = factory::vector<Dependency>(StackID::TRANSFO);
        currentEdgeDependencies.reserve(repetitionValue);
        if (delay && delayValue) {
            auto *getter = delay->getter();
            const auto sinkRV = static_cast<i32>(getRepetitionValue(sink, graphFiring));
            const auto correctedDelay = delayValue - sinkRV * sinkRate;
            const auto getterRate = delay->vertex()->outputEdge(0)->sinkRateExpression().evaluate(params);
            for (u32 i = 0; i < repetitionValue; ++i) {
                auto depMin = static_cast<i32>(pisdf::computeProdLowerDep(sinkRate, sourceRate, i, delayValue));
                auto depMax = static_cast<i32>(pisdf::computeProdUpperDep(sinkRate, sourceRate, i, delayValue));
                if (depMin >= sinkRV) {
                    depMin = static_cast<i32>(pisdf::computeProdLowerDep(getterRate, sourceRate, i, correctedDelay));
                    depMax = static_cast<i32>(pisdf::computeProdUpperDep(getterRate, sourceRate, i, correctedDelay));
                    currentEdgeDependencies.push_back({ getter, depMin, depMax });
                } else if (depMax >= sinkRV) {
                    currentEdgeDependencies.push_back({ sink, depMin, sinkRV - 1 });
                    depMax = static_cast<i32>(pisdf::computeProdUpperDep(getterRate, sourceRate, i, correctedDelay));
                    currentEdgeDependencies.push_back({ getter, 0, depMax });
                } else {
                    currentEdgeDependencies.push_back({ sink, depMin, depMax });
                }
            }
        } else {
            for (u32 i = 0; i < repetitionValue; ++i) {
                const auto depMin = static_cast<i32>(pisdf::computeProdLowerDep(sinkRate, sourceRate, i, 0));
                const auto depMax = static_cast<i32>(pisdf::computeProdUpperDep(sinkRate, sourceRate, i, 0));
                currentEdgeDependencies.push_back({ sink, depMin, depMax });
            }
        }
        dependencies.emplace_back(std::move(currentEdgeDependencies));
    }
}

void spider::srdagless::SRLessHandler::createsConsumptionDependencies(pisdf::Vertex *vertex, u32 graphFiring) {
    const auto &graph = vertex->graph();
    if (consDependencies_.find(graph) == consDependencies_.end()) {
        auto vector = factory::vector<DependencyVector>(StackID::TRANSFO);
        vector.reserve(graph->vertexCount());
        for (size_t i = 0; i < graph->vertexCount(); ++i) {
            vector.emplace_back(makeDependencyVector());
        }
        consDependencies_[graph] = std::move(vector);
    }
    const auto &params = getParameters(graph, graphFiring);
    auto &dependencies = consDependencies_.at(graph).at(vertex->ix());
    if (!dependencies.empty()) {
        return;
    }
    const auto repetitionValue = getRepetitionValue(vertex, graphFiring);
    for (auto edge : vertex->inputEdgeVector()) {
        auto *delay = edge->delay();
        auto *source = edge->source();
        const auto sinkRate = edge->sinkRateExpression().evaluate(params);
        const auto delayValue = delay ? delay->value() : 0;
        if (source->subtype() == pisdf::VertexType::DELAY) {
            edge = source->convertTo<pisdf::DelayVertex>()->delay()->edge();
            source = edge->source();
        }
        const auto sourceRate = edge->sourceRateExpression().evaluate(params);
        /* == Creates the vector of dependencies == */
        auto currentEdgeDependencies = factory::vector<Dependency>(StackID::TRANSFO);
        currentEdgeDependencies.reserve(repetitionValue);
        if (delay && delayValue) {
            auto *setter = delay->setter();
            const auto setterRV = static_cast<i32>(getRepetitionValue(setter, graphFiring));
            const auto setterRate = delay->vertex()->inputEdge(0)->sourceRateExpression().evaluate(params);
            for (u32 i = 0; i < repetitionValue; ++i) {
                auto depMin = static_cast<i32>(pisdf::computeConsLowerDep(sinkRate, sourceRate, i, delayValue));
                auto depMax = static_cast<i32>(pisdf::computeConsUpperDep(sinkRate, sourceRate, i, delayValue));
                if (depMax < 0) {
                    depMin = static_cast<i32>(pisdf::computeConsLowerDep(sinkRate, setterRate, i, 0));
                    depMax = static_cast<i32>(pisdf::computeConsUpperDep(sinkRate, setterRate, i, 0));
                    currentEdgeDependencies.push_back({ setter, depMin, depMax });
                } else if (depMin < 0) {
                    depMin = static_cast<i32>( pisdf::computeProdUpperDep(sinkRate, setterRate, i, 0));
                    currentEdgeDependencies.push_back({ setter, depMin, setterRV - 1 });
                    currentEdgeDependencies.push_back({ source, 0, depMax });
                } else {
                    currentEdgeDependencies.push_back({ source, depMin, depMax });
                }
            }
        } else {
            for (u32 i = 0; i < repetitionValue; ++i) {
                const auto depMin = static_cast<i32>(pisdf::computeConsLowerDep(sinkRate, sourceRate, i, 0));
                const auto depMax = static_cast<i32>(pisdf::computeConsUpperDep(sinkRate, sourceRate, i, 0));
                currentEdgeDependencies.push_back({ source, depMin, depMax });
            }
        }
        dependencies.emplace_back(std::move(currentEdgeDependencies));
    }
}

const spider::vector<std::shared_ptr<spider::pisdf::Param>> &
spider::srdagless::SRLessHandler::getParameters(pisdf::Graph *graph, u32 graphFiring) const {
    if (!(graph->dynamic() && !graph->configVertexCount())) {
        return graph->params();
    }
    const auto &params = parameters_.at(graph);
    return params[graphFiring];
}

void spider::srdagless::SRLessHandler::addVertexToBeScheduled(pisdf::Vertex *vertex) {
    verticesToSchedule_.emplace_back(vertex);
}

void spider::srdagless::SRLessHandler::clearVertexToBeScheduled() {
    verticesToSchedule_.clear();
}

const spider::vector<spider::pisdf::Vertex *> &spider::srdagless::SRLessHandler::getVerticesToSchedule() const {
    return verticesToSchedule_;
}

/* === Private method(s) implementation === */
