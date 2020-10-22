/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_SCHEDGRAPH_H
#define SPIDER2_SCHEDGRAPH_H

/* === Include(s) === */

#include <graphs/sched/SchedEdge.h>
#include <graphs/sched/SchedVertex.h>
#include <graphs/pisdf/Graph.h>
#include <memory/unique_ptr.h>

namespace spider {

    namespace srdag {
        class Vertex;
    }

    namespace pisdf {
        class Vertex;

        class GraphFiring;
    }

    namespace sched {

        /* === Class definition === */

        class Graph {
        public:
            explicit Graph() : vertexVector_{ factory::vector<unique_ptr<sched::Vertex>>(StackID::SCHEDULE) },
                               edgeVector_{ factory::vector<unique_ptr<sched::Edge>>(StackID::SCHEDULE) } {

            }

            Graph(Graph &&) noexcept = default;

            Graph &operator=(Graph &&) = default;

            Graph(const Graph &) = default;

            Graph &operator=(const Graph &) = default;

            ~Graph() = default;

            /* === Method(s) === */

            sched::Edge *createEdge(sched::Vertex *source, u32 srcIx, sched::Vertex *sink, u32 snkIx, Fifo alloc);

            /**
             * @brief Clears a graph without destroying it.
             */
            void clear();

            /**
             * @brief Add a vertex to the graph.
             * @param vertex Vertex to add.
             */
            void addVertex(sched::Vertex *vertex);

            /**
             * @brief Remove a vertex from the graph.
             * @remark If vertex is nullptr, nothing happens.
             * @param vertex Vertex to remove.
             * @throw @refitem Spider::Exception if vertex does not exist in the graph.
             */
            void removeVertex(sched::Vertex *vertex);

            /**
             * @brief Add an edge to the graph.
             * @param edge Edge to add.
             */
            void addEdge(sched::Edge *edge);

            /**
             * @brief Remove an edge from the graph.
             * @remark If edge is nullptr, nothing happens.
             * @param edge Edge to remove.
             * @throw @refitem Spider::Exception if edge does not exist in the graph.
             */
            void removeEdge(sched::Edge *edge);

            /* === Getter(s) === */

            /**
             * @brief A const reference on the vector of vertex. Useful for iterating on the vertices.
             * @return const reference to vertex vector
             */
            inline const vector<unique_ptr<sched::Vertex>> &vertices() const { return vertexVector_; }

            /**
             * @brief Return the vertex corresponding to the ix.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the vertex.
             * @return @refitem Vertex pointer
             */
            inline sched::Vertex *vertex(size_t ix) const { return vertexVector_[ix].get(); }

            /**
             * @brief Get the number of vertices in the graph.
             * @remark This method exclude the number of interfaces and the number of config actors.
             * @return Total number of vertices.
             */
            inline size_t vertexCount() const { return vertexVector_.size(); }

            /**
             * @brief A const reference on the set of edges. Useful for iterating on the edges.
             * @return const reference to edge vector
             */
            inline const vector<unique_ptr<sched::Edge>> &edges() const { return edgeVector_; }

            /**
             * @brief Get the number of edges contained in the graph.
             * @return Number of edges.
             */
            inline size_t edgeCount() const { return edgeVector_.size(); }

            /* === Setter(s) === */

        private:
            spider::vector<spider::unique_ptr<sched::Vertex>> vertexVector_; /* = Vector of all the Vertices of the graph = */
            spider::vector<spider::unique_ptr<sched::Edge>> edgeVector_;     /* = Vector of Edge contained in the graph = */
        };
    }
}
#endif //SPIDER2_SCHEDGRAPH_H
