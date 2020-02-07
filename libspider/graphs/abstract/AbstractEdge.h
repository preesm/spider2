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
#ifndef SPIDER2_ABSTRACTEDGE_H
#define SPIDER2_ABSTRACTEDGE_H

/* === Include(s) === */

#include <graphs-tools/expression-parser/Expression.h>

namespace spider {

    /* === Class definition === */

    template<class V, class E>
    class AbstractEdge {
    public:

        AbstractEdge(V *source, size_t srcIx, Expression srcExpr,
                     V *sink, size_t snkIx, Expression snkExpr) : srcExpression_{ std::move(srcExpr) },
                                                                  snkExpression_{ std::move(snkExpr) },
                                                                  src_{ source },
                                                                  snk_{ sink },
                                                                  srcPortIx_{ srcIx },
                                                                  snkPortIx_{ snkIx } {
            if (!source || !sink) {
                throwSpiderException("nullptr vertex connected to Edge.");
            }
            if (source->graph() != sink->graph()) {
                throwSpiderException("Can not create edge between [%s] and [%s]: not in the same graph.",
                                     source->name().c_str(), sink->name().c_str());
            }
            source->connectOutputEdge(static_cast<E *>(this), srcIx);
            sink->connectInputEdge(static_cast<E *>(this), snkIx);
        }

        virtual ~AbstractEdge() noexcept = default;

        /* === Method(s) === */

        /**
         * @brief Build and return a name of the edge.
         * @return Name of the edge in format "#source -> #sink"
         */
        inline std::string name() const {
            return "edge_" + src_->name() + "-" + snk_->name();
        }

        /* === Getter(s) === */

        /**
         * @brief Get the ix of the edge in the containing graph.
         * @return ix of the edge (SIZE_MAX if no ix).
         */
        inline size_t ix() const {
            return ix_;
        }

        /**
         * @brief Get the source port ix of the edge
         * @return source port ix
         */
        inline size_t sourcePortIx() const {
            return srcPortIx_;
        }

        /**
         * @brief Get the sink port ix of the edge
         * @return sink port ix
         */
        inline size_t sinkPortIx() const {
            return snkPortIx_;
        }

        /**
         * @brief Evaluate the expression rate of the source.
         * @return @refitem Expression of the source rate .
         */
        inline const Expression &sourceRateExpression() const {
            return srcExpression_;
        }

        /**
         * @brief Evaluates the sourceRateExpression with source input param.
         * @return value of the source rate expression.
         */
        inline int64_t sourceRateValue() const {
            return src_ ? srcExpression_.evaluate(src_->inputParamVector()) : 0;
        }

        /**
         * @brief Evaluate the expression rate of the sink.
         * @return @refitem Expression of the sink rate.
         */
        inline const Expression &sinkRateExpression() const {
            return snkExpression_;
        }

        /**
         * @brief Evaluates the sinkRateExpression with source input param.
         * @return value of the sink rate expression.
         */
        inline int64_t sinkRateValue() const {
            return snk_ ? snkExpression_.evaluate(snk_->inputParamVector()) : 0;
        }


        /**
         * @brief Get the source reference vertex.
         * @return reference to source
         */
        inline V *source() const {
            return src_;
        }

        /**
         * @brief Get the sink reference vertex.
         * @return reference to sink
         */
        inline V *sink() const {
            return snk_;
        }

        /* === Setter(s) === */

        /**
         * @brief Set the ix of the edge in the containing graph.
         * @remark This method will override current value.
         * @param ix Ix to set.
         */
        inline void setIx(size_t ix) {
            ix_ = ix;
        }

        /**
         * @brief Set the source vertex of the edge.
         * @remark This method disconnect any previous connected edge on vertex and disconnect current source.
         * @param vertex  Vertex to connect to.
         * @param ix      Output port ix.
         * @param expr    Expression of the rate.
         * @return pointer to the old output @refitem Edge of vertex, nullptr else.
         */
        virtual void setSource(V *vertex, size_t ix, Expression expr);

        /**
         * @brief Set the sink vertex of the edge.
         * @remark This method disconnect any previous connected edge on vertex and disconnect current sink.
         * @param vertex  Vertex to connect to.
         * @param ix      Input port ix.
         * @param expr    Expression of the rate.
         * @return pointer to the old input @refitem Edge of vertex, nullptr else.
         */
        virtual void setSink(V *vertex, size_t ix, Expression expr);

    protected:
        Expression srcExpression_;    /* = Expression of the source rate of the Edge = */
        Expression snkExpression_;    /* = Expression of the sink rate of the Edge = */
        V *src_ = nullptr;            /* = Pointer to the source Vertex (if any) = */
        V *snk_ = nullptr;            /* = Pointer to the sink Vertex (if any) = */
        size_t ix_ = SIZE_MAX;        /* = Index of the Edge in the Graph (used for add and remove) = */
        size_t srcPortIx_ = SIZE_MAX; /* = Index of the Edge in the source outputEdgeArray = */
        size_t snkPortIx_ = SIZE_MAX; /* = Index of the Edge in the sink inputEdgeArray = */

    };

    template<class V, class E>
    inline void spider::AbstractEdge<V, E>::setSource(V *vertex, size_t ix, Expression expr) {
        if (vertex) {
            if (snk_ && vertex->graph() != snk_->graph()) {
                throwSpiderException("Can not set edge between [%s] and [%s]: not in the same graph.",
                                     vertex->name().c_str(), snk_->name().c_str());
            }
            /* == Disconnect current output edge (if any) == */
            vertex->disconnectOutputEdge(ix);

            /* == Connect this edge == */
            vertex->connectOutputEdge(static_cast<E *>(this), ix);
        }

        /* == Disconnect current src (if any) == */
        if (src_) {
            src_->disconnectOutputEdge(srcPortIx_);
        }

        /* == Set source of this edge == */
        src_ = vertex;
        srcPortIx_ = ix;
        srcExpression_ = std::move(expr);
    }

    template<class V, class E>
    inline void spider::AbstractEdge<V, E>::setSink(V *vertex, size_t ix, Expression expr) {
        if (vertex) {
            if (src_ && vertex->graph() != src_->graph()) {
                throwSpiderException("Can not set edge between [%s] and [%s]: not in the same graph.",
                                     src_->name().c_str(), vertex->name().c_str());
            }
            /* == Disconnect current input edge (if any) == */
            vertex->disconnectInputEdge(ix);

            /* == Connect this edge == */
            vertex->connectInputEdge(static_cast<E *>(this), ix);
        }

        /* == Disconnect current snk_ (if any) == */
        if (snk_) {
            snk_->disconnectInputEdge(snkPortIx_);
        }

        /* == Set sink of this edge == */
        snk_ = vertex;
        snkPortIx_ = ix;
        snkExpression_ = std::move(expr);
    }
}

#endif //SPIDER2_ABSTRACTEDGE_H
