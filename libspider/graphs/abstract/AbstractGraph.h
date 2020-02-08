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
#ifndef SPIDER2_ABSTRACTGRAPH_H
#define SPIDER2_ABSTRACTGRAPH_H

/* === Include(s) === */

#include <containers/vector.h>
#include <memory/unique_ptr.h>
#include <graphs-tools/expression-parser/Expression.h>

namespace spider {

    /* === Class definition === */

    template<class G, class V, class E>
    class AbstractGraph {
    public:
        template<StackID stack>
        explicit AbstractGraph(stack_t<stack>, size_t vertexCount = 0, size_t edgeCount = 0) :
                vertexVector_{ sbc::vector<unique_ptr<V>, stack>{ }},
                edgeVector_{ sbc::vector<unique_ptr<E>, stack>{ }} {
            vertexVector_.reserve(vertexCount);
            edgeVector_.reserve(edgeCount);
        }

        virtual ~AbstractGraph() noexcept = default;

        /* === Method(s) === */

        /**
         * @brief Clears a graph without destroying it.
         */
        virtual inline void clear() {
            edgeVector_.clear();
            vertexVector_.clear();
        }

        /**
         * @brief Add a vertex to the graph.
         * @param vertex Vertex to add.
         */
        virtual inline void addVertex(V *vertex) {
            if (!vertex) {
                return;
            }
            vertex->setIx(vertexVector_.size());
            vertex->setGraph(static_cast<G *>(this));
            vertexVector_.emplace_back(vertex);
        }

        /**
         * @brief Remove a vertex from the graph.
         * @remark If vertex is nullptr, nothing happens.
         * @param vertex Vertex to remove.
         * @throw @refitem Spider::Exception if vertex does not exist in the graph.
         */
        virtual inline void removeVertex(V *vertex) {
            if (!vertex) {
                return;
            }
            /* == Assert that vertex is part of the edgeVector_ == */
            assertElement(vertex, vertexVector_);
            /* == Reset vertex input edges == */
            for (auto &edge : vertex->inputEdgeVector()) {
                if (edge) {
                    edge->setSink(nullptr, SIZE_MAX, Expression());
                }
            }
            /* == Reset vertex output edges == */
            for (auto &edge : vertex->outputEdgeVector()) {
                if (edge) {
                    edge->setSource(nullptr, SIZE_MAX, Expression());
                }
            }
            /* == swap and destroy the element == */
            swapElement(vertex, vertexVector_);
        }

        /**
         * @brief Add an edge to the graph.
         * @param edge Edge to add.
         */
        virtual inline void addEdge(E *edge) {
            edge->setIx(edgeVector_.size());
            edgeVector_.emplace_back(edge);
        }

        /**
         * @brief Remove an edge from the graph.
         * @remark If edge is nullptr, nothing happens.
         * @param edge Edge to remove.
         * @throw @refitem Spider::Exception if edge does not exist in the graph.
         */
        virtual inline void removeEdge(E *edge) {
            /* == Assert that edge is part of the edgeVector_ == */
            assertElement(edge, edgeVector_);
            /* == Reset edge source and sink == */
            edge->setSource(nullptr, SIZE_MAX, Expression());
            edge->setSink(nullptr, SIZE_MAX, Expression());
            /* == swap and destroy the element == */
            swapElement(edge, edgeVector_);
        }

        /**
         * @brief Move vertex ownership from this graph to another graph.
         * @remark If graph or vertex is nullptr, nothing happen.
         * @param elt    Vertex to move.
         * @param graph  Graph to move to.
         * @throws spider::Exception if failed to remove vertex from current graph or failed to add it to new graph.
         * Be aware that in the latter case the vertex has already been removed from the graph.
         */
        virtual inline void moveVertex(V *vertex, G *graph) {
            if (!graph || (graph == this) || !vertex) {
                return;
            }
            /* == Assert that elt is part of the vertexVector_ == */
            assertElement(vertex, vertexVector_);
            /* == Release the unique_ptr before swap to avoid destruction == */
            vertex = vertexVector_[vertex->ix()].release();
            swapElement(vertex, vertexVector_);
            /* == Add the edge to the other graph == */
            graph->addVertex(vertex);
        }

        /**
         * @brief Move edge ownership from this graph to another graph.
         * @remark If graph or edge is nullptr, nothing happen.
         * @warning This method simply moves ownership of the Edge, no check on src / snk are performed.
         * @param elt    Edge to move.
         * @param graph  Graph to move to.
         * @throws spider::Exception if failed to remove edge from current graph or failed to add it to new graph.
         * Be aware that in the latter case the edge has already been removed from the graph.
         */
        virtual inline void moveEdge(E *edge, G *graph) {
            if (!graph || (graph == this) || !edge) {
                return;
            }
            /* == Assert that elt is part of the edgeVector_ == */
            assertElement(edge, edgeVector_);
            /* == Release the unique_ptr before swap to avoid destruction == */
            edge = edgeVector_[edge->ix()].release();
            swapElement(edge, edgeVector_);
            /* == Add the edge to the other graph == */
            graph->addEdge(edge);
        }

        /* === Getter(s) === */

        /**
         * @brief A const reference on the vector of vertex. Useful for iterating on the vertices.
         * @return const reference to vertex vector
         */
        inline const vector<unique_ptr<V>> &vertices() const {
            return vertexVector_;
        }

        /**
         * @brief Return the vertex corresponding to the ix.
         * @warning This method does not check for out of bound error.
         * @param ix  Ix of the vertex.
         * @return @refitem Vertex pointer
         */
        inline V *vertex(size_t ix) const {
            return vertexVector_[ix].get();
        }

        /**
         * @brief Get the number of vertices in the graph.
         * @remark This method exclude the number of interfaces and the number of config actors.
         * @return Total number of vertices.
         */
        inline size_t vertexCount() const {
            return vertexVector_.size();
        }

        /**
         * @brief A const reference on the set of edges. Useful for iterating on the edges.
         * @return const reference to edge vector
         */
        inline const vector<unique_ptr<E>> &edges() const {
            return edgeVector_;
        }

        /**
         * @brief Get the number of edges contained in the graph.
         * @return Number of edges.
         */
        inline size_t edgeCount() const {
            return edgeVector_.size();
        }

        /* === Setter(s) === */

    protected:
        vector<unique_ptr<V>> vertexVector_; /* = Vector of all the Vertices of the graph = */
        vector<unique_ptr<E>> edgeVector_;   /* = Vector of Edge contained in the graph = */

        /* === Protected method(s) === */

        template<class T>
        inline void assertElement(T *elt, vector<unique_ptr<T>> &eltVector) {
            auto ix = elt->ix();
            if (ix >= eltVector.size()) {
                throwSpiderException("Trying to remove an element not from this graph.");
            } else if (eltVector[ix].get() != elt) {
                throwSpiderException("Different element in ix position. Expected: %s -- Got: %s", elt->name().c_str(),
                                     eltVector[ix]->name().c_str());
            }
        }

        template<class T, class U>
        inline void swapElement(T *elt, vector<U> &eltVector) {
            auto ix = elt->ix();
            if (eltVector.back()) {
                eltVector.back()->setIx(ix);
            }
            out_of_order_erase(eltVector, ix);
        }
    };
}

#endif //SPIDER2_ABSTRACTGRAPH_H
