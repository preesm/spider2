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
#ifndef SPIDER2_PISDFVERTEX_H
#define SPIDER2_PISDFVERTEX_H

/* === Includes === */

#include <cstdint>
#include <cinttypes>
#include <graphs/pisdf/PiSDFTypes.h>
#include <common/SpiderException.h>
#include <containers/Array.h>
#include <containers/StlContainers.h>

/* === Forward declaration(s) === */

class PiSDFEdge;

class PiSDFGraph;

class PiSDFParam;

/* === Class definition === */

class PiSDFVertex {
public:

    PiSDFVertex(StackID stack,
                PiSDFGraph *graph,
                std::string name,
                PiSDFVertexType type,
                std::uint32_t nEdgesIN,
                std::uint32_t nEdgesOUT,
                std::uint32_t nParamsIn,
                std::uint32_t nParamsOut);

    PiSDFVertex(PiSDFGraph *graph,
                std::string name,
                PiSDFVertexType type,
                std::uint32_t nEdgesIN = 0,
                std::uint32_t nEdgesOUT = 0,
                std::uint32_t nParamsIn = 0,
                std::uint32_t nParamsOut = 0);

    ~PiSDFVertex() = default;

    /* === Methods === */

    /**
     * @brief Export vertex in the dot format to the given file.
     * @param file   File to which the vertex should be exported.
     * @param offset Tab offset (default is "\t").
     */
    void exportDot(FILE *file, const std::string &offset = "\t") const;

    /**
     * @brief Get the hierarchical property of the vertex.
     * @return true if the vertex is hierarchical, false else.
     */
    inline bool isHierarchical() const;

    /**
     * @brief Disconnect input edge on port ix. If no edge is connected, nothing happens
     * @param ix  Index of the input edge to disconnect.
     * @throws @refitem SpiderException if index out of bound.
     */
    void disconnectInputEdge(std::uint16_t ix);

    /**
     * @brief Disconnect output edge on port ix. If no edge is connected, nothing happens
     * @param ix  Index of the output edge to disconnect.
     * @throws @refitem SpiderException if index out of bound.
     */
    void disconnectOutputEdge(std::uint16_t ix);

    /* === Setter(s) === */

    /**
     * @brief Set the input edge of index ix.
     * @param edge Edge to set.
     * @param ix  Index of the edge.
     * @throw @refitem SpiderException if out of bound or already existing edge.
     */
    void setInputEdge(PiSDFEdge *edge, std::uint16_t ix);

    /**
     * @brief Set the output edge of index ix.
     * @param edge Edge to set.
     * @param ix  Index of the edge.
     * @throw @refitem SpiderException if out of bound or already existing edge.
     */
    void setOutputEdge(PiSDFEdge *edge, std::uint16_t ix);

    /**
     * @brief Set the input param of index ix.
     * @param param Param to set.
     * @param ix  Index of the param.
     * @throw @refitem SpiderException if out of bound or already existing param.
     */
    inline void setInputParam(PiSDFParam *param, std::uint32_t ix);

    /**
     * @brief Set the output edge of index ix.
     * @param param Param to set.
     * @param ix  Index of the param.
     * @throw @refitem SpiderException if out of bound or already existing param.
     */
    inline void setOutputParam(PiSDFParam *param, std::uint32_t ix);

    /**
     * @brief Set the repetition vector value of the vertex;
     * @param rv Repetition value to set.
     */
    inline void setRepetitionValue(std::uint32_t rv);

    /**
     * @brief Set the ix of the vertex in the containing graph.
     * @param ix Ix to set.
     */
    inline void setIx(std::uint32_t ix);

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
     * @brief Get the @refitem PiSDFVertexType of the vertex.
     * @return @refitem PiSDFVertexType of the vertex.
     */
    inline PiSDFVertexType type() const;

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
     * @brief A const reference on the array of input params. Useful for iterating on the params.
     * @return const reference to input param array.
     */
    inline const Spider::Array<PiSDFParam *> &inputParams() const;

    /**
     * @brief A const reference on the array of output params. Useful for iterating on the params.
     * @return const reference to output param array.
     */
    inline const Spider::Array<PiSDFParam *> &outputParams() const;

    /**
     * @brief Get the repetition vector value of the vertex.
     * @return repetition vector value of the vertex. (UINT32_MAX if uninitialized)
     */
    inline std::uint32_t repetitionValue() const;

