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
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/Edge.h>

namespace Spider {
    namespace PiSDF {

        /* === Class definition === */

        class Graph final : virtual public Vertex {
        public:

            explicit Graph(std::string name = "unnamed-graph",
                           std::uint32_t vertexCount = 0,
                           std::uint32_t edgeCount = 0,
                           std::uint32_t paramCount = 0,
                           std::uint32_t edgeINCount = 0,
                           std::uint32_t edgeOUTCount = 0,
                           std::uint32_t cfgVertexCount = 0,
                           Graph *graph = nullptr,
                           StackID stack = StackID::PISDF);

            ~Graph() override;

            /* === Method(s) === */

            /**
             * @brief Add a vertex to the graph.
             * @param vertex Vertex to add.
             */
            void addVertex(Vertex *vertex);

            /**
             * @brief Remove a vertex from the graph.
             * @remark If vertex is nullptr, nothing happens.
             * @param vertex Vertex to remove.
             * @throw @refitem Spider::Exception if vertex does not exist in the graph.
             */
            void removeVertex(Vertex *vertex);

            /**
             * @brief Remove a subgraph from the graph.
             * @remark If subgraph is nullptr, nothing happens.
             * @remark This method also removes the graph from the vertexVector.
             * @param subgraph Subgraph to remove.
             * @throw @refitem Spider::Exception if subgraph does not exist in the graph.
             */
            void removeSubgraph(Graph *subgraph);

            /**
             * @brief Add an edge to the graph.
             * @param edge Edge to add.
             */
            void addEdge(Edge *edge);

            /**
             * @brief Remove an edge from the graph.
             * @remark If edge is nullptr, nothing happens.
             * @param edge Edge to remove.
             * @throw @refitem Spider::Exception if edge does not exist in the graph.
             */
            void removeEdge(Edge *edge);

            /**
             * @brief Add an param to the graph.
             * @param param Param to add.
             */
            void addParam(Param *param);

            /**
             * @brief Remove an param from the graph.
             * @remark If param is nullptr, nothing happens.
             * @param param Param to add.
             * @throw @refitem Spider::Exception if param does not exist in the graph.
             */
            void removeParam(Param *param);

            /**
             * @brief Retrieve a parameter from its name.
             * @param name Name of the parameter.
             * @return pointer to the @refitem Spider::PiSDF::Param if found, nullptr else.
             */
            Param *findParam(const std::string &name) const;

            /**
             * @brief Return the vertex corresponding to the ix.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the vertex.
             * @return @refitem Vertex
             */
            inline Vertex *vertex(std::uint32_t ix) const;

            /**
             * @brief Return the input interface corresponding to the port ix.
             * @warning There is no consistency assured between input edges and input interfaces.
             * It is up to the user to ensure this property.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the port
             * @return @refitem Interface
             */
            inline InputInterface *inputInterface(std::uint32_t ix) const;

            /**
             * @brief Return the output interface corresponding to the port ix.
             * @warning There is no consistency assured between output edges and output interfaces.
             * It is up to the user to ensure this property.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the port
             * @return @refitem Interface
             */
            inline OutputInterface *outputInterface(std::uint32_t ix) const;

            Vertex *forwardEdge(const Edge *e) override;

            Vertex *clone(StackID stack, Graph *graph) const override;

            /* === Getter(s) === */

            inline bool hierarchical() const override;

            /**
             * @brief Return dynamic property of the graph.
             * @return true if graph is dynamic, false else.
             */
            inline bool dynamic() const;

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
             * @brief Get the number of params contained in the graph.
             * @return Number of params.
             */
            inline std::uint64_t paramCount() const;

            /**
             * @brief Get the number of subgraphs.
             * @return Number of subgraphs.
             */
            inline std::uint64_t subgraphCount() const;

