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
#include <memory/alloc.h>
#include <graphs-tools/expression-parser/Expression.h>

namespace spider {
    namespace pisdf {

        /* === Forward declaration(s) === */

        class Vertex;

        /* === Class definition === */

        class Edge {
        public:

            Edge(Vertex *source,
                 uint32_t srcIx,
                 Expression &&srcExpr,
                 Vertex *sink,
                 uint32_t snkIx,
                 Expression &&snkExpr);

            ~Edge();

            /* === Method(s) === */

            /**
             * @brief Build and return a name of the edge.
             * @return Name of the edge in format "#source -> #sink"
             */
            std::string name() const;

            /**
             * @brief Remove delay of the Edge and destroy it.
             */
            void removeDelay();

            /* === Getter(s) === */

            /**
             * @brief Get the containing @refitem PiSDFGraph of the edge.
             * @return containing @refitem PiSDFGraph
             */
            inline Graph *graph() const {
                return graph_;
            }

            /**
             * @brief Get the ix of the edge in the containing graph.
             * @return ix of the edge (UINT32_MAX if no ix).
             */
            inline uint32_t ix() const {
                return ix_;
            }

            /**
             * @brief Get the source port ix of the edge
             * @return source port ix
             */
            inline uint32_t sourcePortIx() const {
                return srcPortIx_;
            }

            /**
             * @brief Get the sink port ix of the edge
             * @return sink port ix
             */
            inline uint32_t sinkPortIx() const {
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
             * @brief Evaluate the expression rate of the sink.
             * @return @refitem Expression of the sink rate.
             */
            inline const Expression &sinkRateExpression() const {
                return snkExpression_;
            }

            /**
             * @brief Get the source reference vertex.
             * @return reference to source
             */
            inline Vertex *source() const {
                return src_;
            }

            Vertex *sourceFw() const;

            /**
             * @brief Get the sink reference vertex.
             * @return reference to sink
             */
            inline Vertex *sink() const {
                return snk_;
            }

            Vertex *sinkFw() const;

            /**
             * @brief Get the delay (if any) associated to the edge.
             * @return @refitem PiSDFDelay of the edge.
             */
            inline const Delay *delay() const {
                return delay_;
            }

            /* === Setter(s) === */

            /**
             * @brief Set the ix of the edge in the containing graph.
             * @remark This method will override current value.
             * @param ix Ix to set.
             */
            inline void setIx(uint32_t ix) {
                ix_ = ix;
            }

            /**
             * @brief Set the source vertex of the edge.
             * @remark This method disconnect any previous connected edge on vertex and disconnect current source.
             * @param vertex  Vertex to connect to.
             * @param ix      Output port ix.
             * @param expr    Expression of the rate.
             * @return pointer to the old output @refitem Edge of vertex, nullptr else.
             * @throws @refitem Spider::Exception if vertex is nullptr.
             */
            Edge *setSource(Vertex *vertex, uint32_t ix, Expression &&expr);

            /**
             * @brief Set the sink vertex of the edge.
             * @remark This method disconnect any previous connected edge on vertex and disconnect current sink.
             * @param vertex  Vertex to connect to.
             * @param ix      Input port ix.
             * @param expr    Expression of the rate.
             * @return pointer to the old input @refitem Edge of vertex, nullptr else.
             * @throws @refitem Spider::Exception if vertex is nullptr.
             */
            Edge *setSink(Vertex *vertex, uint32_t ix, Expression &&expr);

            inline void setDelay(Delay *delay) {
                if (!delay) {
                    return;
                } else if (delay_) {
                    throwSpiderException("Cannot set delay. Edge [%s] already has a delay.", name().c_str());
                }
                delay_ = delay;
            }

            inline void setGraph(Graph *graph) {
                if (graph) {
                    graph_ = graph;
                }
            }

        private:
            Expression srcExpression_;
            Expression snkExpression_;
            Graph *graph_ = nullptr;
            Vertex *src_ = nullptr;
            Vertex *snk_ = nullptr;
            Delay *delay_ = nullptr;
            uint32_t ix_ = UINT32_MAX;
            uint32_t srcPortIx_ = UINT32_MAX;
            uint32_t snkPortIx_ = UINT32_MAX;
        };
    }
}
#endif //SPIDER2_GRAPH_H
