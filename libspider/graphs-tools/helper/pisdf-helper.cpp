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

/* === Static function(s) === */

/**
 * @brief Creates an array with parameters needed for the runtime exec of a normal vertex.
 * @param vertex Pointer to the vertex.
 * @return array of int_least_64_t.
 */
spider::array<i64> buildDefaultVertexRuntimeParameters(const spider::pisdf::Vertex *vertex) {
    const auto &inputParams = vertex->refinementParamVector();
    auto outParams = spider::array<i64>(inputParams.size(), StackID::RUNTIME);
    std::transform(std::begin(inputParams), std::end(inputParams), std::begin(outParams),
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
spider::array<i64> buildForkRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
                                                   const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params) {
    const auto &outputEdges = vertex->outputEdgeVector();
    auto outParams = spider::array<i64>(outputEdges.size() + 2, StackID::RUNTIME);
    outParams[0] = vertex->inputEdge(0)->sinkRateExpression().evaluate(params);
    outParams[1] = static_cast<i64>(outputEdges.size());
    std::transform(std::begin(outputEdges), std::end(outputEdges), std::next(std::begin(outParams), 2),
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
spider::array<i64> buildJoinRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
                                                   const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params) {
    const auto &inputEdges = vertex->inputEdgeVector();
    auto outParams = spider::array<i64>(inputEdges.size() + 2, StackID::RUNTIME);
    outParams[0] = vertex->outputEdge(0)->sourceRateExpression().evaluate(params);
    outParams[1] = static_cast<i64>(inputEdges.size());
    std::transform(std::begin(inputEdges), std::end(inputEdges), std::next(std::begin(outParams), 2),
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
spider::array<i64> buildTailRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
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
    auto outParams = spider::array<i64>(4 + inputCount, StackID::RUNTIME);
    /* = Number of input = */
    outParams[0] = static_cast<i64>(inputEdges.size());
    /* = First input to be considered = */
    outParams[1] = static_cast<i64>(inputEdges.size() - inputCount);
    /* = Offset in the first buffer if any = */
    outParams[2] = vertex->inputEdge(inputEdges.size() - inputCount)->sinkRateExpression().evaluate(params) - rate;
    /* = Effective size to copy of the first input = */
    outParams[3] = rate;
    size_t i = 4;
    for (auto it = vertex->inputEdgeVector().rbegin();
         it != vertex->inputEdgeVector().rbegin() + static_cast<long>(inputCount) - 1; ++it) {
        outParams[i++] = (*it)->sinkRateExpression().evaluate(params);
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
spider::array<i64> buildHeadRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
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
    auto outParams = spider::array<i64>(1 + inputCount, StackID::RUNTIME);
    outParams[0] = static_cast<i64>(inputCount);
    rate = vertex->outputEdge(0)->sourceRateExpression().evaluate(params);
    for (size_t i = 0; i < inputCount; ++i) {
        const auto &inRate = vertex->inputEdge(i)->sinkRateExpression().evaluate(params);
        outParams[i + 1] = std::min(inRate, rate);
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
spider::array<i64> buildRepeatRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
                                                     const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params) {
    auto outParams = spider::array<i64>(2, StackID::RUNTIME);
    outParams[0] = vertex->inputEdge(0)->sinkRateExpression().evaluate(params);
    outParams[1] = vertex->outputEdge(0)->sourceRateExpression().evaluate(params);
    return outParams;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::DUPLICATE
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @param params Parameters to use for the rates evaluation. (should contain the same parameters as the graphs)
 * @return array of int_least_64_t.
 */
spider::array<i64> buildDuplicateRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
                                                        const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params) {
    auto outParams = spider::array<i64>(2, StackID::RUNTIME);
    outParams[0] = static_cast<i64>(vertex->outputEdgeCount());
    outParams[1] = vertex->inputEdge(0)->sinkRateExpression().evaluate(params);
    return outParams;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::INIT or
 *        of @refitem pisdf::VertexType::END special vertex.
 * @param vertex Pointer to the @refitem pisdf::DelayVertex associated with the delay.
 * @return array of int_least_64_t.
 */
spider::array<i64> buildInitEndRuntimeInputParameters(const spider::pisdf::Vertex *vertex) {
    auto outParams = spider::array<i64>(3, StackID::RUNTIME);
    if (vertex->subtype() == spider::pisdf::VertexType::DELAY) {
        const auto *delay = vertex->convertTo<spider::pisdf::DelayVertex>()->delay();
        outParams[0] = delay->isPersistent();
        outParams[1] = delay->value();
        outParams[2] = static_cast<i64>(delay->memoryAddress());
    } else {
        outParams[0] = 0;
    }
    return outParams;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::EXTERN_OUT
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @param params Parameters to use for the rates evaluation. (should contain the same parameters as the graphs)
 * @return array of int_least_64_t.
 */
static spider::array<i64> buildExternOutRuntimeInputParameters(const spider::pisdf::Vertex *vertex,
                                                               const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params) {
    auto outParams = spider::array<i64>(2, StackID::RUNTIME);
    const auto *reference = vertex->reference()->convertTo<spider::pisdf::ExternInterface>();
    const auto *inputEdge = vertex->inputEdge(0);
    outParams[0] = static_cast<i64>(reference->bufferIndex());
    outParams[1] = inputEdge->sinkRateExpression().evaluate(params);
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

void spider::pisdf::recursiveSplitDynamicGraph(Graph *graph) {
    if (graph->dynamic()) {
        // TODO: put this method into pisdf namespace and into this cpp file
        srdag::separateRunGraphFromInit(graph);
    }
    for (auto &subgraph : graph->subgraphs()) {
        recursiveSplitDynamicGraph(subgraph);
    }
}

spider::array<i64> spider::pisdf::buildVertexRuntimeInputParameters(const pisdf::Vertex *vertex) {
    return buildVertexRuntimeInputParameters(vertex, vertex->inputParamVector());
}

spider::array<i64> spider::pisdf::buildVertexRuntimeInputParameters(const pisdf::Vertex *vertex,
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
            return buildInitEndRuntimeInputParameters(vertex->reference()->outputEdge(0)->sink());
        case VertexType::END:
            return buildInitEndRuntimeInputParameters(vertex->reference()->inputEdge(0)->source());
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
