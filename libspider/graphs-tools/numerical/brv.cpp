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
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Param.h>

/* === Static function(s) === */
namespace spider {
    namespace brv {
        /**
         * @brief Pre-compute rates of every edge of a given graph.
         * @param graph   Pointer to the graph.
         * @param params  Const reference to the vector of parameter to use for the evaluation of rates.
         * @return vector of pair of rates (first is source rate, second is sink rate).
         */
        static vector<std::pair<i64, i64>> preComputeEdgeRates(const pisdf::Graph *graph,
                                                               const vector<std::shared_ptr<pisdf::Param>> &params) {
            auto result = factory::vector<std::pair<i64, i64>>(graph->edgeCount(), StackID::TRANSFO);
            for (const auto &edge : graph->edges()) {
                result[edge->ix()].first = edge->sourceRateExpression().evaluate(params);
                result[edge->ix()].second = edge->sinkRateExpression().evaluate(params);
            }
            return result;
        }

        static void updateRational(const pisdf::Edge *edge,
                                   const vector<std::pair<i64, i64>> &rates,
                                   vector<Rational> &rationalVector) {
            auto dummyRational = Rational{ 1 };
            const auto *source = edge->source();
            const auto *sink = edge->sink();
            const auto sourceRate = rates[edge->ix()].first;
            const auto sinkRate = rates[edge->ix()].second;
            /* == Check rates validity == */
            if ((!sinkRate && sourceRate) || (sinkRate && !sourceRate)) {
                throwSpiderException("Invalid rates on edge. Source [%s]: %" PRIu64" -- Sink [%s]: %" PRIu64".",
                                     source->name().c_str(), sourceRate, sink->name().c_str(), sinkRate);
            }
            auto &sourceRational = source->subtype() == pisdf::VertexType::INPUT ? dummyRational
                                                                                 : rationalVector[source->ix()];
            auto &sinkRational = sink->subtype() == pisdf::VertexType::OUTPUT ? dummyRational
                                                                              : rationalVector[sink->ix()];
            if (sinkRate && !sinkRational.nominator()) {
                sinkRational = Rational{ sourceRate, sinkRate };
                if (sourceRational.nominator()) {
                    sinkRational *= sourceRational;
                }
            }
            if (sourceRate && !sourceRational.nominator()) {
                sourceRational = Rational{ sinkRate, sourceRate };
                if (sinkRational.nominator()) {
                    sourceRational *= sinkRational;
                }
            }
        }

        /**
         * @brief Creates a Connected component from a given seed Vertex.
         * @param vertex   Pointer to the seed Vertex.
         * @param rates    Const reference to the vector of pre-computed rates.
         * @param handler  Reference to the BRV handler.
         * @return created ConnectedComponent.
         */
        static ConnectedComponent extractConnectedComponent(pisdf::Vertex *vertex,
                                                            const vector<std::pair<i64, i64>> &rates,
                                                            BRVHandler &handler) {
            auto component = ConnectedComponent{ };
            const auto offset = std::distance(std::begin(handler.vertexVector_), std::end(handler.vertexVector_));
            handler.vertexVector_.emplace_back(vertex);
            handler.visitedVertices_[vertex->ix()] = true;
            auto visitedIndex = static_cast<size_t>(offset);
            while (visitedIndex != handler.vertexVector_.size()) {
                const auto *currentVertex = handler.vertexVector_[visitedIndex++];
                component.edgeCount_ += currentVertex->outputEdgeCount();
                component.hasConfig_ |= (currentVertex->subtype() == pisdf::VertexType::CONFIG);
                for (const auto *edge : currentVertex->outputEdgeVector()) {
                    if (!edge) {
                        throwSpiderException("Vertex [%s] has null output edge.", currentVertex->name().c_str());
                    } else if (!handler.visitedEdges_[edge->ix()]) {
                        handler.visitedEdges_[edge->ix()] = true;
                        auto *sink = edge->sink();
                        const auto isOutputIf = (sink->subtype() == pisdf::VertexType::OUTPUT);
                        component.hasInterfaces_ |= isOutputIf;
                        updateRational(edge, rates, handler.rationalVector_);
                        if (!isOutputIf && !handler.visitedVertices_[sink->ix()]) {
                            /* == Register the vertex == */
                            handler.vertexVector_.emplace_back(sink);
                            handler.visitedVertices_[sink->ix()] = true;
                        }
                    }
                }
                for (const auto *edge : currentVertex->inputEdgeVector()) {
                    if (!edge) {
                        throwSpiderException("Vertex [%s] has null input edge.", currentVertex->name().c_str());
                    } else if (!handler.visitedEdges_[edge->ix()]) {
                        handler.visitedEdges_[edge->ix()] = true;
                        auto *source = edge->source();
                        const auto isInputIf = (source->subtype() == pisdf::VertexType::INPUT);
                        component.edgeCount_ += isInputIf;
                        component.hasInterfaces_ |= isInputIf;
                        updateRational(edge, rates, handler.rationalVector_);
                        if (!isInputIf && !handler.visitedVertices_[source->ix()]) {
                            /* == Register the vertex == */
                            handler.vertexVector_.emplace_back(source);
                            handler.visitedVertices_[source->ix()] = true;
                        }
                    }
                }
            }
            component.startIt_ = std::begin(handler.vertexVector_) + offset;
            component.endIt_ = std::end(handler.vertexVector_);
            return component;
        }

