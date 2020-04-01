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
#ifndef SPIDER2_ABSTRACTVERTEX_H
#define SPIDER2_ABSTRACTVERTEX_H

/* === Include(s) === */

#include <string>
#include <cstddef>
#include <containers/vector.h>
#include <graphs-tools/expression-parser/Expression.h>

namespace spider {

    /* === Class definition === */

    template<class G, class E>
    class AbstractVertex {
    public:
        template<StackID stack>
        explicit AbstractVertex(stack_t <stack>, std::string name, size_t edgeINCount = 0, size_t edgeOUTCount = 0) :
                name_{ std::move(name) },
                inputEdgeVector_{ factory::vector<E *>(edgeINCount, nullptr, stack) },
                outputEdgeVector_{ factory::vector<E *>(edgeOUTCount, nullptr, stack) } { }

        AbstractVertex(AbstractVertex &&) noexcept = default;

        AbstractVertex &operator=(AbstractVertex &&) = default;

        virtual ~AbstractVertex() noexcept = default;

        /* === Method(s) === */

        /**
         * @brief Connect an input edge at given position.
         * @param edge  Pointer to the edge to connect.
         * @param pos   Input position where to connect the edge.
         * @throw @refitem std::out_of_range.
         * @throw @refitem spider::Exception if an edge already exists at this position.
         */
        virtual inline void connectInputEdge(E *edge, size_t pos) {
            connectEdge(inputEdgeVector_, edge, pos);
        }

        /**
         * @brief Connect an output edge at given position.
         * @param edge  Pointer to the edge to connect.
         * @param pos   Output position where to connect the edge.
         * @throw @refitem std::out_of_range.
         * @throw @refitem spider::Exception if an edge already exists at this position.
         */
        virtual inline void connectOutputEdge(E *edge, size_t pos) {
            connectEdge(outputEdgeVector_, edge, pos);
        }

        /**
         * @brief Disconnect input edge on port ix. If no edge is connected, nothing happens.
         * @remark Call @refitem Edge::setSink to reset the edge if found.
         * @param ix  Index of the input edge to disconnect.
         */
        virtual inline E *disconnectInputEdge(size_t ix) {
            auto *edge = disconnectEdge(inputEdgeVector_, ix);
            if (edge) {
                /* == Reset the Edge == */
                edge->setSink(nullptr, SIZE_MAX, Expression());
            }
            return edge;
        }

        /**
         * @brief Disconnect output edge on port ix. If no edge is connected, nothing happens
         * @remark Call @refitem Edge::setSource to reset the edge if found.
         * @param ix  Index of the output edge to disconnect.
         */
        virtual inline E *disconnectOutputEdge(size_t ix) {
            auto *edge = disconnectEdge(outputEdgeVector_, ix);
            if (edge) {
                /* == Reset the Edge == */
                edge->setSource(nullptr, SIZE_MAX, Expression());
            }
            return edge;
        }

        /* === Getter(s) === */

        /**
         * @brief Get the name string of the vertex.
         * @return name of the vertex.
         */
        inline const std::string &name() const { return name_; };

        /**
         * @brief Get the ix of the vertex in the containing graph.
         * @return ix of the vertex (SIZE_MAX if no ix).
         */
        inline const size_t &ix() const { return ix_; };

        /**
         * @brief Returns the graph of the vertex (if any)
         * @return Pointer to the containing graph, nullptr else.
         */
        inline G *graph() const { return graph_; }

        /**
         * @brief A const reference on the array of input edges. Useful for iterating on the edges.
         * @return const reference to input edge array
         */
        inline const vector<E *> &inputEdgeVector() const { return inputEdgeVector_; };

        /**
         * @brief Get input edge connected to port Ix.
         * @param ix Index of the input port.
         * @return @refitem spider::pisdf::Edge
         * @throw std::out_of_range.
         */
        inline E *inputEdge(size_t ix) const { return inputEdgeVector_.at(ix); };

        /**
         * @brief Get the number of input edges connected to the vertex.
         * @return number of input edges.
         */
        inline size_t inputEdgeCount() const { return inputEdgeVector_.size(); };

        /**
         * @brief A const reference on the array of output edges. Useful for iterating on the edges.
         * @return const reference to output edge array.
         */
        inline const vector<E *> &outputEdgeVector() const { return outputEdgeVector_; };

        /**
         * @brief Get input edge connected to port Ix.
         * @param ix Index of the output port.
         * @return @refitem spider::pisdf::Edge
         * @throw std::out_of_range.
         */
        inline E *outputEdge(size_t ix) const { return outputEdgeVector_.at(ix); };

        /**
         * @brief Get the number of output edges connected to the vertex.
         * @return number of output edges.
         */
        inline size_t outputEdgeCount() const { return outputEdgeVector_.size(); };

        /* === Setter(s) === */

        /**
         * @brief Set the name of the vertex.
         * @remark This method will replace current name of the vertex.
         * @warning No check on the name is performed to see if it is unique in the graph.
         * @param name  Name to set.
         */
        virtual inline void setName(std::string name) { name_ = std::move(name); };

        /**
         * @brief Set the ix of the vertex in the containing graph.
         * @param ix Ix to set.
         */
        virtual inline void setIx(size_t ix) { ix_ = ix; };

        /**
         * @brief Set the containing graph of the vertex.
         * @remark override current value.
         * @remark if graph is nullptr, nothing happens.
         * @param graph  Pointer to the graph to set.
         */
        virtual inline void setGraph(G *graph) {
            if (graph) {
                graph_ = graph;
            }
        }

    protected:
        std::string name_ = "unnamed-vertex";  /* = Name of the Vertex (uniqueness is not required) = */
        vector<E *> inputEdgeVector_;          /* = Vector of input Edge = */
        vector<E *> outputEdgeVector_;         /* = Vector of output Edge = */
        G *graph_ = nullptr;                  /* = Graph of the vertex = */
        size_t ix_ = SIZE_MAX;                 /* = Index of the Vertex in the containing Graph = */

        static inline E *disconnectEdge(vector<E *> &edges, size_t ix) {
            auto *&edge = edges.at(ix);
            auto *ret = edge;
            if (edge) {
                edge = nullptr;
            }
            return ret;
        }

        static void connectEdge(vector<E *> &edges, E *edge, size_t ix) {
            auto *&current = edges.at(ix);
            if (!current) {
                current = edge;
                return;
            }
            throwSpiderException("Edge already exists at position: %zu", ix);
        }
    };
}
#endif //SPIDER2_ABSTRACTVERTEX_H
