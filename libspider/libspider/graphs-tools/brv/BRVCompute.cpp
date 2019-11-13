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

#include <graphs-tools/brv/BRVCompute.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/Edge.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

BRVCompute::BRVCompute(const PiSDFGraph *graph) : BRVCompute(graph, graph->params()) {
}

BRVCompute::BRVCompute(const PiSDFGraph *graph, const Spider::vector<PiSDFParam *> &params) : graph_{ graph },
                                                                                              params_{ params } {
    Spider::Array<const PiSDFAbstractVertex *> connectedComponentsKeys{ graph->vertexCount(), nullptr,
                                                                        StackID::TRANSFO };
    Spider::Array<const PiSDFAbstractVertex *> vertexArray{ graph->vertexCount(), nullptr, StackID::TRANSFO };
    BRVComponent component;
    for (auto *v:graph->vertices()) {
        if (!connectedComponentsKeys[v->ix()]) {
            component.nEdges = 0;
            component.vertices.clear();
            /* == Register current vertex == */
            connectedComponentsKeys[v->ix()] = v;
            component.vertices.push_back(v);

            /* == Extract the connected component vertices == */
            extractConnectedComponent(component, connectedComponentsKeys);
            connectedComponents_.push_back((component));
        }
    }
}

/* === Private Method(s) implementation === */

void BRVCompute::extractConnectedComponent(BRVComponent &component,
                                           Spider::Array<const PiSDFAbstractVertex *> &keyArray) {
    auto scannedIndex = component.vertices.size() - 1; /* = Index of current scanned vertex = */
    bool addedVertex;
    do {
        addedVertex = false;
        auto *currentVertex = component.vertices[scannedIndex];

        /* == Scan output edges == */
        for (const auto *edge : currentVertex->outputEdgeArray()) {
            if (!edge) {
                throwSpiderException("Vertex [%s] has null edge.", currentVertex->name().c_str());
            }
            auto *sink = edge->sink();
            if (sink->type() != Spider::PiSDF::VertexType::INTERFACE &&
                !keyArray[sink->ix()]) {
                /* == Register the vertex == */
                component.vertices.push_back(sink);
                keyArray[sink->ix()] = sink;
                addedVertex = true;
            }
            component.nEdges += 1;
        }

        /* == Scan input edges == */
        for (const auto *edge : currentVertex->inputEdgeArray()) {
            if (!edge) {
                throwSpiderException("Vertex [%s] has null edge.", currentVertex->name().c_str());
            }
            auto *source = edge->source();
            if (source->type() != Spider::PiSDF::VertexType::INTERFACE &&
                !keyArray[source->ix()]) {
                /* == Register the vertex == */
                component.vertices.push_back(source);
                keyArray[source->ix()] = source;
                addedVertex = true;
            } else if (source->type() == Spider::PiSDF::VertexType::INTERFACE) {
                component.nEdges += 1;
            }
        }
        scannedIndex += 1;
    } while (addedVertex || scannedIndex != component.vertices.size());
}

void BRVCompute::extractEdges(Spider::Array<const PiSDFEdge *> &edgeArray, const BRVComponent &component) {
    const auto &vertexArray = component.vertices;
    std::uint32_t index = 0;
    for (const auto &v: vertexArray) {
        for (const auto &edge: v->outputEdgeArray()) {
            edgeArray[index++] = edge;
        }
        for (const auto &edge: v->inputEdgeArray()) {
            if (edge->source()->type() == PiSDFVertexType::INTERFACE) {
                edgeArray[index++] = edge;
            }
        }
    }
}

void BRVCompute::updateBRV(Spider::Array<const PiSDFEdge *> &edgeArray, const BRVComponent &component) {
    std::uint64_t scaleRVFactor{ 1 };

    /* == Compute the scale factor == */
    for (const auto &edge : edgeArray) {
        if (edge->source()->type() == PiSDFVertexType::INTERFACE) {
            scaleRVFactor *= updateBRVFromInputIF(edge, scaleRVFactor);
        } else if (edge->sink()->type() == PiSDFVertexType::INTERFACE) {
            scaleRVFactor *= updateBRVFromOutputIF(edge, scaleRVFactor);
        } else if (edge->source()->type() == PiSDFVertexType::CONFIG) {
            scaleRVFactor *= updateBRVFromCFGActor(edge, scaleRVFactor);
        }
    }

    /* == Apply the scale factor (if needed) == */
    if (scaleRVFactor > 1) {
        for (const auto &v : component.vertices) {
            v->setRepetitionValue(v->repetitionValue() * scaleRVFactor);
        }
    }
}

std::uint64_t BRVCompute::updateBRVFromInputIF(const PiSDFEdge *edge, std::uint64_t currentScaleFactor) const {
    if (edge->sink()->type() != PiSDFVertexType::INTERFACE) {
        std::uint64_t sourceRate = edge->sourceRateExpression().evaluate(params_);
        std::uint64_t sinkRate = edge->sinkRateExpression().evaluate(params_);
        auto totalCons = sinkRate * edge->sink()->repetitionValue() * currentScaleFactor;
        if (totalCons && totalCons < sourceRate) {
            /* == Return ceil(interfaceProd / vertexCons) == */
            return Spider::Math::ceilDiv(sourceRate, totalCons);
        }
    }
    return 1;
}

std::uint64_t BRVCompute::updateBRVFromOutputIF(const PiSDFEdge *edge, std::uint64_t currentScaleFactor) const {
    if (edge->source()->type() != PiSDFVertexType::INTERFACE) {
        std::uint64_t sourceRate = edge->sourceRateExpression().evaluate(params_);
        std::uint64_t sinkRate = edge->sinkRateExpression().evaluate(params_);
        auto totalProd = sourceRate * edge->source()->repetitionValue() * currentScaleFactor;
        if (totalProd && totalProd < sinkRate) {
            /* == Return ceil(vertexProd / interfaceCons) == */
            return Spider::Math::ceilDiv(sinkRate, totalProd);
        }
    }
    return 1;
}

std::uint64_t BRVCompute::updateBRVFromCFGActor(const PiSDFEdge *edge, std::uint64_t currentScaleFactor) const {
    if (edge->sink()->type() != PiSDFVertexType::INTERFACE) {
        std::uint64_t sourceRate = edge->sourceRateExpression().evaluate(params_);
        std::uint64_t sinkRate = edge->sinkRateExpression().evaluate(params_);
        auto totalCons = sinkRate * edge->sink()->repetitionValue() * currentScaleFactor;
        if (totalCons && totalCons < sourceRate) {
            /* == Return ceil(cfgActorProd / vertexCons) == */
            return Spider::Math::ceilDiv(sourceRate, totalCons);
        }
    }
    return 1;
}

void BRVCompute::print() const {
    using namespace Spider;
    if (API::verbose()) {
        Logger::verbose(LOG_TRANSFO, "BRV values for graph [%s]\n", graph_->name().c_str());
        for (const auto &vertex : graph_->vertices()) {
            Logger::verbose(LOG_TRANSFO, ">> Vertex: %-20s --> RV[%" PRIu32"]\n", vertex->name().c_str(),
                            vertex->repetitionValue());
        }
    }
}
