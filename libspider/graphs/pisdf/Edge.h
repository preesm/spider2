/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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
#ifndef SPIDER2_EDGE_H
#define SPIDER2_EDGE_H

/* === Include(s) === */

#include <memory/unique_ptr.h>
#include <graphs/pisdf/Delay.h>
#include <graphs-tools/expression-parser/Expression.h>

namespace spider {
    namespace pisdf {

        /* === Forward declaration(s) === */

        class Vertex;

        class Graph;

        /* === Class definition === */

        class Edge {
        public:

            Edge(Vertex *source, size_t srcIx, Expression srcExpr, Vertex *sink, size_t snkIx, Expression snkExpr);

            ~Edge() noexcept = default;

            /* === Method(s) === */

            /**
             * @brief Build and return a name of the edge.
             * @return Name of the edge in format "#source -> #sink"
             */
            std::string name() const;

            /* === Getter(s) === */

            /**
             * @brief Get the containing @refitem spider::pisdf::Graph of the edge.
             * @return containing @refitem spider::pisdf::Graph
             */
            Graph *graph() const;

            /**
             * @brief Get the delay (if any) associated to the edge.
             * @return @refitem PiSDFDelay of the edge.
             */
            Delay *delay() const;

            /**
             * @brief Get the ix of the edge in the containing graph.
             * @return ix of the edge (SIZE_MAX if no ix).
             */
            inline size_t ix() const { return ix_; }

            /**
             * @brief Get the source port ix of the edge
             * @return source port ix
             */
            inline size_t sourcePortIx() const { return srcPortIx_; }

            /**
             * @brief Get the sink port ix of the edge
             * @return sink port ix
             */
            inline size_t sinkPortIx() const { return snkPortIx_; }

            /**
             * @brief Evaluate the expression rate of the source.
             * @return @refitem Expression of the source rate .
             */
            inline const Expression &sourceRateExpression() const { return *(srcExpression_.get()); }

            /**
             * @brief Shortcurt for the method of @refitem Expression::value.
             * @return value of the source rate expression.
             */
            int64_t sourceRateValue() const;

            /**
             * @brief Evaluate the expression rate of the sink.
             * @return @refitem Expression of the sink rate.
             */
            inline const Expression &sinkRateExpression() const { return *(snkExpression_.get()); }

            /**
             * @brief Shortcurt for the method of @refitem Expression::value.
             * @return value of the sink rate expression.
             */
            int64_t sinkRateValue() const;

            /**
             * @brief Get the source reference vertex.
             * @return reference to source
             */
            inline Vertex *source() const { return src_; }

            /**
             * @brief Get the sink reference vertex.
             * @return reference to sink
             */
            inline Vertex *sink() const { return snk_; }

            /* === Setter(s) === */

            /**
             * @brief Set the delay associated with the Edge.
             * @param delay Pointer to the delay to set.
             */
            void setDelay(Delay *delay);

            /**
             * @brief Set the ix of the edge in the containing graph.
             * @remark This method will override current value.
             * @param ix Ix to set.
             */
            inline void setIx(size_t ix) { ix_ = ix; }

            /**
             * @brief Set the source vertex of the edge.
             * @remark This method disconnect any previous connected edge on vertex and disconnect current source.
             * @param vertex  Vertex to connect to.
             * @param ix      Output port ix.
             * @param expr    Expression of the rate.
             * @return pointer to the old output @refitem Edge of vertex, nullptr else.
             */
            void setSource(Vertex *vertex, size_t ix, Expression expr);

            /**
             * @brief Set the sink vertex of the edge.
             * @remark This method disconnect any previous connected edge on vertex and disconnect current sink.
             * @param vertex  Vertex to connect to.
             * @param ix      Input port ix.
             * @param expr    Expression of the rate.
             * @return pointer to the old input @refitem Edge of vertex, nullptr else.
             */
            void setSink(Vertex *vertex, size_t ix, Expression expr);

        private:
            spider::unique_ptr<Expression> srcExpression_;     /* = Expression of the source rate of the Edge = */
            spider::unique_ptr<Expression> snkExpression_;     /* = Expression of the sink rate of the Edge = */
            Vertex *src_ = nullptr;        /* = Pointer to the source Vertex (if any) = */
            Vertex *snk_ = nullptr;        /* = Pointer to the sink Vertex (if any) = */
            unique_ptr<Delay> delay_;      /* = Pointer to Delay associated to the Edge (if any) = */
            size_t ix_ = SIZE_MAX;         /* = Index of the Edge in the Graph (used for add and remove) = */
            size_t srcPortIx_ = SIZE_MAX;  /* = Index of the Edge in the source outputEdgeArray = */
            size_t snkPortIx_ = SIZE_MAX;  /* = Index of the Edge in the sink inputEdgeArray = */
        };
    }
}
#endif //SPIDER2_GRAPH_H
