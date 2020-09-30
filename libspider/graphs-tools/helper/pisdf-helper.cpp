/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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

#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs-tools/transformation/srdag/Transformation.h>
#include <api/pisdf-api.h>

/* === Static function(s) === */

template<class InterfaceVector, class EdgeReconnector>
static ufast32 reconnectInterface(const InterfaceVector &interfaceVector, const EdgeReconnector &reconnector) {
    ufast32 ix = 0;
    for (auto &interface : interfaceVector) {
        const auto &vertex = interface->opposite();
        if (vertex->subtype() != spider::pisdf::VertexType::CONFIG) {
            auto *edge = reconnector(interface.get(), interface->edge(), ix);
            interface->graph()->addEdge(edge);
            ix++;
        }
    }
    return ix;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of a normal vertex.
 * @param vertex Pointer to the vertex.
 * @return array of int_least_64_t.
 */
static spider::unique_ptr<i64> buildDefaultVertexRuntimeParameters(const spider::pisdf::Vertex *vertex) {
    const auto &inputParams = vertex->refinementParamVector();
    auto outParams = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(inputParams.size()));
    std::transform(std::begin(inputParams), std::end(inputParams), outParams.get(),
                   [](const std::shared_ptr<spider::pisdf::Param> &param) { return param->value(); });
    return outParams;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::FORK
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @param params Parameters to use for the rates evaluation. (should contain the same parameters as the graphs)
 * @return array of int_least_64_t.
 */
static spider::unique_ptr<i64> buildForkRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
                                                               const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params) {
    const auto &outputEdges = vertex->outputEdgeVector();
    auto outParams = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(outputEdges.size() + 2));
    outParams.get()[0] = vertex->inputEdge(0)->sinkRateExpression().evaluate(params);
    outParams.get()[1] = static_cast<i64>(outputEdges.size());
    std::transform(std::begin(outputEdges), std::end(outputEdges), std::next(outParams.get(), 2),
                   [&params](const spider::pisdf::Edge *edge) {
                       return edge->sourceRateExpression().evaluate(params);
                   });
    return outParams;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::JOIN
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @param params Parameters to use for the rates evaluation. (should contain the same parameters as the graphs)
 * @return array of int_least_64_t.
 */
static spider::unique_ptr<i64> buildJoinRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
                                                               const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params) {
    const auto &inputEdges = vertex->inputEdgeVector();
    auto outParams = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(inputEdges.size() + 2));
    outParams.get()[0] = vertex->outputEdge(0)->sourceRateExpression().evaluate(params);
    outParams.get()[1] = static_cast<i64>(inputEdges.size());
    std::transform(std::begin(inputEdges), std::end(inputEdges), std::next(outParams.get(), 2),
                   [&params](const spider::pisdf::Edge *edge) { return edge->sinkRateExpression().evaluate(params); });
    return outParams;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::TAIL
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @param params Parameters to use for the rates evaluation. (should contain the same parameters as the graphs)
 * @return array of int_least_64_t.
 */
static spider::unique_ptr<i64> buildTailRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
                                                               const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params) {
    size_t inputCount = 1;
    auto rate = vertex->outputEdge(0)->sourceRateExpression().evaluate(params);
    const auto &inputEdges = vertex->inputEdgeVector();
    for (auto it = inputEdges.rbegin(); it != inputEdges.rend(); ++it) {
        const auto &inRate = (*it)->sinkRateExpression().evaluate(params);
        if (inRate >= rate) {
            break;
        }
        rate -= inRate;
        inputCount++;
    }
    auto outParams = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(inputCount + 4u));
    /* = Number of input = */
    outParams.get()[0] = static_cast<i64>(inputEdges.size());
    /* = First input to be considered = */
    outParams.get()[1] = static_cast<i64>(inputEdges.size() - inputCount);
    /* = Offset in the first buffer if any = */
    outParams.get()[2] =
            vertex->inputEdge(inputEdges.size() - inputCount)->sinkRateExpression().evaluate(params) - rate;
    /* = Effective size to copy of the first input = */
    outParams.get()[3] = rate;
    size_t i = 4;
    for (auto it = vertex->inputEdgeVector().rbegin();
         it != vertex->inputEdgeVector().rbegin() + static_cast<long>(inputCount) - 1; ++it) {
        outParams.get()[i++] = (*it)->sinkRateExpression().evaluate(params);
    }
    return outParams;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::HEAD
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @param params Parameters to use for the rates evaluation. (should contain the same parameters as the graphs)
 * @return array of int_least_64_t.
 */
