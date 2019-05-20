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
#ifndef SPIDER2_PISDFEDGE_H
#define SPIDER2_PISDFEDGE_H

/* === Includes === */

#include <cstdint>
#include <string>
#include <common/containers/Set.h>

/* === Forward declaration(s) === */

class PiSDFVertex;

class PiSDFGraph;

/* === Class definition === */

class PiSDFEdge : public Spider::SetElement {
public:

    PiSDFEdge(PiSDFGraph *graph, PiSDFVertex *source, PiSDFVertex *sink);

    ~PiSDFEdge() = default;

    /* === Methods === */

    /* === Setters === */

    inline void setSource(PiSDFVertex *vertex, std::uint32_t srcPortIx, std::string prodExpr);

    inline void setSink(PiSDFVertex *vertex, std::uint32_t snkPortIx, std::string consExpr);

    /* === Getters ===  */

    /**
     * @brief Get the containing @refitem PiSDFGraph of the edge.
     * @return containing @refitem PiSDFGraph
     */
    inline PiSDFGraph *containingGraph() const;

    /**
     * @brief Get the source @refitem PiSDFVertex of the edge.
     * @return source @refitem PiSDFVertex
     */
    inline PiSDFVertex *source() const;

    /**
     * @brief Get the sink @refitem PiSDFVertex of the edge.
     * @return sink @refitem PiSDFVertex
     */
    inline PiSDFVertex *sink() const;

    /**
     * @brief Get the source rate of the edge.
     * @return source rate
     */
    inline std::uint32_t sourcePortIx() const;

    /**
     * @brief Get the sink rate of the edge.
     * @return sink rate
     */
    inline std::uint32_t sinkPortIx() const;

    /**
     * @brief Get the source rate of the edge.
     * @return source rate
     */
    std::uint64_t sourceRate() const;

    /**
     * @brief Get the sink rate of the edge.
     * @return sink rate
     */
    std::uint64_t sinkRate() const;

private:
    PiSDFGraph *graph_ = nullptr;
    PiSDFVertex *source_ = nullptr;
    PiSDFVertex *sink_ = nullptr;

    std::uint32_t sourcePortIx_ = 0;
    std::uint32_t sinkPortIx_ = 0;

    std::uint64_t sourceRate_ = 0;
    std::uint64_t sinkRate_ = 0;
};

/* === Inline methdos === */

PiSDFGraph *PiSDFEdge::containingGraph() const {
    return graph_;
}

PiSDFVertex *PiSDFEdge::source() const {
    return source_;
}

PiSDFVertex *PiSDFEdge::sink() const {
    return sink_;
}

std::uint32_t PiSDFEdge::sourcePortIx() const {
    return sourcePortIx_;
}


std::uint32_t PiSDFEdge::sinkPortIx() const {
    return sinkPortIx_;
}

void PiSDFEdge::setSource(PiSDFVertex *vertex, std::uint32_t srcPortIx, std::string /* prodExpr */){
    source_ = vertex;
    sourcePortIx_ = srcPortIx;
    // TODO: set prod Expression
}

void PiSDFEdge::setSink(PiSDFVertex *vertex, std::uint32_t snkPortIx, std::string /* consExpr */) {
    sink_ = vertex;
    sinkPortIx_ = snkPortIx;
    // TODO: set cons Expression
}

#endif //SPIDER2_PISDFEDGE_H