        /**
         * @brief Computes the repetition values of current connected component.
         * @param component       Const reference to current connected component.
         * @param handler         Reference to the BRV handler.
         */
        static void computeRepetitionValues(const ConnectedComponent &component, BRVHandler &handler) {
            /* == 0. Compute the LCM factor for the current component == */
            auto lcmFactor = int64_t{ 1 };
            for (auto it = component.startIt_; it < component.endIt_; ++it) {
                lcmFactor = math::lcm(lcmFactor, handler.rationalVector_[(*it)->ix()].denominator());
            }

            /* == 1. Compute the repetition value for vertices of the connected component == */
            std::for_each(component.startIt_, component.endIt_, [&lcmFactor, &handler](pisdf::Vertex *vertex) {
                handler.rationalVector_[vertex->ix()] *= lcmFactor;
                vertex->setRepetitionValue(static_cast<u32>(handler.rationalVector_[vertex->ix()].toUInt64()));
            });
        }

        /**
         * @brief Update repetition vector values of a connected component based on PiSDF rules.
         * @param component  Const reference to the connected component.
         * @param rates      Const reference to the vector of pre-computed rates.
         */
        static void updateComponentBRV(const ConnectedComponent &component, const vector<std::pair<i64, i64>> &rates) {
            const auto graph = (*component.startIt_)->graph();
            u32 scaleRVFactor{ 1 };
            const auto updateInput = [&scaleRVFactor, &rates](const pisdf::Edge *edge) {
                const auto sourceRate = rates[edge->ix()].first;
                const auto sinkRate = rates[edge->ix()].second;
                const auto totalCons = sinkRate * edge->sink()->repetitionValue() * scaleRVFactor;
                if (totalCons && totalCons < sourceRate) {
                    /* == Return ceil( prod / vertexCons) == */
                    scaleRVFactor *= static_cast<u32>(math::ceilDiv(sourceRate, totalCons));
                }
            };
            if (component.hasConfig_) {
                for (const auto &cfg : graph->configVertices()) {
                    for (const auto &edge : cfg->outputEdgeVector()) {
                        updateInput(edge);
                    }
                }
            }
            if (component.hasInterfaces_) {
                for (const auto &input : graph->inputInterfaceVector()) {
                    updateInput(input->outputEdge());
                }
                for (const auto &output : graph->outputInterfaceVector()) {
                    const auto edge = output->inputEdge();
                    const auto sourceRate = rates[edge->ix()].first;
                    const auto sinkRate = rates[edge->ix()].second;
                    const auto totalProd = sourceRate * edge->source()->repetitionValue() * scaleRVFactor;
                    if (totalProd && totalProd < sinkRate) {
                        /* == Return ceil(interfaceCons / vertexProd) == */
                        scaleRVFactor *= static_cast<u32>(math::ceilDiv(sinkRate, totalProd));
                    }
                }
            }
            /* == Apply the scale factor (if needed) == */
            if (scaleRVFactor > 1) {
                std::for_each(component.startIt_, component.endIt_, [scaleRVFactor](pisdf::Vertex *vertex) {
                    vertex->setRepetitionValue(vertex->repetitionValue() * scaleRVFactor);
                });
            }
        }

