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
#include <common/containers/Array.h>
#include <common/containers/Set.h>

/* === Forward declaration(s) === */

class PiSDFEdge;

class PiSDFGraph;

class PiSDFParam;

/* === Class definition === */

class PiSDFVertex : public Spider::SetElement {
public:

    PiSDFVertex(PiSDFGraph *graph, PiSDFType type, PiSDFSubType subType, std::uint32_t nEdgesIN,
                std::uint32_t nEdgesOUT, std::string name);

    ~PiSDFVertex() = default;

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

    /**
     * @brief Set the subgraph of a hierarchical vertex.
     * @remark This method calls @refitem setParentVertex of the  @refitem PiSDFGraph class.
     * @param subgraph The subgraph to set.
     * @throw @refitem SpiderException if vertex already has a subgraph or nullptr.
     */
    void setSubGraph(PiSDFGraph *subgraph);

    /* === Getters ===  */

    /**
     * @brief Get the containing @refitem PiSDFGraph of the vertex.
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
    inline PiSDFEdge *inputEdge(std::uint32_t ix) const;

    /**
     * @brief Get input edge connected to port Ix.
     * @param ix Index of the output port.
     * @return @refitem PiSDFEdge
     * @throw @refitem SpiderException if out of bound.
     */
    inline PiSDFEdge *outputEdge(std::uint32_t ix) const;

    /**
     * @brief Get input edge connected to port Ix.
     * @param ix Index of the input port.
     * @return @refitem PiSDFParam
     * @throw @refitem SpiderException if out of bound.
     */
    inline PiSDFParam *inputParam(std::uint32_t ix) const;

    /**
     * @brief Get input edge connected to port Ix.
     * @param ix Index of the output port.
     * @return @refitem PiSDFParam
     * @throw @refitem SpiderException if out of bound.
     */
    inline PiSDFParam *outputParam(std::uint32_t ix) const;

    /**
     * @brief A const reference on the array of input edges. Useful for iterating on the edges.
     * @return const reference to input edge array
     */
    inline const Spider::Array<PiSDFEdge *> &inputEdges() const;

    /**
     * @brief A const reference on the array of output edges. Useful for iterating on the edges.
     * @return const reference to output edge array
     */
    inline const Spider::Array<PiSDFEdge *> &outputEdges() const;

    /**
     * @brief Get the hierarchical property of the vertex.
     * @return true if vertex is hierarchical, false else;
     */
    inline bool isHierarchical() const;

    /**
     * @brief Get the subgraph attached to the vertex.
     * @return Subgraph, nullptr if vertex is not hierarchical.
     */
    inline PiSDFGraph *subgraph() const;

private:
    PiSDFGraph *graph_ = nullptr;
    std::string name_ = "unnamed-vertex";

    PiSDFType type_ = PiSDFType::VERTEX;
    PiSDFSubType subType_ = PiSDFSubType::NORMAL;

    std::uint32_t nEdgesIN_ = 0;
    std::uint32_t nEdgesOUT_ = 0;
    std::uint32_t nParamsIN_ = 0;
    std::uint32_t nParamsOUT_ = 0;

    Spider::Array<PiSDFEdge *> inputEdgeArray_;
    Spider::Array<PiSDFEdge *> outputEdgeArray_;

    bool hierarchical_ = false;
    PiSDFGraph *subgraph_ = nullptr;
};

/* === Inline methods === */

void PiSDFVertex::setInputEdge(PiSDFEdge *edge, std::uint32_t ix) {
    if (ix >= nEdgesIN_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nEdgesIN_);
    } else if (inputEdgeArray_[ix]) {
        throwSpiderException("Already existing input edge at ix: %"
                                     PRIu32
                                     ".", ix);
    }
    inputEdgeArray_[ix] = edge;
}

void PiSDFVertex::setOutputEdge(PiSDFEdge *edge, std::uint32_t ix) {
    if (ix >= nEdgesOUT_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nEdgesOUT_);
    } else if (outputEdgeArray_[ix]) {
        throwSpiderException("Already existing output edge at ix: %"
                                     PRIu32
                                     ".", ix);
    }
    outputEdgeArray_[ix] = edge;
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

PiSDFEdge *PiSDFVertex::inputEdge(std::uint32_t ix) const {
    if (ix >= nEdgesIN_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nEdgesIN_);
    }
    return inputEdgeArray_[ix];
}

PiSDFEdge *PiSDFVertex::outputEdge(std::uint32_t ix) const {
    if (ix >= nEdgesOUT_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nEdgesOUT_);
    }
    return outputEdgeArray_[ix];
}

PiSDFParam *PiSDFVertex::inputParam(std::uint32_t ix) const {
    if (ix >= nParamsIN_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nParamsIN_);
    }
    return nullptr;
}

PiSDFParam *PiSDFVertex::outputParam(std::uint32_t ix) const {
    if (ix >= nParamsOUT_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nParamsOUT_);
    }
    return nullptr;
}

const Spider::Array<PiSDFEdge *> &PiSDFVertex::inputEdges() const {
    return inputEdgeArray_;
}

const Spider::Array<PiSDFEdge *> &PiSDFVertex::outputEdges() const {
    return inputEdgeArray_;
}

bool PiSDFVertex::isHierarchical() const {
    return hierarchical_;
}

PiSDFGraph *PiSDFVertex::subgraph() const {
    return subgraph_;
}

#endif //SPIDER2_PISDFVERTEX_H
