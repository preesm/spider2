/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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
#ifndef SPIDER2_PISDFVERTEX_H
#define SPIDER2_PISDFVERTEX_H

/* === Includes === */

#include <cstdint>
#include <cinttypes>
#include <graphs/pisdf/PiSDFTypes.h>
#include <common/SpiderException.h>

/* === Forward declaration(s) === */

class PiSDFEdge;

class PiSDFGraph;

class PiSDFParam;

/* === Class definition === */

class PiSDFVertex {
public:

    PiSDFVertex(PiSDFGraph *graph, PiSDFType type, PiSDFSubType subType, std::uint32_t nEdgesIN,
                std::uint32_t nEdgesOUT);

    ~PiSDFVertex();

    /* === Methods === */

    /* === Setters === */

    /**
     * @brief Set the input edge of index ix.
     * @param edge Edge to set.
     * @param ix  Index of the edge.
     * @throw @refitem SpiderException if out of bound or already existing edge.
     */
    inline void setInputEdge(PiSDFEdge *edge, std::uint32_t ix);

    /**
     * @brief Set the output edge of index ix.
     * @param edge Edge to set.
     * @param ix  Index of the edge.
     * @throw @refitem SpiderException if out of bound or already existing edge.
     */
    inline void setOutputEdge(PiSDFEdge *edge, std::uint32_t ix);

    /* === Getters ===  */

    /**
     * @brief Get the containing @refitem PiSDFGraph of the edge.
     * @return containing @refitem PiSDFGraph
     */
    inline PiSDFGraph *containingGraph() const;

    /**
     * @brief Get the name string of the vertex.
     * @return name of the vertex.
     */
    inline std::string name() const;

    /**
     * @brief Get the number of input edges connected to the vertex.
     * @return number of input edges.
     */
    inline std::uint32_t nEdgesIN() const;

    /**
     * @brief Get the number of output edges connected to the vertex.
     * @return number of output edges.
     */
    inline std::uint32_t nEdgesOUT() const;

    /**
     * @brief Get the number of input parameters connected to the vertex.
     * @return number of input edges.
     */
    inline std::uint32_t nParamsIN() const;

    /**
     * @brief Get the number of output parameters connected to the vertex.
     * @return number of output parameters (config actor only).
     */
    inline std::uint32_t nParamsOUT() const;

    /**
     * @brief Get the @refitem PiSDFType of the vertex.
     * @return @refitem PiSDFType of the vertex.
     */
    inline PiSDFType type() const;

    /**
     * @brief Get the @refitem PiSDFSubType of the vertex.
     * @return @refitem PiSDFSubType of the vertex.
     */
    inline PiSDFSubType subType() const;

    /**
     * @brief Get input edge connected to port Ix.
     * @param ix Index of the input port.
     * @return @refitem PiSDFEdge
     * @throw @refitem SpiderException if out of bound
     */
    inline PiSDFEdge *inEdge(std::uint32_t ix) const;

    /**
     * @brief Get input edge connected to port Ix.
     * @param ix Index of the output port.
     * @return @refitem PiSDFEdge
     * @throw @refitem SpiderException if out of bound.
     */
    inline PiSDFEdge *outEdge(std::uint32_t ix) const;

    /**
     * @brief Get input edge connected to port Ix.
     * @param ix Index of the input port.
     * @return @refitem PiSDFParam
     * @throw @refitem SpiderException if out of bound.
     */
    inline PiSDFParam *inParam(std::uint32_t ix) const;

    /**
     * @brief Get input edge connected to port Ix.
     * @param ix Index of the output port.
     * @return @refitem PiSDFParam
     * @throw @refitem SpiderException if out of bound.
     */
    inline PiSDFParam *outParam(std::uint32_t ix) const;

private:
    PiSDFGraph *graph_ = nullptr;
    std::string name_ = "unnamed-vertex";

    PiSDFType type_ = PiSDFType::VERTEX;
    PiSDFSubType subType_ = PiSDFSubType::NORMAL;

    std::uint32_t nEdgesIN_ = 0;
    std::uint32_t nEdgesOUT_ = 0;
    std::uint32_t nParamsIN_ = 0;
    std::uint32_t nParamsOUT_ = 0;

    PiSDFEdge **inputEdgeList_;
    PiSDFEdge **outputEdgeList_;
};

/* === Inline methods === */

void PiSDFVertex::setInputEdge(PiSDFEdge *edge, std::uint32_t ix) {
    if (ix >= nEdgesIN_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nEdgesIN_);
    } else if (inputEdgeList_[ix]) {
        throwSpiderException("Already existing input edge at ix: %"
                                     PRIu32
                                     ".", ix);
    }
    inputEdgeList_[ix] = edge;
}

void PiSDFVertex::setOutputEdge(PiSDFEdge *edge, std::uint32_t ix) {
    if (ix >= nEdgesOUT_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nEdgesOUT_);
    } else if (outputEdgeList_[ix]) {
        throwSpiderException("Already existing output edge at ix: %"
                                     PRIu32
                                     ".", ix);
    }
    outputEdgeList_[ix] = edge;
}

PiSDFGraph *PiSDFVertex::containingGraph() const {
    return graph_;
}

std::string PiSDFVertex::name() const {
    return name_;
}

std::uint32_t PiSDFVertex::nEdgesIN() const {
    return nEdgesIN_;
}

std::uint32_t PiSDFVertex::nEdgesOUT() const {
    return nEdgesOUT_;
}

std::uint32_t PiSDFVertex::nParamsIN() const {
    return nParamsIN_;
}

std::uint32_t PiSDFVertex::nParamsOUT() const {
    return nParamsOUT_;
}

PiSDFType PiSDFVertex::type() const {
    return type_;
}

PiSDFSubType PiSDFVertex::subType() const {
    return subType_;
}

PiSDFEdge *PiSDFVertex::inEdge(std::uint32_t ix) const {
    if (ix >= nEdgesIN_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nEdgesIN_);
    }
    return inputEdgeList_[ix];
}

PiSDFEdge *PiSDFVertex::outEdge(std::uint32_t ix) const {
    if (ix >= nEdgesOUT_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nEdgesOUT_);
    }
    return outputEdgeList_[ix];
}

PiSDFParam *PiSDFVertex::inParam(std::uint32_t ix) const {
    if (ix >= nParamsIN_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nParamsIN_);
    }
    return nullptr;
}

PiSDFParam *PiSDFVertex::outParam(std::uint32_t ix) const {
    if (ix >= nParamsOUT_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nParamsOUT_);
    }
    return nullptr;
}

#endif //SPIDER2_PISDFVERTEX_H
