/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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

#include <graphs-tools/helper/srdag-helper.h>
#include <graphs/srdag/SRDAGGraph.h>
#include <graphs/srdag/SRDAGEdge.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/pisdf/ExternInterface.h>

/* === function(s) definition === */

namespace spider {
    namespace srdag {

        /**
         * @brief Creates an array with parameters needed for the runtime exec of a normal vertex.
         * @param vertex Pointer to the vertex.
         * @return array of int_least_64_t.
         */
        spider::unique_ptr<i64> buildDefaultVertexRuntimeParameters(const srdag::Vertex *vertex) {
            const auto &inputParams = vertex->refinementParamVector();
            auto result = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(inputParams.size()));
            std::transform(std::begin(inputParams), std::end(inputParams), result.get(),
                           [](const std::shared_ptr<spider::pisdf::Param> &param) {
                               return param->value();
                           });
            return result;
        }

        /**
         * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::FORK
         *        special vertex.
         * @param vertex Pointer to the vertex.
         * @return array of int_least_64_t.
         */
        spider::unique_ptr<i64> buildForkRuntimeInputParameters(const srdag::Vertex *vertex) {
            const auto &outputEdges = vertex->outputEdges();
            auto result = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(outputEdges.size() + 2));
            result.get()[0] = vertex->inputEdge(0)->sinkRateValue();
            result.get()[1] = static_cast<i64>(outputEdges.size());
            std::transform(std::begin(outputEdges), std::end(outputEdges), std::next(result.get(), 2),
                           [](const spider::srdag::Edge *edge) {
                               return edge->sourceRateValue();
                           });
            return result;
        }

        /**
         * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::JOIN
         *        special vertex.
         * @param vertex Pointer to the vertex.
         * @return array of int_least_64_t.
         */
        spider::unique_ptr<i64> buildJoinRuntimeInputParameters(const srdag::Vertex *vertex) {
            const auto &inputEdges = vertex->inputEdges();
            auto result = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(inputEdges.size() + 2));
            result.get()[0] = vertex->outputEdge(0)->sourceRateValue();
            result.get()[1] = static_cast<i64>(inputEdges.size());
            std::transform(std::begin(inputEdges), std::end(inputEdges), std::next(result.get(), 2),
                           [](const srdag::Edge *edge) {
                               return edge->sinkRateValue();
                           });
            return result;
        }

        /**
         * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::TAIL
         *        special vertex.
         * @param vertex Pointer to the vertex.
         * @return array of int_least_64_t.
         */
        spider::unique_ptr<i64> buildTailRuntimeInputParameters(const srdag::Vertex *vertex) {
            size_t inputCount = 1;
            auto rate = vertex->outputEdge(0)->sourceRateValue();
            const auto inputEdges = vertex->inputEdges();
            const auto itRBegin = std::next(std::begin(inputEdges), static_cast<long>(inputEdges.size() - 1 ));
            const auto itREnd = std::next(std::begin(inputEdges), -1);
            for (auto it = itRBegin; it != itREnd; --it) {
                const auto &inRate = (*it)->sinkRateValue();
                if (inRate >= rate) {
                    break;
                }
                rate -= inRate;
                inputCount++;
            }
            auto result = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(inputCount + 4u));
            /* = Number of input = */
            result.get()[0] = static_cast<i64>(inputEdges.size());
            /* = First input to be considered = */
            result.get()[1] = static_cast<i64>(inputEdges.size() - inputCount);
            /* = Offset in the first buffer if any = */
            result.get()[2] =
                    vertex->inputEdge(inputEdges.size() - inputCount)->sinkRateValue() - rate;
            /* = Effective size to copy of the first input = */
            result.get()[3] = rate;
            size_t i = 4;
            for (auto it = itRBegin; it != itREnd - static_cast<long>(inputCount) + 1; --it) {
                result.get()[i++] = (*it)->sinkRateValue();
            }
            return result;
        }

        /**
         * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::HEAD
         *        special vertex.
         * @param vertex Pointer to the vertex.
         * @return array of int_least_64_t.
         */
        spider::unique_ptr<i64> buildHeadRuntimeInputParameters(const srdag::Vertex *vertex) {
            size_t inputCount = 1;
            auto rate = vertex->outputEdge(0)->sourceRateValue();
            for (auto &edge : vertex->inputEdges()) {
                const auto &inRate = edge->sinkRateValue();
                if (inRate >= rate) {
                    break;
                }
                rate -= inRate;
                inputCount++;
            }
            auto result = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(inputCount + 1u));
            result.get()[0] = static_cast<i64>(inputCount);
            rate = vertex->outputEdge(0)->sourceRateValue();
            for (size_t i = 0; i < inputCount; ++i) {
                const auto &inRate = vertex->inputEdge(i)->sinkRateValue();
                result.get()[i + 1] = std::min(inRate, rate);
                rate -= inRate;
            }
            return result;
        }

        /**
         * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::REPEAT
         *        special vertex.
         * @param vertex Pointer to the vertex.
         * @return array of int_least_64_t.
         */
        spider::unique_ptr<i64> buildRepeatRuntimeInputParameters(const srdag::Vertex *vertex) {
            auto result = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(2u));
            result.get()[0] = vertex->inputEdge(0)->sinkRateValue();
            result.get()[1] = vertex->outputEdge(0)->sourceRateValue();
            return result;
        }

        /**
         * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::DUPLICATE
         *        special vertex.
         * @param vertex Pointer to the vertex.
         * @return array of int_least_64_t.
         */
        spider::unique_ptr<i64> buildDuplicateRuntimeInputParameters(const srdag::Vertex *vertex) {
            auto result = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(2u));
            result.get()[0] = static_cast<i64>(vertex->outputEdgeCount());
            result.get()[1] = vertex->inputEdge(0)->sinkRateValue();
            return result;
        }

        /**
         * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::INIT special vertex.
         * @param vertex Pointer to the @refitem pisdf::Vertex associated with the delay.
         * @return array of int_least_64_t.
         */
        spider::unique_ptr<i64> buildInitRuntimeInputParameters(const srdag::Vertex *vertex) {
            auto result = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(3u));
            const auto *reference = vertex->reference();
            const auto *sink = reference->outputEdge(0u)->sink();
            if (sink->subtype() == pisdf::VertexType::DELAY) {
                const auto *delayVertex = sink->convertTo<pisdf::DelayVertex>();
                const auto *delay = delayVertex->delay();
                result.get()[0] = delay->isPersistent();                    /* = Persistence property = */
                result.get()[1] = delay->value();                           /* = Value of the delay = */
                result.get()[2] = static_cast<i64>(delay->memoryAddress()); /* = Memory address (may be unused) = */
            } else {
                result.get()[0] = 0;
                result.get()[1] = 0;
                result.get()[2] = 0;
            }
            return result;
        }

        /**
         * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::END special vertex.
         * @param vertex Pointer to the @refitem pisdf::Vertex associated with the delay.
         * @return array of int_least_64_t.
         */
        spider::unique_ptr<i64> buildEndRuntimeInputParameters(const srdag::Vertex *vertex) {
            auto result = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(3u));
            const auto *reference = vertex->reference();
            const auto *source = reference->inputEdge(0u)->source();
            if (source->subtype() == pisdf::VertexType::DELAY) {
                const auto *delayVertex = source->convertTo<pisdf::DelayVertex>();
                const auto *delay = delayVertex->delay();
                result.get()[0] = delay->isPersistent();                    /* = Persistence property = */
                result.get()[1] = delay->value();                           /* = Value of the delay = */
                result.get()[2] = static_cast<i64>(delay->memoryAddress()); /* = Memory address (may be unused) = */
            } else {
                result.get()[0] = 0;
                result.get()[1] = 0;
                result.get()[2] = 0;
            }
            return result;
        }

        /**
         * @brief Creates an array with parameters needed for the runtime exec of @refitem pisdf::VertexType::EXTERN_OUT
         *        special vertex.
         * @param vertex Pointer to the vertex.
         * @return array of int_least_64_t.
         */
        spider::unique_ptr<i64> buildExternOutRuntimeInputParameters(const srdag::Vertex *vertex) {
            auto result = spider::make_unique(spider::allocate<i64, StackID::RUNTIME>(2u));
            const auto *reference = vertex->reference()->convertTo<spider::pisdf::ExternInterface>();
            const auto *inputEdge = vertex->inputEdge(0);
            result.get()[0] = static_cast<i64>(reference->bufferIndex());
            result.get()[1] = inputEdge->sinkRateValue();
            return result;
        }
    }
}

/* === Function(s) definition === */

spider::unique_ptr<i64> spider::srdag::buildVertexRuntimeInputParameters(const srdag::Vertex *vertex) {
    switch (vertex->subtype()) {
        case pisdf::VertexType::FORK:
            return buildForkRuntimeInputParameters(vertex);
        case pisdf::VertexType::JOIN:
            return buildJoinRuntimeInputParameters(vertex);
        case pisdf::VertexType::TAIL:
            return buildTailRuntimeInputParameters(vertex);
        case pisdf::VertexType::HEAD:
            return buildHeadRuntimeInputParameters(vertex);
        case pisdf::VertexType::REPEAT:
            return buildRepeatRuntimeInputParameters(vertex);
        case pisdf::VertexType::DUPLICATE:
            return buildDuplicateRuntimeInputParameters(vertex);
        case pisdf::VertexType::INIT:
            return buildInitRuntimeInputParameters(vertex);
        case pisdf::VertexType::END:
            return buildEndRuntimeInputParameters(vertex);
        case pisdf::VertexType::EXTERN_OUT:
            return buildExternOutRuntimeInputParameters(vertex);;
        default:
            return buildDefaultVertexRuntimeParameters(vertex);
    }
}
