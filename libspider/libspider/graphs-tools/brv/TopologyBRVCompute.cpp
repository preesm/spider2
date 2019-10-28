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

#include "TopologyBRVCompute.h"
#include <graphs/tmp/Graph.h>
#include <graphs/tmp/Vertex.h>
#include <graphs/tmp/Edge.h>
#include <cstdint>
#include <common/Rational.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

void TopologyBRVCompute::execute() {
    /* == Array of vertex ix in the matrix == */
    Spider::Array<std::int32_t> vertexIxArray{graph_->vertexCount(), -1, StackID::TRANSFO};

    /* == Go through all connected components == */
    for (const auto &component : connectedComponents_) {
        /* == Extract the edges == */
        Spider::Array<const Spider::PiSDF::Edge *> edgeArray{component.nEdges, StackID::TRANSFO};
        BRVCompute::extractEdges(edgeArray, component);

        /* == Set the ix of the corresponding vertices in the topology matrix == */
        std::uint32_t nMatVertices = 0;
        for (const auto &vertex : component.vertices) {
            if (isVertexExecutable(vertex)) {
                vertexIxArray[vertex->ix()] = nMatVertices++;
            }
        }

        /* == Check the number of valid edges == */
        Spider::vector<const Spider::PiSDF::Edge *> validEdgeVector;
        validEdgeVector.reserve(component.nEdges); /* = Reserve the memory for the worst case = */
        std::uint32_t nMatEdges = 0;
        for (const auto &edge : edgeArray) {
            if (isEdgeValid(edge, vertexIxArray)) {
                nMatEdges += 1;
                validEdgeVector.push_back(edge);
            }
        }

        /* == Fill the topology matrix == */
        Spider::Array<std::int64_t> topologyMatrix{nMatEdges * nMatVertices, 0, StackID::TRANSFO};
        std::uint32_t edgeRow = 0;
        for (const auto &edge : validEdgeVector) {
            auto edgeRowOffset = edgeRow * nMatVertices;
            topologyMatrix[edgeRowOffset + vertexIxArray[edge->source()->ix()]] = edge->sourceRateExpression().evaluate();
            topologyMatrix[edgeRowOffset + vertexIxArray[edge->sink()->ix()]] = -edge->sinkRateExpression().evaluate();
            edgeRow += 1;
        }

        /* == Compute the LCM from the null space of the matrix == */
        computeBRVFromNullSpace(topologyMatrix, nMatVertices, nMatEdges, vertexIxArray, component);

        /* == Update the repetition vector values using the interfaces of the graph == */
        updateBRV(edgeArray, component);
    }

    /* == Print the BRV (in VERBOSE mode only) == */
    BRVCompute::print();
}

bool TopologyBRVCompute::isVertexExecutable(const Spider::PiSDF::Vertex *vertex) {
    /* == Check all input edges rate to 0 == */
    for (const auto &e:vertex->inputEdgeArray()) {
        if (e->sinkRateExpression().evaluate()) {
            return true;
        }
    }

    /* == Check all output edges rate to 0 == */
    for (const auto &e:vertex->outputEdgeArray()) {
        if (e->sourceRateExpression().evaluate()) {
            return true;
        }
    }
    return false;
}

bool TopologyBRVCompute::isEdgeValid(const Spider::PiSDF::Edge *edge, Spider::Array<std::int32_t> &vertexIxArray) {
    return edge->source()->type() != Spider::PiSDF::VertexType::INTERFACE &&
           edge->sink()->type() != Spider::PiSDF::VertexType::INTERFACE &&
           edge->source() != edge->sink() &&
           edge->source()->type() != Spider::PiSDF::VertexType::CONFIG &&
           edge->sink()->type() != Spider::PiSDF::VertexType::CONFIG &&
           vertexIxArray[edge->source()->ix()] >= 0 &&
           vertexIxArray[edge->sink()->ix()] >= 0;
}

void TopologyBRVCompute::computeBRVFromNullSpace(Spider::Array<std::int64_t> &topologyMatrix,
                                                 std::uint32_t nMatVertices,
                                                 std::uint32_t nMatEdges,
                                                 Spider::Array<std::int32_t> &vertexIxArray,
                                                 const BRVComponent &component) {
    /* == Copy topology matrix into the rational matrix == */
    Spider::Array<Spider::Rational> rationalMatrix{nMatVertices * nMatEdges, StackID::TRANSFO};
    auto *rationalMatrixIterator = rationalMatrix.begin();
    for (const auto &val : topologyMatrix) {
        (*(rationalMatrixIterator++)) = Spider::Rational{static_cast<std::int64_t>(val)};
    }

    for (std::uint32_t i = 0; i < nMatEdges; ++i) {
        auto pivotMax{rationalMatrix[i * nMatVertices + i].abs()};
        std::uint32_t maxIndex = i;

        for (std::uint32_t t = i + 1; t < nMatEdges; ++t) {
            auto newPivot{rationalMatrix[t * nMatVertices + i].abs()};
            if (newPivot > pivotMax) {
                maxIndex = t;
                pivotMax = newPivot;
            }
        }

        if (pivotMax && maxIndex != i) {
            /* == Switch rows == */
            for (std::uint32_t t = 0; t < nMatVertices; ++t) {
                auto temp{rationalMatrix[maxIndex * nMatVertices + t]};
                rationalMatrix[maxIndex * nMatVertices + t] = Spider::Rational{rationalMatrix[i * nMatVertices + t]};
                rationalMatrix[i * nMatVertices + t] = temp;
            }
        } else if (maxIndex == i && pivotMax) {
            /* == Do nothing == */
        } else {
            break;
        }

        auto oldPivot{rationalMatrix[i * nMatVertices + i]};
        for (std::uint32_t t = i; t < nMatVertices; ++t) {
            rationalMatrix[i * nMatVertices + t] /= oldPivot;
        }

        for (std::uint32_t j = i + 1; j < nMatEdges; ++j) {
            auto oldElementJI{rationalMatrix[j * nMatVertices + i]};
            if (oldElementJI) {
                for (std::uint32_t k = 0; k < nMatVertices; ++k) {
                    rationalMatrix[j * nMatVertices + k] -= (oldElementJI * rationalMatrix[i * nMatVertices + k]);
                }
            }
        }
    }

    /* == Scale the result == */
    Spider::Array<Spider::Rational> rationalResult{nMatVertices, Spider::Rational{1}, StackID::TRANSFO};
    for (std::int32_t i = nMatEdges - 1; i >= 0; i--) {
        auto val{Spider::Rational()};
        for (std::uint32_t k = i + 1; k < nMatVertices; ++k) {
            val += (rationalResult[k] * rationalMatrix[i * nMatVertices + k]);
        }
        if (val) {
            if (!rationalMatrix[i * nMatVertices + i]) {
                throwSpiderException("Diagonal element of the topology matrix [%"
                                             PRIu32
                                             "][%"
                                             PRIu32
                                             "] is null.", i, i);
            }
            rationalResult[i] = val.abs() / rationalMatrix[i * nMatVertices + i];
        }
    }

    /* == Compute the LCM == */
    std::int64_t lcm = 1;
    for (const auto &r : rationalResult) {
        lcm = Spider::Math::lcm(lcm, r.denominator());
    }

    /* == Apply the LCM to compute BRV == */
    std::uint32_t vertexIx = 0;
    for (const auto &vertex : component.vertices) {
        if (vertexIxArray[vertex->ix()] >= 0) {
            auto rv = (rationalResult[vertexIx] * lcm).abs().toInt32();
            vertex->setRepetitionValue(rv);
            vertexIx += 1;
        }
    }
}
