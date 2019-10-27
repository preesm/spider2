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
#ifndef SPIDER2_GRAPH_H
#define SPIDER2_GRAPH_H

/* === Include(s) === */

#include <string>
#include <cstdint>
#include <common/Exception.h>
#include <containers/StlContainers.h>
#include <graphs/tmp/Vertex.h>
#include <graphs/tmp/Edge.h>

namespace Spider {
    namespace PiSDF {

        /* === Forward declaration(s) === */
        class ExecVertex;
        class Interface;

        /* === Class definition === */

        class Graph final : public Vertex {
        public:

            explicit Graph(std::string name = "unnamed-graph",
                           std::uint32_t edgeINCount = 0,
                           std::uint32_t edgeOUTCount = 0,
                           Graph *graph = nullptr,
                           StackID stack = StackID::PISDF);

            ~Graph() override;

            /* === Method(s) === */

            /**
             * @brief Add a vertex to the graph.
             * @param vertex Vertex to add.
             */
            inline void addVertex(Vertex *vertex);

            /**
             * @brief Remove a vertex from the graph.
             * @remark If vertex is nullptr, nothing happens.
             * @param vertex Vertex to remove.
             * @throw @refitem Spider::Exception if vertex does not exist in the graph.
             */
            void removeVertex(ExecVertex *vertex);

            /**
             * @brief Add an edge to the graph.
             * @param edge Edge to add.
             */
            inline void addEdge(Edge *edge);

            /**
             * @brief Remove an edge from the graph.
             * @remark If edge is nullptr, nothing happens.
             * @param edge Edge to remove.
             * @throw @refitem Spider::Exception if edge does not exist in the graph.
             */
            void removeEdge(Edge *edge);

            /**
             * @brief Return the input interface corresponding to the port ix.
             * @warning There is no consistency assured between input edges and input interfaces.
             * It is up to the user to ensure this property.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the port
             * @return @refitem Interface
             */
            inline Interface *inputInterfaceFromIx(std::uint32_t ix) const;

            /**
             * @brief Return the output interface corresponding to the port ix.
             * @warning There is no consistency assured between output edges and output interfaces.
             * It is up to the user to ensure this property.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the port
             * @return @refitem Interface
             */
            inline Interface *outputInterfaceFromIx(std::uint32_t ix) const;

            /* === Getter(s) === */

            inline bool hierarchical() const override;

            /**
             * @brief Get the number of vertices in the graph.
             * @remark This method exclude the number of interfaces and the number of config actors.
             * @return Total number of vertices.
             */
            inline std::uint64_t vertexCount() const;

            /**
             * @brief Get the number of config actors in the graph.
             * @return Total number of config actors.
             */
            inline std::uint64_t configVertexCount() const;

            /**
             * @brief Get the number of edges contained in the graph.
             * @return Number of edges.
             */
            inline std::uint64_t edgeCount() const;

            /**
             * @brief Get the number of input interfaces.
             * @return Number of input interfaces.
             */
            inline std::uint64_t inputIFCount() const;

            /**
             * @brief Get the number of output interfaces.
             * @return Number of output interfaces.
             */
            inline std::uint64_t outputIFCount() const;

            /**
            * @brief A const reference on the set of vertices. Useful for iterating on the vertices.
            * @return const reference to vertex set
            */
            inline const Spider::vector<ExecVertex *> &vertices() const;

            /**
            * @brief A const reference on the set of vertices. Useful for iterating on the vertices.
            * @return const reference to vertex set
            */
            inline const Spider::vector<Vertex *> &configActors() const;

            /**
            * @brief A const reference on the set of output interfaces. Useful for iterating on the input interfaces.
            * @return const reference to input interface set
            */
            inline const Spider::Array<Interface *> &inputInterfaces() const;

            /**
            * @brief A const reference on the set of output interfaces. Useful for iterating on the output interfaces.
            * @return const reference to output interface set
            */
            inline const Spider::Array<Interface *> &outputInterfaces() const;

            /**
            * @brief A const reference on the set of edges. Useful for iterating on the edges.
            * @return const reference to edge set
            */
            inline const Spider::vector<Edge *> &edges() const;

            /* === Setter(s) === */

        private:

            /* === Contained elements of the graph === */

            Spider::vector<ExecVertex *> vertexVector_;
            Spider::vector<Vertex *> configVertexVector_;
            Spider::vector<Graph *> subgraphVector_;
            Spider::vector<Edge *> edgeVector_;
            Spider::Array<Interface *> inputInterfaceArray_;
            Spider::Array<Interface *> outputInterfaceArray_;

            /* === Private method(s) === */

            template<class T>
            void removeElement(Spider::vector<T *> &eltVector, T *elt);

            /**
             * @brief Add an interface to the graph.
             * @param interface Interface to add.
             */
            void addInterface(Interface *interface);

            /**
             * @brief Add a subgraph to the graph.
             * @param graph Subgraph to add.
             */
            void addSubGraph(Graph *graph);
        };

        /* === Inline method(s) === */

        void Graph::addEdge(Edge *edge) {
            edge->setIx(edgeVector_.size());
            edgeVector_.push_back(edge);
        }

        bool Graph::hierarchical() const {
            return true;
        }

        std::uint64_t Graph::vertexCount() const {
            return vertexVector_.size();
        }

        std::uint64_t Graph::configVertexCount() const {
            return configVertexVector_.size();
        }

        std::uint64_t Graph::edgeCount() const {
            return edgeVector_.size();
        }

        std::uint64_t Graph::inputIFCount() const {
            return inputInterfaceArray_.size();
        }

        std::uint64_t Graph::outputIFCount() const {
            return outputInterfaceArray_.size();
        }

        const Spider::vector<ExecVertex *> &Graph::vertices() const {
            return vertexVector_;
        }

        const Spider::vector<Vertex *> &Graph::configActors() const {
            return configVertexVector_;
        }

        const Spider::Array<Interface *> &Graph::inputInterfaces() const {
            return inputInterfaceArray_;
        }

        const Spider::Array<Interface *> &Graph::outputInterfaces() const {
            return outputInterfaceArray_;
        }

        const Spider::vector<Edge *> &Graph::edges() const {
            return edgeVector_;
        }

        Interface *Graph::inputInterfaceFromIx(std::uint32_t ix) const {
            return inputInterfaceArray_[ix];
        }

        Interface *Graph::outputInterfaceFromIx(std::uint32_t ix) const {
            return outputInterfaceArray_[ix];
        }
    }
}
#endif //SPIDER2_GRAPH_H
