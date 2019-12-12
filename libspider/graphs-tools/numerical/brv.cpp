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

#include <graphs-tools/numerical/brv.h>
#include <graphs-tools/numerical/UpdateBRVVisitor.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Param.h>
#include <api/config-api.h>

/* === Function(s) definition === */

void spider::brv::compute(const spider::pisdf::Graph *graph, const spider::vector<pisdf::Param *> &params) {
    /* == 0. Extract connected components of the graph == */
    const auto &connectedComponents = extractConnectedComponents(graph);

    /* == 1. For each connected component compute LCM and RV == */
    spider::array<spider::Rational> rationalArray{ graph->vertexCount(), spider::Rational(), StackID::TRANSFO };
    for (const auto &component : connectedComponents) {
        /* == 1.1 Extract the edges == */
        auto edgeArray = extractEdgesFromComponent(component);

        /* == 1.2 Compute the Rationals == */
        extractRationalsFromEdges(rationalArray, edgeArray, params);

        /* == 1.3 Compute the LCM factor for the current component == */
        int64_t lcmFactor = 1;
        for (const auto &vertex : component.vertexVector_) {
            lcmFactor = spider::math::lcm(lcmFactor, rationalArray[vertex->ix()].denominator());
        }

        /* == 1.4 Compute the repetition value for vertices of the connected component == */
        for (auto &vertex : component.vertexVector_) {
            rationalArray[vertex->ix()] *= lcmFactor;
            vertex->setRepetitionValue(static_cast<uint32_t>(rationalArray[vertex->ix()].toUInt64()));
        }

        /* == 1.5 Update repetition values based on PiSDF rules of input / output interfaces and config actors == */
        updateBRV(component, params);

        /* == 1.6 Check consistency of the connected component == */
        checkConsistency(edgeArray, params);
    }

    /* == Print BRV (if VERBOSE) == */
    spider::brv::print(graph);
}

void spider::brv::compute(const spider::pisdf::Graph *graph) {
    compute(graph, graph->params());
}

spider::vector<spider::brv::ConnectedComponent>
spider::brv::extractConnectedComponents(const spider::pisdf::Graph *graph) {
    /* == Keys used to check if a vertex has already be assigned to a connected component == */
    auto visited = spider::containers::vector<bool>(graph->vertexCount(), false, StackID::TRANSFO);

    /* == Iterate over every vertex and assign it to a connected component == */
    auto connectedComponents = spider::containers::vector<ConnectedComponent>(StackID::TRANSFO);
    for (const auto &vertex : graph->vertices()) {
        if (!visited[vertex->ix()]) {
            /* == Initiate a new connected component == */
            visited[vertex->ix()] = true;
            ConnectedComponent component;
            component.vertexVector_.emplace_back(vertex);

            /* == Extract every vertices of the connected component using a non-recursive BFS algorithm == */
            size_t visitedIndex = 0;
            while (visitedIndex != component.vertexVector_.size()) {
                auto *currentVertex = component.vertexVector_[visitedIndex++];

                /* == Scan output edges == */
                component.edgeCount_ += currentVertex->outputEdgeCount();
                for (const auto *edge : currentVertex->outputEdgeArray()) {
                    if (!edge) {
                        throwSpiderException("Vertex [%s] has null output edge.", currentVertex->name().c_str());
                    }
                    auto *sink = edge->sink();
                    if (sink->subtype() != spider::pisdf::VertexType::OUTPUT && !visited[sink->ix()]) {
                        /* == Register the vertex == */
                        component.vertexVector_.emplace_back(sink);
                        visited[sink->ix()] = true;
                    }
                }

                /* == Scan input edges == */
                for (const auto *edge : currentVertex->inputEdgeArray()) {
                    if (!edge) {
                        throwSpiderException("Vertex [%s] has null input edge.", currentVertex->name().c_str());
                    }
                    auto *source = edge->source();
                    if (source->subtype() != spider::pisdf::VertexType::INPUT && !visited[source->ix()]) {
                        /* == Register the vertex == */
                        component.vertexVector_.emplace_back(source);
                        visited[source->ix()] = true;
                    } else if (source->subtype() == spider::pisdf::VertexType::INPUT) {
                        component.edgeCount_ += 1;
                    }
                }
            }

            /* == Move the component into output vector == */
            connectedComponents.emplace_back(std::move(component));
        }
    }
    return connectedComponents;
}

