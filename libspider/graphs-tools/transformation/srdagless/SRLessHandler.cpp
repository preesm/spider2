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
#include <graphs-tools/helper/pisdf.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs-tools/numerical/dependencies.h>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::srdagless::SRLessHandler::SRLessHandler(spider::pisdf::Graph *graph) : graph_{ graph } { }

void spider::srdagless::SRLessHandler::computeRV(pisdf::Graph *graph, u32 graphFiring) {
    const auto &paramVector = graph2Params_.at(graph);
    if (graph->dynamic() && !graph->configVertexCount()) {
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
        brv::compute(graph, paramVector[0]);
    }
}

ifast64 spider::srdagless::SRLessHandler::computeConsLowerDep(pisdf::Edge *edge,
                                                              u32 vertexFiring,
                                                              pisdf::Graph *graph,
                                                              u32 graphFiring) const {
    const auto &paramVector = graph2Params_.at(graph);
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
    const auto &paramVector = graph2Params_.at(graph);
    if (!(graph->dynamic() && !graph->configVertexCount())) {
        graphFiring = 0;
    }
    const auto prod = edge->sourceRateExpression().evaluate(paramVector[graphFiring]);
    const auto cons = edge->sinkRateExpression().evaluate(paramVector[graphFiring]);
    const auto delay = edge->delay() ? edge->delay()->value() : 0;
    return pisdf::computeConsUpperDep(cons, prod, vertexFiring, delay);
}

void spider::srdagless::SRLessHandler::copyParameters(pisdf::Graph *graph, u32 graphRepCount, u32 parentFiring) {
    if (graph2Params_.find(graph) == graph2Params_.end()) {
        graph2Params_[graph] = makeParamVector();
    }
    auto &paramVector = graph2Params_[graph];
    paramVector.reserve(paramVector.size() + graphRepCount);

    for (u32 i = 0; i < graphRepCount; ++i) {
        graph2Params_[graph].emplace_back(factory::vector<std::shared_ptr<pisdf::Param>>(StackID::TRANSFO));
        for (auto &param : graph->params()) {
            auto p = param;
            if (param->dynamic()) {
                if (param->type() == pisdf::ParamType::DYNAMIC) {
                    p = make_shared<pisdf::DynamicParam, StackID::PISDF>(param->name(), param->expression());
                    p->setIx(param->ix());
                } else {
                    const auto &parentParamVector = graph2Params_[graph->graph()][parentFiring];
                    const auto &parentParam = parentParamVector[param->parent()->ix()];
                    p = make_shared<pisdf::Param, StackID::PISDF>(param->name(), parentParam->value());
                }
            }
            graph2Params_[graph].back().emplace_back(std::move(p));
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
    const auto &paramVector = graph2Params_.at(graph);
    if (!(graph->dynamic() && !graph->configVertexCount())) {
        graphFiring = 0;
    }
    paramVector[graphFiring][param->ix()]->setValue(value);
}

