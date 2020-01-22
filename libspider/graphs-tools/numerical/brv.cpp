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

/* === Constant === */
static constexpr size_t DEFAULT_MAX_CC_COUNT = 3;

/* === Static function(s) === */

/**
 * @brief Creates a Connected component from a given seed Vertex.
 * @param vertex   Pointer to the seed Vertex.
 * @param visited  Reference to the vector of visited vertices.
 * @param vertices Reference to the global vector of vertices (shared by all connected components).
 * @return created ConnectedComponent.
 */
static spider::brv::ConnectedComponent
extractOneComponent(spider::pisdf::Vertex *vertex,
                    spider::vector<bool> &visited,
                    spider::vector<spider::pisdf::Vertex *> &vertices) {
    spider::brv::ConnectedComponent component(vertices);
    component.vertexVector_.emplace_back(vertex);

    /* == Extract every vertices of the connected component using a non-recursive BFS algorithm == */
    size_t visitedIndex = component.offsetVertexVector_;
    visited[vertex->ix()] = true;
    while (visitedIndex != component.vertexVector_.size()) {
        auto *currentVertex = component.vertexVector_[visitedIndex++];

        /* == Scan output edges == */
        component.edgeCount_ += currentVertex->outputEdgeCount();
        for (const auto *edge : currentVertex->outputEdgeVector()) {
            if (!edge) {
                throwSpiderException("Vertex [%s] has null output edge.", currentVertex->name().c_str());
            }
            auto *sink = edge->sink();
            auto isOutputIF = (sink->subtype() == spider::pisdf::VertexType::OUTPUT);
            component.hasInterfaces_ |= isOutputIF;
            if (!isOutputIF && !visited[sink->ix()]) {
                /* == Register the vertex == */
                component.hasConfig_ |= (sink->subtype() == spider::pisdf::VertexType::CONFIG);
                component.vertexVector_.emplace_back(sink);
                visited[sink->ix()] = true;
            }
        }

        /* == Scan input edges == */
        for (const auto *edge : currentVertex->inputEdgeVector()) {
            if (!edge) {
                throwSpiderException("Vertex [%s] has null input edge.", currentVertex->name().c_str());
            }
            auto *source = edge->source();
            auto isInputIF = (source->subtype() == spider::pisdf::VertexType::INPUT);
            component.edgeCount_ += isInputIF;
            component.hasInterfaces_ |= isInputIF;
            if (!isInputIF && !visited[source->ix()]) {
                /* == Register the vertex == */
                component.hasConfig_ |= (source->subtype() == spider::pisdf::VertexType::CONFIG);
                component.vertexVector_.emplace_back(source);
                visited[source->ix()] = true;
            }
        }
    }

    /* == Update the number of vertices inside the component == */
    component.vertexCount_ = vertices.size() - component.offsetVertexVector_;
    return component;
}


/**
 * @brief Computes the repetition values of current connected component.
 * @param component       Const reference to current connected component.
 * @param rationalVector  Reference to @refitem Rational vector.
 */
static void computeRepetitionValues(const spider::brv::ConnectedComponent &component,
                                    spider::vector<spider::Rational> &rationalVector) {
    const auto &startIter = component.offsetVertexVector_;
    const auto &endIter = component.offsetVertexVector_ + component.vertexCount_;
    /* == 0. Compute the LCM factor for the current component == */
    int64_t lcmFactor = 1;
    for (auto it = startIter; it < endIter; ++it) {
        const auto &vertex = component.vertexVector_[it];
        lcmFactor = spider::math::lcm(lcmFactor, rationalVector[vertex->ix()].denominator());
    }

    /* == 1. Compute the repetition value for vertices of the connected component == */
    for (auto it = startIter; it < endIter; ++it) {
        const auto &vertex = component.vertexVector_[it];
        rationalVector[vertex->ix()] *= lcmFactor;
        vertex->setRepetitionValue(static_cast<uint32_t>(rationalVector[vertex->ix()].toUInt64()));
    }
}

/* === Function(s) definition === */

void spider::brv::compute(const pisdf::Graph *graph, const spider::vector<std::shared_ptr<pisdf::Param>> &params) {
    auto vertices = containers::vector<pisdf::Vertex *>(StackID::TRANSFO);
    vertices.reserve(graph->vertexCount());
    /* == 0. Extract connected components of the graph == */
    const auto &connectedComponents = extractConnectedComponents(graph, vertices);

    auto registeredEdgeVector = containers::vector<bool>(graph->edgeCount(), false, StackID::TRANSFO);
    auto rationalVector = containers::vector<Rational>(graph->vertexCount(), StackID::TRANSFO);
    /* == 1. For each connected component compute LCM and RV == */
    for (const auto &component : connectedComponents) {
        /* == 1.1 Extract the edges == */
        auto edgeArray = extractEdgesFromComponent(component, registeredEdgeVector);

        /* == 1.1.1 If there is no edge at all and this is normal then all RV are 1 by default == */
        if (edgeArray.empty() && !component.edgeCount_) {
            continue;
        }

        /* == 1.2 Compute the Rationals == */
        extractRationalsFromEdges(rationalVector, edgeArray, params);

        /* == 1.3 Compute the repetition values for the current component == */
        computeRepetitionValues(component, rationalVector);

        /* == 1.4 Update repetition values based on PiSDF rules of input / output interfaces and config actors == */
        updateBRV(component, params);

        /* == 1.5 Check consistency of the connected component == */
        checkConsistency(edgeArray, params);
    }

    /* == Print BRV (if VERBOSE) == */
    print(graph);
}