    /**
     * @brief Get the subgraph (if any) associated to the vertex.
     * @return subgraph associated to the vertex, nullptr if vertex is not hierarchical.
     */
    inline PiSDFGraph *subgraph() const;

    /**
     * @brief Get the ix of the vertex in the containing graph.
     * @return ix of the vertex (UINT32_MAX if no ix).
     */
    inline std::uint32_t getIx() const;

private:
    PiSDFGraph *graph_ = nullptr;
    std::string name_ = "unnamed-vertex";

    PiSDFVertexType type_ = PiSDFVertexType::NORMAL;

    std::uint32_t nEdgesIN_ = 0;
    std::uint32_t nEdgesOUT_ = 0;
    std::uint32_t nParamsIN_ = 0;
    std::uint32_t nParamsOUT_ = 0;

    Spider::Array<PiSDFEdge *> inputEdgeArray_;
    Spider::Array<PiSDFEdge *> outputEdgeArray_;
    Spider::Array<PiSDFParam *> inputParamArray_;
    Spider::Array<PiSDFParam *> outputParamArray_;

    PiSDFGraph *subgraph_ = nullptr;

    std::uint32_t repetitionValue_ = 0;
    std::uint32_t ix_ = UINT32_MAX;

    /* === Private methods === */

    /**
     * @brief Export the input ports of the vertex in DOT format.
     * @param file    File to export to.
     * @param offset  Tab offset for the export.
     */
    void exportInputPortsToDot(FILE *file, const std::string &offset) const;

    /**
     * @brief Export the output ports of the vertex in DOT format.
     * @param file    File to export to.
     * @param offset  Tab offset for the export.
     */
    void exportOutputPortsToDot(FILE *file, const std::string &offset) const;

    /**
     * @brief Check for subtype consistency.
     */
    void checkSubtypeConsistency() const;
};

/* === Inline methods === */

bool PiSDFVertex::isHierarchical() const {
    return type_ == PiSDFVertexType::HIERARCHICAL;
}

void PiSDFVertex::setInputParam(PiSDFParam *param, std::uint32_t ix) {
    if (ix >= nParamsIN_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nParamsIN_);
    } else if (inputParamArray_[ix]) {
        throwSpiderException("Already existing input param at ix: %"
                                     PRIu32
                                     ".", ix);
    }
    inputParamArray_[ix] = param;
}

void PiSDFVertex::setOutputParam(PiSDFParam *param, std::uint32_t ix) {
    if (ix >= nParamsOUT_) {
        throwSpiderException("Index out of bound: %"
                                     PRIu32
                                     " -- Max: %"
                                     PRIu32
                                     ".", ix, nParamsOUT_);
    } else if (outputParamArray_[ix]) {
        throwSpiderException("Already existing output param at ix: %"
                                     PRIu32
                                     ".", ix);
    }
    outputParamArray_[ix] = param;
}

void PiSDFVertex::setRepetitionValue(std::uint32_t rv) {
    repetitionValue_ = rv;
}

void PiSDFVertex::setIx(std::uint32_t ix) {
    ix_ = ix;
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

PiSDFVertexType PiSDFVertex::type() const {
    return type_;
}

PiSDFEdge *PiSDFVertex::inputEdge(std::uint32_t ix) const {
    return inputEdgeArray_[ix];
}

PiSDFEdge *PiSDFVertex::outputEdge(std::uint32_t ix) const {
    return outputEdgeArray_[ix];
}

PiSDFParam *PiSDFVertex::inputParam(std::uint32_t ix) const {
    return inputParamArray_[ix];
}

PiSDFParam *PiSDFVertex::outputParam(std::uint32_t ix) const {
    return outputParamArray_[ix];
}

const Spider::Array<PiSDFEdge *> &PiSDFVertex::inputEdges() const {
    return inputEdgeArray_;
}

const Spider::Array<PiSDFEdge *> &PiSDFVertex::outputEdges() const {
    return outputEdgeArray_;
}

const Spider::Array<PiSDFParam *> &PiSDFVertex::inputParams() const {
    return inputParamArray_;
}

const Spider::Array<PiSDFParam *> &PiSDFVertex::outputParams() const {
    return outputParamArray_;
}

std::uint32_t PiSDFVertex::repetitionValue() const {
    return repetitionValue_;
}

PiSDFGraph *PiSDFVertex::subgraph() const {
    return subgraph_;
}

std::uint32_t PiSDFVertex::getIx() const {
    return ix_;
}

#endif //SPIDER2_PISDFVERTEX_H
