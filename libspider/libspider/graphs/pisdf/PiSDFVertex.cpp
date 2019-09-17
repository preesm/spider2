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

/* === Includes === */

#include <graphs/pisdf/PiSDFVertex.h>
#include <graphs/pisdf/PiSDFGraph.h>
#include <graphs/pisdf/PiSDFEdge.h>
#include <memory/Allocator.h>

/* === Methods implementation === */

PiSDFVertex::PiSDFVertex(StackID stack,
                         PiSDFGraph *graph,
                         std::string name,
                         PiSDFVertexType type,
                         std::uint32_t nEdgesIN,
                         std::uint32_t nEdgesOUT,
                         std::uint32_t nParamsIn,
                         std::uint32_t nParamsOut) : graph_{graph},
                                                     name_{std::move(name)},
                                                     type_{type},
                                                     nEdgesIN_{nEdgesIN},
                                                     nEdgesOUT_{nEdgesOUT},
                                                     nParamsIN_{nParamsIn},
                                                     nParamsOUT_{nParamsOut},
                                                     inputEdgeArray_(stack, nEdgesIN, nullptr),
                                                     outputEdgeArray_(stack, nEdgesOUT, nullptr),
                                                     inputParamArray_(stack, nParamsIn, nullptr),
                                                     outputParamArray_(stack, nParamsOut, nullptr) {
    checkSubtypeConsistency();

    if (type == PiSDFVertexType::CONFIG ||
        type == PiSDFVertexType::INTERFACE) {
        /* == Configuration actors and interfaces have a fixed repetition vector value of 1 == */
        repetitionValue_ = 1;
    }

    if (graph && type != PiSDFVertexType::INTERFACE) {
        graph->addVertex(this);
    }

    /* == Set the reference as itself, it is not a copy == */
    reference_ = this;
}


PiSDFVertex::PiSDFVertex(const PiSDFVertex &other, StackID stack) : PiSDFVertex(stack,
                                                                                other.graph_,
                                                                                other.name_ + "copy_" +
                                                                                std::to_string(other.copyCount_),
                                                                                other.type(),
                                                                                other.nEdgesIN(),
                                                                                other.nEdgesOUT(),
                                                                                other.nParamsIN(),
                                                                                other.nParamsOUT()) {

    /* == Set the reference == */
    setReference(other.reference_);
}

PiSDFVertex::PiSDFVertex(PiSDFGraph *graph,
                         std::string name,
                         PiSDFVertexType type,
                         std::uint32_t nEdgesIN,
                         std::uint32_t nEdgesOUT,
                         std::uint32_t nParamsIn,
                         std::uint32_t nParamsOut) : PiSDFVertex(StackID::PISDF,
                                                                 graph,
                                                                 std::move(name),
                                                                 type,
                                                                 nEdgesIN,
                                                                 nEdgesOUT,
                                                                 nParamsIn,
                                                                 nParamsOut) {
}

void PiSDFVertex::disconnectInputEdge(std::uint16_t ix) {
    if (ix >= inputEdgeArray_.size()) {
        throwSpiderException("Trying to disconnect input edge out of bound: %s[%"
                                     PRIu16
                                     "].", name_.c_str(), ix);
    }
    if (!inputEdgeArray_[ix]) {
        return;
    }
    inputEdgeArray_[ix] = nullptr;
}

void PiSDFVertex::disconnectOutputEdge(std::uint16_t ix) {
    if (ix >= outputEdgeArray_.size()) {
        throwSpiderException("Trying to disconnect output edge out of bound: %s[%"
                                     PRIu16
                                     "].", name_.c_str(), ix);
    }
    if (!outputEdgeArray_[ix]) {
        return;
    }
    outputEdgeArray_[ix] = nullptr;
}

void PiSDFVertex::setInputEdge(PiSDFEdge *edge, std::uint16_t ix) {
    if (inputEdgeArray_[ix]) {
        throwSpiderException("Already existing input edge at ix: %"
                                     PRIu16
                                     ".", ix);
    }
    inputEdgeArray_[ix] = edge;
}

void PiSDFVertex::setOutputEdge(PiSDFEdge *edge, std::uint16_t ix) {
    if (outputEdgeArray_[ix]) {
        throwSpiderException("Already existing output edge at ix: %"
                                     PRIu16
                                     ".", ix);
    }
    outputEdgeArray_[ix] = edge;
}

/* === Private method(s) === */

void PiSDFVertex::checkSubtypeConsistency() const {
    if (!graph_ && type_ != PiSDFVertexType::GRAPH) {
        throwSpiderException("Vertex should belong to a graph: [%s].", name_.c_str());
    }
    if (nParamsOUT_ && (type_ != PiSDFVertexType::CONFIG)) {
        throwSpiderException("Non configuration actors can not have output parameters: [%s].", name_.c_str());
    }
    switch (type_) {
        case PiSDFVertexType::HEAD:
        case PiSDFVertexType::TAIL:
        case PiSDFVertexType::JOIN:
            if (nEdgesOUT_ != 1) {
                throwSpiderException("Join, Head and Tail actors should have exactly 1 output edge: [%s].",
                                     name_.c_str());
            }
            break;
        case PiSDFVertexType::FORK:
        case PiSDFVertexType::DUPLICATE:
            if (nEdgesIN_ != 1) {
                throwSpiderException("Fork and Duplicate actors should have exactly 1 input edge: [%s].",
                                     name_.c_str());
            }
            break;
        case PiSDFVertexType::UPSAMPLE:
        case PiSDFVertexType::DOWNSAMPLE:
            if (nEdgesOUT_ != 1 || nEdgesIN_ != 1) {
                throwSpiderException(
                        "Upsample and Downsample actors should have exactly 1 input edge and 1 output edge: [%s].",
                        name_.c_str());
            }
            break;
        case PiSDFVertexType::INIT:
            if (nEdgesIN_) {
                throwSpiderException("Init actors can not have input edges: [%s].", name_.c_str());
            }
            break;
        case PiSDFVertexType::END:
            if (nEdgesOUT_) {
                throwSpiderException("End actors can not have output edges: [%s].", name_.c_str());
            }
            break;
        default:
            break;
    }
}