            /**
            * @brief A const reference on the set of vertices. Useful for iterating on the vertices.
            * @return const reference to exec vertex vector
            */
            inline const Spider::vector<Vertex *> &vertices() const;

            /**
            * @brief A const reference on the set of subgraphs. Useful for iterating on the subgraphs.
            * @return const reference to subgraph vector
            */
            inline const Spider::vector<Graph *> &subgraphs() const;

            /**
            * @brief A const reference on the set of vertices. Useful for iterating on the vertices.
            * @return const reference to vertex vector
            */
            inline const Spider::vector<Vertex *> &configActors() const;

            /**
            * @brief A const reference on the set of output interfaces. Useful for iterating on the input interfaces.
            * @return const reference to input interface array
            */
            inline const Spider::Array<InputInterface *> &inputInterfaces() const;

            /**
            * @brief A const reference on the set of output interfaces. Useful for iterating on the output interfaces.
            * @return const reference to output interface array
            */
            inline const Spider::Array<OutputInterface *> &outputInterfaces() const;

            /**
            * @brief A const reference on the set of edges. Useful for iterating on the edges.
            * @return const reference to edge vector
            */
            inline const Spider::vector<Edge *> &edges() const;

            /**
            * @brief A const reference on the set of params. Useful for iterating on the params.
            * @return const reference to param vector
            */
            inline const Spider::vector<Param *> &params() const;

            /* === Setter(s) === */

        private:
            bool dynamic_ = false;
            std::uint32_t subIx_ = UINT32_MAX;

            /* === Contained elements of the graph === */

            Spider::vector<Vertex *> vertexVector_;
            Spider::vector<Vertex *> configVertexVector_;
            Spider::vector<Graph *> subgraphVector_;
            Spider::vector<Edge *> edgeVector_;
            Spider::Array<InputInterface *> inputInterfaceArray_;
            Spider::Array<OutputInterface *> outputInterfaceArray_;
            Spider::vector<Param *> paramVector_;

            /* === Private method(s) === */

            template<class T>
            void removeElement(Spider::vector<T *> &eltVector, T *elt);

            /**
             * @brief Add a subgraph to the graph.
             * @param graph Subgraph to add.
             */
            void addSubGraph(Graph *graph);

            /**
             * @brief Destroy and deallocate a vertex with dynamic_cast to proper type (needed for proper deallocation).
             * @param vertex Vertex to destroy.
             */
            static void destroyVertex(Vertex *vertex);
        };

        /* === Inline method(s) === */

        bool Graph::hierarchical() const {
            return true;
        }

        bool Graph::dynamic() const {
            return dynamic_;
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

        std::uint64_t Graph::paramCount() const {
            return paramVector_.size();
        }

        std::uint64_t Graph::subgraphCount() const {
            return subgraphVector_.size();
        }

        const Spider::vector<Vertex *> &Graph::vertices() const {
            return vertexVector_;
        }

        const Spider::vector<Graph *> &Graph::subgraphs() const {
            return subgraphVector_;
        }

        const Spider::vector<Vertex *> &Graph::configActors() const {
            return configVertexVector_;
        }

        const Spider::Array<InputInterface *> &Graph::inputInterfaces() const {
            return inputInterfaceArray_;
        }

        const Spider::Array<OutputInterface *> &Graph::outputInterfaces() const {
            return outputInterfaceArray_;
        }

        const Spider::vector<Edge *> &Graph::edges() const {
            return edgeVector_;
        }

        const Spider::vector<Param *> &Graph::params() const {
            return paramVector_;
        }

        Vertex *Graph::vertex(std::uint32_t ix) const {
            return vertexVector_[ix];
        }

        InputInterface *Graph::inputInterface(std::uint32_t ix) const {
            return inputInterfaceArray_[ix];
        }

        OutputInterface *Graph::outputInterface(std::uint32_t ix) const {
            return outputInterfaceArray_[ix];
        }

    }
}
#endif //SPIDER2_GRAPH_H
