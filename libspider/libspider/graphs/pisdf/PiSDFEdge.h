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
#ifndef SPIDER2_PISDFEDGE_H
#define SPIDER2_PISDFEDGE_H

/* === Includes === */

#include <cstdint>
#include <string>
#include <graphs-tools/expression-parser/Expression.h>
#include <graphs/pisdf/PiSDFPort.h>

/* === Forward declaration(s) === */

class PiSDFVertex;

class PiSDFGraph;

class PiSDFDelay;

/* === Class definition === */

class PiSDFEdge {
public:

    PiSDFEdge(PiSDFGraph *graph,
              PiSDFVertex *source,
              std::uint16_t srcPortIx,
              const std::string &prodExpr,
              PiSDFVertex *sink,
              std::uint16_t snkPortIx,
              const std::string &consExpr);

    PiSDFEdge(PiSDFGraph *graph,
              PiSDFVertex *source,
              std::uint16_t srcPortIx,
              std::int64_t srcRate,
              PiSDFVertex *sink,
              std::uint16_t snkPortIx,
              std::int64_t snkRate);

    ~PiSDFEdge();

    /* === Methods === */

    /**
     * @brief Export edge in the dot format to the given file.
     * @param file   File to which the edge should be exported.
     */
    void exportDot(FILE *file, const std::string &offset) const;

    /**
     * @brief Connect the source vertex of the edge.
     * @param vertex    Source vertex.
     * @param portIx    Output port ix on the vertex.
     * @param prodExpr  Rate expression.
     * @throws @refitem SpiderException if edge is already connected to a source vertex.
     */
    void connectSource(PiSDFVertex *vertex, std::uint16_t portIx, const std::string &prodExpr);

    /**
     * @brief Connect the source vertex of the edge.
     * @param vertex    Source vertex.
     * @param portIx    Output port ix on the vertex.
     * @param prod      Rate value.
     * @throws @refitem SpiderException if edge is already connected to a source vertex.
     */
    void connectSource(PiSDFVertex *vertex, std::uint16_t portIx, std::int64_t prod);

    /**
     * @brief Connect the sink vertex of the edge.
     * @param vertex    Sink vertex.
     * @param portIx    Input port ix on the vertex.
     * @param consExpr  Rate expression.
     * @throws @refitem SpiderException if edge is already connected to a sink vertex.
     */
    void connectSink(PiSDFVertex *vertex, std::uint32_t portIx, const std::string &consExpr);

    /**
     * @brief Connect the sink vertex of the edge.
     * @param vertex    Sink vertex.
     * @param portIx    Input port ix on the vertex.
     * @param cons      Rate value.
     * @throws @refitem SpiderException if edge is already connected to a sink vertex.
     */
    void connectSink(PiSDFVertex *vertex, std::uint32_t portIx, std::int64_t cons);

    /**
     * @brief Disconnect current source vertex of the edge.
     */
    void disconnectSource();

    /**
     * @brief Disconnect current sink vertex of the edge.
     */
    void disconnectSink();

    /**
     * @brief Build and return a name of the edge.
     * @return Name of the edge in format "#source -> #sink"
     */
    std::string name() const;

    /* === Setters === */

    /**
     * @brief Set the ix of the edge in the containing graph.
     * @param ix Ix to set.
     */
    inline void setIx(std::uint32_t ix);

    /**
     * @brief Set the delay of the edge.
     * If nullptr is passed, nothing happens.
     * @param delay Delay to set.
     * @throws @refitem SpiderException if edge already has a delay.
     */
    inline void setDelay(PiSDFDelay *delay);

    /* === Getters ===  */

    /**
     * @brief Get the containing @refitem PiSDFGraph of the edge.
     * @return containing @refitem PiSDFGraph
     */
    inline PiSDFGraph *containingGraph() const;

    /**
     * @brief Get the source @refitem PiSDFVertex of the edge.
     * @param forward  Enable forwarding of source if it is an interface (default false).
     * @return source @refitem PiSDFVertex
     * @remark if source is hierarchical, return vertex connected to the interface
     */
    PiSDFVertex *source(bool forward = false) const;

    /**
     * @brief Get the sink @refitem PiSDFVertex of the edge.
     * @param forward  Enable forwarding of sink if it is an interface (default false).
     * @return sink @refitem PiSDFVertex
     * @remark if sink is hierarchical, return vertex connected to the interface
     */
    PiSDFVertex *sink(bool forward = false) const;

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
     * @brief Get the ix of the edge in the containing graph.
     * @return ix of the edge (UINT32_MAX if no ix).
     */
    inline std::uint32_t ix() const;

    /**
     * @brief Get the delay (if any) associated to the edge.
     * @return @refitem PiSDFDelay of the edge.
     */
    inline const PiSDFDelay *delay() const;

    /**
     * @brief Return the delay value (if any).
     * @return value of associated delay, 0 else.
     */
    std::int64_t delayValue() const;

private:
    PiSDFGraph *graph_ = nullptr;
    PiSDFVertex *source_ = nullptr;
    PiSDFVertex *sink_ = nullptr;

    PiSDFPort *sinkPort_ = nullptr;
    PiSDFPort *sourcePort_ = nullptr;

    std::uint32_t ix_ = UINT32_MAX;

    PiSDFDelay *delay_ = nullptr;
};

/* === Inline method(s) === */

void PiSDFEdge::setIx(std::uint32_t ix) {
    ix_ = ix;
}

void PiSDFEdge::setDelay(PiSDFDelay *delay) {
    if (!delay) {
        return;
    } else if (delay_) {
        throwSpiderException("Cannot set delay. Edge [%s] already has a delay.", name().c_str());
    }
    delay_ = delay;
}

PiSDFGraph *PiSDFEdge::containingGraph() const {
    return graph_;
}

std::uint32_t PiSDFEdge::sourcePortIx() const {
    return sourcePort_->ix();
}

std::uint32_t PiSDFEdge::sinkPortIx() const {
    return sinkPort_->ix();
}

std::uint32_t PiSDFEdge::ix() const {
    return ix_;
}

const PiSDFDelay *PiSDFEdge::delay() const {
    return delay_;
}

#endif //SPIDER2_PISDFEDGE_H