void spider::brv::compute(const pisdf::Graph *graph) {
    compute(graph, graph->params());
}

spider::vector<spider::brv::ConnectedComponent>
spider::brv::extractConnectedComponents(const pisdf::Graph *graph, spider::vector<pisdf::Vertex *> &vertices) {
    /* == Keys used to check if a vertex has already be assigned to a connected component == */
    auto visited = containers::vector<bool>(graph->vertexCount(), false, StackID::TRANSFO);

    /* == Iterate over every vertex and assign it to a connected component == */
    auto connectedComponents = containers::vector<ConnectedComponent>(StackID::TRANSFO);
    /* == We assume 3 ConnectedComponents as a basis, this may be superior or inferior but this will save some time == */
    connectedComponents.reserve(DEFAULT_MAX_CC_COUNT);
    for (const auto &vertex : graph->vertices()) {
        if (!visited[vertex->ix()]) {
            /* == Extract the component into output vector == */
            connectedComponents.emplace_back(extractOneComponent(vertex, visited, vertices));
        }
    }
    return connectedComponents;
}

spider::array<const spider::pisdf::Edge *>
spider::brv::extractEdgesFromComponent(const ConnectedComponent &component,
                                       spider::vector<bool> &registeredEdgeVector) {
    auto edgeArray = spider::array<const pisdf::Edge *>{ component.edgeCount_, StackID::TRANSFO };
    size_t index = 0;
    auto start = component.offsetVertexVector_;
    auto end = component.offsetVertexVector_ + component.vertexCount_;
    for (size_t it = start; it < end; ++it) {
        const auto &vertex = component.vertexVector_[it];
        for (const auto &edge: vertex->outputEdgeVector()) {
            if (!registeredEdgeVector[edge->ix()]) {
                edgeArray[index++] = edge;
                registeredEdgeVector[edge->ix()] = true;
            }
        }
        for (const auto &edge: vertex->inputEdgeVector()) {
            if (!registeredEdgeVector[edge->ix()]) {
                edgeArray[index++] = edge;
                registeredEdgeVector[edge->ix()] = true;
            }
        }
    }
    return edgeArray;
}

void spider::brv::extractRationalsFromEdges(spider::vector<Rational> &rationalVector,
                                            const spider::array<const pisdf::Edge *> &edgeArray,
                                            const spider::vector<std::shared_ptr<pisdf::Param>> &params) {
    auto dummyRational = Rational{ 1 };
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
                source->subtype() == pisdf::VertexType::INPUT ? dummyRational : rationalVector[source->ix()];
        auto &sinkRational = sink->subtype() == pisdf::VertexType::OUTPUT ? dummyRational : rationalVector[sink->ix()];

        if (!sinkRational.nominator() && sinkRate) {
            sinkRational = Rational{ sourceRate, sinkRate };
            if (sourceRational.nominator()) {
                sinkRational *= sourceRational;
            }
        }

        if (!sourceRational.nominator() && sourceRate) {
            sourceRational = Rational{ sinkRate, sourceRate };
            if (sinkRational.nominator()) {
                sourceRational *= sinkRational;
            }
        }
    }
}

void spider::brv::updateBRV(const ConnectedComponent &component,
                            const spider::vector<std::shared_ptr<pisdf::Param>> &params) {
    uint32_t scaleRVFactor{ 1 };
    if (!component.hasConfig_ && !component.hasInterfaces_) {
        return;
    }

    const auto &startIter = component.offsetVertexVector_;
    const auto &endIter = component.offsetVertexVector_ + component.vertexCount_;
    /* == Compute the scale factor == */
    UpdateBRVVisitor brvVisitor{ scaleRVFactor, params };
    for (auto it = startIter; it < endIter; ++it) {
        const auto &v = component.vertexVector_[it];
        for (const auto &edge : v->inputEdgeVector()) {
            edge->source()->visit(&brvVisitor);
        }
        for (const auto &edge : v->outputEdgeVector()) {
            edge->sink()->visit(&brvVisitor);
        }
    }

    /* == Apply the scale factor (if needed) == */
    if (scaleRVFactor > 1) {
        for (auto it = startIter; it < endIter; ++it) {
            const auto &v = component.vertexVector_[it];
            v->setRepetitionValue(v->repetitionValue() * scaleRVFactor);
        }
    }
}

void spider::brv::checkConsistency(const spider::array<const pisdf::Edge *> &edgeArray,
                                   const spider::vector<std::shared_ptr<pisdf::Param>> &params) {
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
    if (api::verbose() && log::enabled<log::Type::TRANSFO>()) {
        const auto &separation = std::string(46, '-');
        log::verbose<log::Type::TRANSFO>("%s\n", separation.c_str());
        log::verbose<log::Type::TRANSFO>("Repetition values for graph [%s]\n", graph->name().c_str());
        for (const auto &vertex : graph->vertices()) {
            log::verbose<log::Type::TRANSFO>("    >> Vertex: %-30s --> [%" PRIu32"]\n",
                                             vertex->name().c_str(),
                                             vertex->repetitionValue());
        }
        log::verbose<log::Type::TRANSFO>("%s\n", separation.c_str());
    }
}
