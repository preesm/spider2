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
#ifndef SPIDER2_EDGE_H
#define SPIDER2_EDGE_H

/* === Include(s) === */

#include <cstdint>
#include <string>
#include <functional>
#include <memory/Allocator.h>
#include <graphs-tools/expression-parser/Expression.h>

namespace Spider {
    namespace PiSDF {

        /* === Forward declaration(s) === */

        class Vertex;

        /* === Class definition === */

        class Edge {
        public:

            Edge(Vertex *source,
                 std::uint32_t srcIx,
                 Expression &&srcExpr,
                 Vertex *sink,
                 std::uint32_t snkIx,
                 Expression &&snkExpr);

            ~Edge();

            /* === Method(s) === */

            /**
             * @brief Build and return a name of the edge.
             * @return Name of the edge in format "#source -> #sink"
             */
            std::string name() const;

            /* === Getter(s) === */

            /**
             * @brief Get the containing @refitem PiSDFGraph of the edge.
             * @return containing @refitem PiSDFGraph
             */
            inline Graph *containingGraph() const;

            /**
             * @brief Get the ix of the edge in the containing graph.
             * @return ix of the edge (UINT32_MAX if no ix).
             */
            inline std::uint32_t ix() const;

            /**
             * @brief Get the source port ix of the edge
             * @return source port ix
             */
            inline std::uint32_t sourcePortIx() const;

            /**
             * @brief Get the sink port ix of the edge
             * @return sink port ix
             */
            inline std::uint32_t sinkPortIx() const;

            /**
             * @brief Evaluate the expression rate of the source.
             * @return @refitem Expression of the source rate .
             */
            inline const Expression &sourceRateExpression() const;

            /**
             * @brief Evaluate the expression rate of the sink.
             * @return @refitem Expression of the sink rate.
             */
            inline const Expression &sinkRateExpression() const;

            /**
             * @brief Get the source reference vertex.
             * @return reference to source
             */
            template<bool = false>
            inline Vertex *source() const;

            /**
             * @brief Get the sink reference vertex.
             * @return reference to sink
             */
            template<bool = false>
            inline Vertex *sink() const;

            /**
             * @brief Get the delay (if any) associated to the edge.
             * @return @refitem PiSDFDelay of the edge.
             */
            inline const Delay *delay() const;

            /* === Setter(s) === */

            /**
             * @brief Set the ix of the edge in the containing graph.
             * @remark This method will override current value.
             * @param ix Ix to set.
             */
            inline void setIx(std::uint32_t ix);

            /**
             * @brief Set the source vertex of the edge.
             * @remark This method disconnect any previous connected edge on vertex and disconnect current source.
             * @param vertex  Vertex to connect to.
             * @param ix      Output port ix.
             * @param expr    Expression of the rate.
             * @return pointer to the old output @refitem Edge of vertex, nullptr else.
             * @throws @refitem Spider::Exception if vertex is nullptr.
             */
            Edge *setSource(Vertex *vertex, std::uint32_t ix, Expression &&expr);

            /**
             * @brief Set the sink vertex of the edge.
             * @remark This method disconnect any previous connected edge on vertex and disconnect current sink.
             * @param vertex  Vertex to connect to.
             * @param ix      Input port ix.
             * @param expr    Expression of the rate.
             * @return pointer to the old input @refitem Edge of vertex, nullptr else.
             * @throws @refitem Spider::Exception if vertex is nullptr.
             */
            Edge *setSink(Vertex *vertex, std::uint32_t ix, Expression &&expr);

            inline void setDelay(Delay *delay);

        private:
            Graph *graph_ = nullptr;
            std::uint32_t ix_ = UINT32_MAX;

            Vertex *src_;
            std::uint32_t srcPortIx_ = UINT32_MAX;
            Expression srcExpression_;
            Vertex *snk_;
            std::uint32_t snkPortIx_ = UINT32_MAX;
            Expression snkExpression_;

            Delay *delay_ = nullptr;

            /* === Private method(s) === */
        };

        /* === Inline method(s) === */

        Graph *Edge::containingGraph() const {
            return graph_;
        }

        std::uint32_t Edge::ix() const {
            return ix_;
        }

        std::uint32_t Edge::sourcePortIx() const {
            return srcPortIx_;
        }

        std::uint32_t Edge::sinkPortIx() const {
            return snkPortIx_;
        }

        const Expression &Edge::sourceRateExpression() const {
            return srcExpression_;
        }

        const Expression &Edge::sinkRateExpression() const {
            return snkExpression_;
        }

        template<bool>
        Vertex *Edge::source() const {
            return src_;
        }

        template<bool>
        Vertex *Edge::sink() const {
            return snk_;
        }

        const Delay *Edge::delay() const {
            return delay_;
        }

        void Edge::setIx(std::uint32_t ix) {
            ix_ = ix;
        }

        void Edge::setDelay(Delay *delay) {
            if (!delay) {
                return;
            } else if (delay_) {
                throwSpiderException("Cannot set delay. Edge [%s] already has a delay.", name().c_str());
            }
            delay_ = delay;
        }
    }
}
#endif //SPIDER2_GRAPH_H