        /**
         * @brief Check the consistency (see consistency property of SDF graph) of a connected component.
         * @param component  Const reference to the connected component.
         * @param rates      Const reference to the vector of pre-computed rates.
         * @throws @refitem spider::Exception.
         */
        static void checkConsistency(const ConnectedComponent &component, const vector<std::pair<i64, i64>> &rates) {
            for (auto it = component.startIt_; it < component.endIt_; ++it) {
                for (const auto &edge : (*it)->outputEdgeVector()) {
                    if (edge->sink()->subtype() == pisdf::VertexType::OUTPUT) {
                        continue;
                    }
                    const auto sourceRate = rates[edge->ix()].first;
                    const auto sinkRate = rates[edge->ix()].second;
                    const auto *source = edge->source();
                    const auto *sink = edge->sink();
                    if ((sourceRate * source->repetitionValue()) != (sinkRate * sink->repetitionValue())) {
                        throwSpiderException(
                                "Edge [%s]: prod(%" PRId64") * sourceRV(%" PRIu32") != cons(%" PRId64") * sinkRV(%"
                                PRIu32").", edge->name().c_str(), sourceRate, source->repetitionValue(),
                                sinkRate, sink->repetitionValue());
                    }
                }
            }
        }
    }
}

/* === Function(s) definition === */

void spider::brv::compute(const pisdf::Graph *graph, const vector<std::shared_ptr<pisdf::Param>> &params) {
    auto handler = BRVHandler{ graph->vertexCount(), graph->edgeCount() };
    /* == 0. Pre-compute rates == */
    const auto preComputedEdgeRates = preComputeEdgeRates(graph, params);

    /* == 1. Iterate over all vertices == */
    for (const auto &vertex : graph->vertices()) {
        if (!handler.visitedVertices_[vertex->ix()]) {
            /* == 2. Extract current connected component == */
            const auto component = extractConnectedComponent(vertex.get(), preComputedEdgeRates, handler);
            /* == 2.1 If there are no edges then RV is supposed to be 1 == */
            if (!component.edgeCount_) {
                continue;
            }
            /* == 3. Compute Repetition vector for current connected component == */
            computeRepetitionValues(component, handler);
            /* == 4. Update repetition vector based on PiSDF rules == */
            if (component.hasConfig_ || component.hasInterfaces_) {
                updateComponentBRV(component, preComputedEdgeRates);
            }
            /* == 5. Check graph consistency == */
            checkConsistency(component, preComputedEdgeRates);
        }
    }
    /* == Print BRV (if VERBOSE) == */
    print(graph);
}

void spider::brv::compute(const pisdf::Graph *graph) {
    compute(graph, graph->params());
}

void spider::brv::print(const pisdf::Graph *graph) {
    if (log::enabled<log::TRANSFO>()) {
        const auto &separation = std::string(46, '-');
        log::verbose<log::TRANSFO>("%s\n", separation.c_str());
        log::verbose<log::TRANSFO>("Repetition values for graph [%s]\n", graph->name().c_str());
        for (const auto &vertex : graph->vertices()) {
            log::verbose<log::TRANSFO>("    >> Vertex: %-30s --> [%" PRIu32"]\n",
                                       vertex->name().c_str(),
                                       vertex->repetitionValue());
        }
        log::verbose<log::TRANSFO>("%s\n", separation.c_str());
    }
}