static spider::unique_ptr<i64> buildHeadRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
                                                               const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params) {
    size_t inputCount = 1;
    auto rate = vertex->outputEdge(0)->sourceRateExpression().evaluate(params);
    for (auto &edge : vertex->inputEdgeVector()) {
        const auto &inRate = edge->sinkRateExpression().evaluate(params);
        if (inRate >= rate) {
            break;
        }
        rate -= inRate;
        inputCount++;
    }
    auto outParams = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(inputCount + 1u));
    outParams.get()[0] = static_cast<i64>(inputCount);
    rate = vertex->outputEdge(0)->sourceRateExpression().evaluate(params);
    for (size_t i = 0; i < inputCount; ++i) {
        const auto &inRate = vertex->inputEdge(i)->sinkRateExpression().evaluate(params);
        outParams.get()[i + 1] = std::min(inRate, rate);
        rate -= inRate;
    }
    return outParams;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::REPEAT
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @param params Parameters to use for the rates evaluation. (should contain the same parameters as the graphs)
 * @return array of int_least_64_t.
 */
static spider::unique_ptr<i64> buildRepeatRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
                                                                 const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params) {
    auto outParams = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(2u));
    outParams.get()[0] = vertex->inputEdge(0)->sinkRateExpression().evaluate(params);
    outParams.get()[1] = vertex->outputEdge(0)->sourceRateExpression().evaluate(params);
    return outParams;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::DUPLICATE
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @param params Parameters to use for the rates evaluation. (should contain the same parameters as the graphs)
 * @return array of int_least_64_t.
 */
static spider::unique_ptr<i64> buildDuplicateRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
                                                                    const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params) {
    auto outParams = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(2u));
    outParams.get()[0] = static_cast<i64>(vertex->outputEdgeCount());
    outParams.get()[1] = vertex->inputEdge(0)->sinkRateExpression().evaluate(params);
    return outParams;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::INIT special vertex.
 * @param vertex Pointer to the @refitem pisdf::Vertex associated with the delay.
 * @return array of int_least_64_t.
 */
static spider::unique_ptr<i64> buildInitRuntimeInputParameters(const spider::pisdf::Vertex *vertex) {
    auto outParams = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(3u));
    const auto *reference = vertex->reference();
    const auto *delayVertex = reference->outputEdge(0u)->sink()->convertTo<spider::pisdf::DelayVertex>();
    const auto *delay = delayVertex->delay();
    outParams.get()[0] = delay->isPersistent();                    /* = Persistence property = */
    outParams.get()[1] = delay->value();                           /* = Value of the delay = */
    outParams.get()[2] = static_cast<i64>(delay->memoryAddress()); /* = Memory address (may be unused) = */
    return outParams;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::END special vertex.
 * @param vertex Pointer to the @refitem pisdf::Vertex associated with the delay.
 * @return array of int_least_64_t.
 */
static spider::unique_ptr<i64> buildEndRuntimeInputParameters(const spider::pisdf::Vertex *vertex) {
    auto outParams = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(3u));
    const auto *reference = vertex->reference();
    const auto *delayVertex = reference->inputEdge(0u)->source()->convertTo<spider::pisdf::DelayVertex>();
    const auto *delay = delayVertex->delay();
    outParams.get()[0] = delay->isPersistent();                    /* = Persistence property = */
    outParams.get()[1] = delay->value();                           /* = Value of the delay = */
    outParams.get()[2] = static_cast<i64>(delay->memoryAddress()); /* = Memory address (may be unused) = */
    return outParams;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::EXTERN_OUT
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @param params Parameters to use for the rates evaluation. (should contain the same parameters as the graphs)
 * @return array of int_least_64_t.
 */
static spider::unique_ptr<i64> buildExternOutRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
                                                                    const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params) {
    auto outParams = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(2u));
    const auto *reference = vertex->reference()->convertTo<spider::pisdf::ExternInterface>();
    const auto *inputEdge = vertex->inputEdge(0);
    outParams.get()[0] = static_cast<i64>(reference->bufferIndex());
    outParams.get()[1] = inputEdge->sinkRateExpression().evaluate(params);
    return outParams;
}

/* === Function(s) definition === */

