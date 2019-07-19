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
#include <common/expression-parser/Expression.h>
#include <graphs/pisdf/PiSDFInterface.h>

/* === Forward declaration(s) === */

class PiSDFVertex;

class PiSDFGraph;


/* === Class definition === */

class PiSDFEdge : public Spider::SetElement {
public:

    PiSDFEdge(PiSDFGraph *graph,
              PiSDFVertex *source,
              std::uint32_t srcPortIx,
              const std::string &prodExpr,
              PiSDFVertex *sink,
              std::uint32_t snkPortIx,
              const std::string &consExpr);

    PiSDFEdge(PiSDFGraph *graph,
              PiSDFInterface *sourceIf,
              const std::string &prodExpr,
              PiSDFInterface *sinkIf,
              const std::string &consExpr);

    PiSDFEdge(PiSDFGraph *graph,
              PiSDFInterface *sourceIf,
              const std::string &prodExpr,
              PiSDFVertex *sink,
              std::uint32_t snkPortIx,
              const std::string &consExpr);

    PiSDFEdge(PiSDFGraph *graph,
              PiSDFVertex *source,
              std::uint32_t srcPortIx,
              const std::string &prodExpr,
              PiSDFInterface *sinkIf,
              const std::string &consExpr);

    ~PiSDFEdge();

    /* === Methods === */

    /**
     * @brief Export edge in the dot format to the given file.
     * @param file   File to which the edge should be exported.
     */
    void exportDot(FILE *file, const std::string &offset) const;

    /* === Setters === */

    void setSource(PiSDFVertex *vertex, std::uint32_t srcPortIx, const std::string &prodExpr);

    void setSource(PiSDFInterface *interface, const std::string &prodExpr);

    void setSink(PiSDFVertex *vertex, std::uint32_t snkPortIx, const std::string &consExpr);

    void setSink(PiSDFInterface *interface, const std::string &consExpr);

    /* === Getters ===  */

    /**
     * @brief Get the containing @refitem PiSDFGraph of the edge.
     * @return containing @refitem PiSDFGraph
     */
    inline const PiSDFGraph *containingGraph() const;

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

    /**
     * @brief Get the sink @refitem PiSDFInterface of the edge (if any).
     * @return sink @refitem PiSDFInterface
     */
    inline PiSDFInterface *sinkIf() const;

    /**
     * @brief Get the source @refitem PiSDFInterface of the edge (if any).
     * @return source @refitem PiSDFInterface
     */
    inline PiSDFInterface *sourceIf() const;

private:
    PiSDFGraph *graph_ = nullptr;
    PiSDFVertex *source_ = nullptr;
    PiSDFVertex *sink_ = nullptr;

    std::uint32_t sourcePortIx_ = 0;
    std::uint32_t sinkPortIx_ = 0;

    Expression *sourceRateExpr_ = nullptr;
    Expression *sinkRateExpr_ = nullptr;

    PiSDFInterface *sourceIf_ = nullptr;
    PiSDFInterface *sinkIf_ = nullptr;
};

/* === Inline methdos === */

const PiSDFGraph *PiSDFEdge::containingGraph() const {
    return graph_;
}

PiSDFVertex *PiSDFEdge::source() const {
    if (!source_ && sourceIf_) {
        return sourceIf_->inputEdge()->source();
    }
    return source_;
}

PiSDFVertex *PiSDFEdge::sink() const {
    if (!sink_ && sinkIf_) {
        return sinkIf_->outputEdge()->sink();
    }
    return sink_;
}

std::uint32_t PiSDFEdge::sourcePortIx() const {
    return sourcePortIx_;
}


std::uint32_t PiSDFEdge::sinkPortIx() const {
    return sinkPortIx_;
}

PiSDFInterface *PiSDFEdge::sourceIf() const {
    return sourceIf_;
}

PiSDFInterface *PiSDFEdge::sinkIf() const {
    return sinkIf_;
}

#endif //SPIDER2_PISDFEDGE_H