spider::array<const spider::pisdf::Edge *>
spider::brv::extractEdgesFromComponent(const ConnectedComponent &component) {
    spider::array<const pisdf::Edge *> edgeArray{ component.edgeCount_, StackID::TRANSFO };
    size_t index = 0;
    for (const auto &vertex : component.vertexVector_) {
        for (const auto &edge: vertex->outputEdgeArray()) {
            edgeArray[index++] = edge;
        }
        for (const auto &edge: vertex->inputEdgeArray()) {
            if (edge->source()->subtype() == pisdf::VertexType::INPUT) {
                edgeArray[index++] = edge;
            }
        }
    }
    return edgeArray;
}

void spider::brv::extractRationalsFromEdges(spider::array<spider::Rational> &rationalArray,
                                            const spider::array<const pisdf::Edge *> &edgeArray,
                                            const spider::vector<pisdf::Param *> &params) {
    auto dummyRational = spider::Rational{ 1 };
    for (const auto &edge : edgeArray) {
        const auto *source = edge->source();
        const auto *sink = edge->sink();
        const auto &sourceRate = edge->sourceRateExpression().evaluate(params);
        const auto &sinkRate = edge->sinkRateExpression().evaluate(params);

        /* == Check rates validity == */
        if ((!sinkRate && sourceRate) || (sinkRate && !sourceRate)) {
            throwSpiderException("Invalid rates on edge. Source [%s]: %"
                                         PRIu64
                                         " -- Sink [%s]: %"
                                         PRIu64
                                         ".",
                                 source->name().c_str(),
                                 sourceRate,
                                 sink->name().c_str(),
                                 sinkRate);
        }

        auto &sourceRational =
                source->subtype() == pisdf::VertexType::INPUT ? dummyRational : rationalArray[source->ix()];
        auto &sinkRational = sink->subtype() == pisdf::VertexType::OUTPUT ? dummyRational : rationalArray[sink->ix()];

        if (!sinkRational.nominator() && sinkRate) {
            sinkRational = spider::Rational{ sourceRate, sinkRate };
            if (sourceRational.nominator()) {
                sinkRational *= sourceRational;
            }
        }

        if (!sourceRational.nominator() && sourceRate) {
            sourceRational = spider::Rational{ sinkRate, sourceRate };
            if (sinkRational.nominator()) {
                sourceRational *= sinkRational;
            }
        }
    }
}

void spider::brv::updateBRV(const ConnectedComponent &component, const spider::vector<pisdf::Param *> &params) {
    uint32_t scaleRVFactor{ 1 };

    /* == Compute the scale factor == */
    UpdateBRVVisitor brvVisitor{ scaleRVFactor, params };
    for (const auto &v : component.vertexVector_) {
        for (const auto &edge : v->inputEdgeArray()) {
            edge->source()->visit(&brvVisitor);
        }
        for (const auto &edge : v->outputEdgeArray()) {
            edge->sink()->visit(&brvVisitor);
        }
    }

    /* == Apply the scale factor (if needed) == */
    if (scaleRVFactor > 1) {
        for (const auto &v : component.vertexVector_) {
            v->setRepetitionValue(v->repetitionValue() * scaleRVFactor);
        }
    }
}

void spider::brv::checkConsistency(const spider::array<const pisdf::Edge *> &edgeArray,
                                   const spider::vector<pisdf::Param *> &params) {
    for (const auto &edge : edgeArray) {
        if (edge->source()->subtype() == pisdf::VertexType::INPUT ||
            edge->sink()->subtype() == pisdf::VertexType::OUTPUT) {
            continue;
        }
        const auto &sourceRate = edge->sourceRateExpression().evaluate(params);
        const auto &sinkRate = edge->sinkRateExpression().evaluate(params);
        const auto *source = edge->source();
        const auto *sink = edge->sink();

        if ((sourceRate * source->repetitionValue()) != (sinkRate * sink->repetitionValue())) {
            throwSpiderException("Edge [%s]: prod(%"
                                         PRId64
                                         ") * sourceRV(%"
                                         PRIu32
                                         ") != cons(%"
                                         PRId64
                                         ") * sinkRV(%"
                                         PRIu32
                                         ").",
                                 edge->name().c_str(),
                                 sourceRate,
                                 source->repetitionValue(),
                                 sinkRate,
                                 sink->repetitionValue());
        }
    }
}

void spider::brv::print(const pisdf::Graph *graph) {
    if (api::verbose() && log_enabled<LOG_TRANSFO>()) {
        log::verbose<LOG_TRANSFO>("BRV values for graph [%s]\n", graph->name().c_str());
        for (const auto &vertex : graph->vertices()) {
            log::verbose<LOG_TRANSFO>(">> Vertex: %-20s --> RV[%" PRIu32"]\n", vertex->name().c_str(),
                                      vertex->repetitionValue());
        }
    }
}