bool spider::pisdf::isGraphFullyStatic(const Graph *graph) {
    if (!graph) {
        return false;
    }
    bool isFullyStatic = !graph->dynamic();
    if (isFullyStatic) {
        for (const auto &subgraph : graph->subgraphs()) {
            isFullyStatic &= isGraphFullyStatic(subgraph);
            if (!isFullyStatic) {
                break;
            }
        }
    }
    return isFullyStatic;
}

void spider::pisdf::separateRunGraphFromInit(Graph *graph) {
    if (!graph->configVertexCount() || !graph->dynamic()) {
        return;
    }

    /* == Compute the input interface count for both graphs == */
    ufast32 cfg2OutputIfCount = 0;
    ufast32 cfg2RunIfCount = 0;
    ufast32 inputIf2CfgCount = 0;
    for (const auto &cfg : graph->configVertices()) {
        for (const auto &edge : cfg->inputEdgeVector()) {
            const auto &source = edge->source();
            if (source->subtype() != pisdf::VertexType::INPUT) {
                throwSpiderException("Config vertex can not have source of type other than interface.");
            }
            inputIf2CfgCount++;
        }
        for (const auto &edge : cfg->outputEdgeVector()) {
            auto isOutputIf = edge->sink()->subtype() == pisdf::VertexType::OUTPUT;
            cfg2OutputIfCount += (isOutputIf);
            cfg2RunIfCount += (!isOutputIf);
        }
    }
    const auto runInputIfCount = graph->inputEdgeCount() + cfg2RunIfCount - inputIf2CfgCount;
    const auto runOutputIfCount = graph->outputEdgeCount() - cfg2OutputIfCount;

    /* == Create the run subgraph == */
    auto *runGraph = api::createGraph("run", graph->vertexCount(), graph->edgeCount(), graph->paramCount(),
                                      runInputIfCount, runOutputIfCount);

    /* == Move the edges == */
    auto itEdge = graph->edges().begin();
    while (graph->edgeCount() != (inputIf2CfgCount + cfg2OutputIfCount)) {
        const auto *source = itEdge->get()->source();
        const auto *sink = itEdge->get()->sink();
        if (sink->subtype() == pisdf::VertexType::CONFIG ||
            (sink->subtype() == pisdf::VertexType::OUTPUT &&
             source->subtype() == pisdf::VertexType::CONFIG)) {
            itEdge++;
        } else {
            graph->moveEdge(itEdge->get(), runGraph);
            itEdge = graph->edges().begin();
        }
    }

    /* == Move the subgraphs == */
    while (!graph->subgraphs().empty()) {
        graph->moveVertex(graph->subgraphs()[0], runGraph);
    }

    /* == Move the vertices == */
    auto itVertex = graph->vertices().begin();
    while (graph->vertexCount() != graph->configVertexCount()) {
        if ((*itVertex)->subtype() == pisdf::VertexType::CONFIG) {
            itVertex++;
        } else {
            graph->moveVertex(itVertex->get(), runGraph);
            itVertex = graph->vertices().begin();
        }
    }

    /* == Add run graph == */
    graph->addVertex(runGraph);

    /* == Reconnect Edges from input interfaces == */
    auto inputRunIx = reconnectInterface(graph->inputInterfaceVector(),
                                         [&runGraph](pisdf::Interface *input, pisdf::Edge *edge, ufast32 ix) {
                                             auto expr = edge->sourceRateExpression();
                                             /* == Change source of original edge to run graph interface == */
                                             edge->setSource(runGraph->inputInterface(ix), 0u, expr);
                                             edge->source()->setName(input->name());
                                             /* == Create an edge with the original interface == */
                                             return make<pisdf::Edge, StackID::PISDF>(input, 0u, expr,
                                                                                      runGraph, ix, std::move(expr));
                                         });

    /* == Reconnect Edges from output interfaces == */
    reconnectInterface(graph->outputInterfaceVector(),
                       [&runGraph](pisdf::Interface *output, pisdf::Edge *edge, ufast32 ix) {
                           auto expr = edge->sinkRateExpression();
                           /* == Change sink of original edge to run graph interface == */
                           edge->setSink(runGraph->outputInterface(ix), 0u, expr);
                           edge->sink()->setName(output->name());
                           /* == Create an edge with the original interface == */
                           return make<pisdf::Edge, StackID::PISDF>(runGraph, ix, expr,
                                                                    output, 0u, std::move(expr));
                       });

    /* == Connect the output edges of config vertices == */
    for (auto &cfg : graph->configVertices()) {
        for (auto edge : cfg->outputEdgeVector()) {
            const auto &sink = edge->sink();
            if (sink->subtype() != pisdf::VertexType::OUTPUT) {
                const auto srcRate = edge->sourceRateValue(); /* = Config actors can not have dynamic rate = */
                const auto srcPortIx = edge->sourcePortIx();
                /* == Connect input interface to vertex in run graph == */
                auto *input = runGraph->inputInterface(inputRunIx);
                edge->setSource(input, 0, edge->sourceRateExpression());
                input->setName(cfg->name() + "::out:" + std::to_string(srcPortIx));
                /* == Connect cfg to run graph == */
                api::createEdge(cfg, edge->sourcePortIx(), srcRate, runGraph, inputRunIx, srcRate);
                inputRunIx += 1;
            }
        }
    }

    /* == Copy the params to the run graph == */
    auto paramsToRemove = factory::vector<std::shared_ptr<pisdf::Param>>(StackID::TRANSFO);
    for (auto &param : graph->params()) {
        api::createInheritedParam(runGraph, param->name(), param);
    }
}

