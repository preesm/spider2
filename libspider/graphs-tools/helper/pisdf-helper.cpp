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

#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/pisdf/ExternInterface.h>

/* === Static function(s) === */

/**
 * @brief Creates an array with parameters needed for the runtime exec of a normal vertex.
 * @param vertex Pointer to the vertex.
 * @return array of int_least_64_t.
 */
spider::array<i64> buildDefaultVertexRuntimeParameters(const spider::pisdf::Vertex *vertex) {
    const auto &inputParams = vertex->refinementParamVector();
    auto params = spider::array<i64>(inputParams.size(), StackID::RUNTIME);
    std::transform(std::begin(inputParams), std::end(inputParams), std::begin(params),
                   [](const std::shared_ptr<spider::pisdf::Param> &param) { return param->value(); });
    return params;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::FORK
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @return array of int_least_64_t.
 */
spider::array<i64> buildForkRuntimeInputParameters(const spider::pisdf::Vertex *vertex) {
    const auto &outputEdges = vertex->outputEdgeVector();
    auto params = spider::array<i64>(outputEdges.size() + 2, StackID::RUNTIME);
    params[0] = vertex->inputEdge(0)->sinkRateValue();
    params[1] = static_cast<i64>(outputEdges.size());
    std::transform(std::begin(outputEdges), std::end(outputEdges), std::next(std::begin(params), 2),
                   [](const spider::pisdf::Edge *edge) { return edge->sourceRateValue(); });
    return params;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::JOIN
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @return array of int_least_64_t.
 */
spider::array<i64> buildJoinRuntimeInputParameters(const spider::pisdf::Vertex *vertex) {
    const auto &inputEdges = vertex->inputEdgeVector();
    auto params = spider::array<i64>(inputEdges.size() + 2, StackID::RUNTIME);
    params[0] = vertex->outputEdge(0)->sourceRateValue();
    params[1] = static_cast<i64>(inputEdges.size());
    std::transform(std::begin(inputEdges), std::end(inputEdges), std::next(std::begin(params), 2),
                   [](const spider::pisdf::Edge *edge) { return edge->sinkRateValue(); });
    return params;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::TAIL
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @return array of int_least_64_t.
 */
spider::array<i64> buildTailRuntimeInputParameters(const spider::pisdf::Vertex *vertex) {
    size_t inputCount = 1;
    auto rate = vertex->outputEdge(0)->sourceRateValue();
    const auto &inputEdges = vertex->inputEdgeVector();
    for (auto it = inputEdges.rbegin(); it != inputEdges.rend(); ++it) {
        const auto &inRate = (*it)->sinkRateValue();
        if (inRate >= rate) {
            break;
        }
        rate -= inRate;
        inputCount++;
    }
    auto params = spider::array<i64>(4 + inputCount, StackID::RUNTIME);
    /* = Number of input = */
    params[0] = static_cast<i64>(inputEdges.size());
    /* = First input to be considered = */
    params[1] = static_cast<i64>(inputEdges.size() - inputCount);
    /* = Offset in the first buffer if any = */
    params[2] = vertex->inputEdge(inputEdges.size() - inputCount)->sinkRateValue() - rate;
    /* = Effective size to copy of the first input = */
    params[3] = rate;
    size_t i = 4;
    for (auto it = vertex->inputEdgeVector().rbegin();
         it != vertex->inputEdgeVector().rbegin() + static_cast<long>(inputCount) - 1; ++it) {
        params[i++] = (*it)->sinkRateValue();
    }
    return params;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::HEAD
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @return array of int_least_64_t.
 */
spider::array<i64> buildHeadRuntimeInputParameters(const spider::pisdf::Vertex *vertex) {
    size_t inputCount = 1;
    auto rate = vertex->outputEdge(0)->sourceRateValue();
    for (auto &edge : vertex->inputEdgeVector()) {
        const auto &inRate = edge->sinkRateValue();
        if (inRate >= rate) {
            break;
        }
        rate -= inRate;
        inputCount++;
    }
    auto params = spider::array<i64>(1 + inputCount, StackID::RUNTIME);
    params[0] = static_cast<i64>(inputCount);
    rate = vertex->outputEdge(0)->sourceRateValue();
    for (size_t i = 0; i < inputCount; ++i) {
        const auto &inRate = vertex->inputEdge(i)->sinkRateValue();
        params[i + 1] = std::min(inRate, rate);
        rate -= inRate;
    }
    return params;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::REPEAT
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @return array of int_least_64_t.
 */
spider::array<i64> buildRepeatRuntimeInputParameters(const spider::pisdf::Vertex *vertex) {
    auto params = spider::array<i64>(2, StackID::RUNTIME);
    params[0] = vertex->inputEdge(0)->sinkRateValue();
    params[1] = vertex->outputEdge(0)->sourceRateValue();
    return params;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::DUPLICATE
 *        special vertex.
 * @param vertex Pointer to the vertex.
 * @return array of int_least_64_t.
 */
spider::array<i64> buildDuplicateRuntimeInputParameters(const spider::pisdf::Vertex *vertex) {
    auto params = spider::array<i64>(2, StackID::RUNTIME);
    params[0] = static_cast<i64>(vertex->outputEdgeCount());
    params[1] = vertex->inputEdge(0)->sinkRateValue();
    return params;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::INIT or
 *        of @refitem pisdf::VertexType::END special vertex.
 * @param vertex Pointer to the @refitem pisdf::DelayVertex associated with the delay.
 * @return array of int_least_64_t.
 */
spider::array<i64> buildInitEndRuntimeInputParameters(const spider::pisdf::Vertex *vertex) {
    auto params = spider::array<i64>(3, StackID::RUNTIME);
    const auto *delay = vertex->convertTo<spider::pisdf::DelayVertex>()->delay();
    params[0] = delay->isPersistent();
    params[1] = delay->value();
    params[2] = static_cast<i64>(delay->memoryAddress());
    return params;
}

/**
 * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::EXTERN_OUT
 *        special vertex.
 * @param vertex Pointer to the @refitem pisdf::DelayVertex associated with the delay.
 * @return array of int_least_64_t.
 */
static spider::array<i64> buildExternOutRuntimeInputParameters(const spider::pisdf::Vertex *vertex) {
    auto params = spider::array<i64>(2, StackID::RUNTIME);
    const auto *reference = vertex->reference()->convertTo<spider::pisdf::ExternInterface>();
    const auto *inputEdge = vertex->inputEdge(0);
    params[0] = static_cast<i64>(reference->bufferIndex());
    params[1] = inputEdge->sinkRateValue();
    return params;
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

spider::array<i64> spider::pisdf::buildVertexRuntimeInputParameters(const pisdf::Vertex *vertex) {
    switch (vertex->subtype()) {
        case VertexType::FORK:
            return buildForkRuntimeInputParameters(vertex);
        case VertexType::JOIN:
            return buildJoinRuntimeInputParameters(vertex);
        case VertexType::TAIL:
            return buildTailRuntimeInputParameters(vertex);
        case VertexType::HEAD:
            return buildHeadRuntimeInputParameters(vertex);
        case VertexType::REPEAT:
            return buildRepeatRuntimeInputParameters(vertex);
        case VertexType::DUPLICATE:
            return buildDuplicateRuntimeInputParameters(vertex);
        case VertexType::INIT:
            return buildInitEndRuntimeInputParameters(vertex->reference()->outputEdge(0)->sink());
        case VertexType::END:
            return buildInitEndRuntimeInputParameters(vertex->reference()->inputEdge(0)->source());
        case VertexType::EXTERN_OUT:
            return buildExternOutRuntimeInputParameters(vertex);;
        default:
            return buildDefaultVertexRuntimeParameters(vertex);
    }
}