void spider::pisdf::recursiveSplitDynamicGraph(Graph *graph) {
    if (graph->dynamic()) {
        // TODO: put this method into pisdf namespace and into this cpp file
        separateRunGraphFromInit(graph);
    }
    for (auto &subgraph : graph->subgraphs()) {
        recursiveSplitDynamicGraph(subgraph);
    }
}

spider::unique_ptr<i64> spider::pisdf::buildVertexRuntimeInputParameters(const pisdf::Vertex *vertex) {
    return buildVertexRuntimeInputParameters(vertex, vertex->inputParamVector());
}

spider::unique_ptr<i64> spider::pisdf::buildVertexRuntimeInputParameters(const pisdf::Vertex *vertex,
                                                                         const vector<std::shared_ptr<pisdf::Param>> &params) {
    switch (vertex->subtype()) {
        case VertexType::FORK:
            return buildForkRuntimeInputParameters(vertex, params);
        case VertexType::JOIN:
            return buildJoinRuntimeInputParameters(vertex, params);
        case VertexType::TAIL:
            return buildTailRuntimeInputParameters(vertex, params);
        case VertexType::HEAD:
            return buildHeadRuntimeInputParameters(vertex, params);
        case VertexType::REPEAT:
            return buildRepeatRuntimeInputParameters(vertex, params);
        case VertexType::DUPLICATE:
            return buildDuplicateRuntimeInputParameters(vertex, params);
        case VertexType::INIT:
            return buildInitRuntimeInputParameters(vertex);
        case VertexType::END:
            return buildEndRuntimeInputParameters(vertex);
        case VertexType::EXTERN_OUT:
            return buildExternOutRuntimeInputParameters(vertex, params);;
        default:
            return buildDefaultVertexRuntimeParameters(vertex);
    }
}

spider::pisdf::Vertex *spider::pisdf::getIndirectSource(const pisdf::Vertex *vertex, size_t ix) {
    const auto *edge = vertex->inputEdge(ix);
    auto *source = edge->source();
    while (source->subtype() == VertexType::INPUT ||
           source->subtype() == VertexType::GRAPH) {
        if (source->subtype() == VertexType::GRAPH) {
            const auto *graph = source->convertTo<Graph>();
            const auto *interface = graph->outputInterface(edge->sourcePortIx());
            edge = interface->Vertex::inputEdge(0);
        } else {
            edge = source->graph()->inputEdge(source->ix());
        }
        source = edge->source();
    }
    return source;
}

spider::pisdf::Vertex *spider::pisdf::getIndirectSink(const pisdf::Vertex *vertex, size_t ix) {
    const auto *edge = vertex->outputEdge(ix);
    auto *sink = edge->sink();
    while (sink->subtype() == VertexType::GRAPH ||
           sink->subtype() == VertexType::OUTPUT) {
        if (sink->subtype() == VertexType::GRAPH) {
            const auto *graph = sink->convertTo<Graph>();
            const auto *interface = graph->inputInterface(edge->sinkPortIx());
            edge = interface->Vertex::outputEdge(0);
        } else {
            edge = sink->graph()->outputEdge(sink->ix());
        }
        sink = edge->sink();
    }
    return sink;
}
