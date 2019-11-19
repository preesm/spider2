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

namespace spider {

    /* === Forward declaration(s) === */

    class Scenario;

    namespace pisdf {

        /* === Forward declaration(s) === */

        struct GraphAddVertexVisitor;

        struct GraphRemoveVertexVisitor;

        /* === Class definition === */

        class Graph final : public Vertex {
        public:

            explicit Graph(std::string name = "unnamed-graph",
                           std::uint32_t vertexCount = 0,
                           std::uint32_t edgeCount = 0,
                           std::uint32_t paramCount = 0,
                           std::uint32_t edgeINCount = 0,
                           std::uint32_t edgeOUTCount = 0,
                           std::uint32_t cfgVertexCount = 0,
                           StackID stack = StackID::PISDF);

            ~Graph() override;

            friend GraphAddVertexVisitor;

            friend GraphRemoveVertexVisitor;

            friend CloneVertexVisitor;

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
             * @brief Move vertex ownership from this graph to another graph.
             * @remark If graph is nullptr, nothing happen.
             * @param elt    Vertex to move.
             * @param graph  Graph to move to.
             */
            inline void moveVertex(Vertex *elt, Graph *graph);

            /**
             * @brief Move param ownership from this graph to another graph.
             * @remark If graph is nullptr, nothing happen.
             * @param elt    Param to move.
             * @param graph  Graph to move to.
             */
            void moveParam(Param *elt, Graph *graph);

            /**
             * @brief Move edge ownership from this graph to another graph.
             * @remark If graph is nullptr, nothing happen.
             * @param elt    Edge to move.
             * @param graph  Graph to move to.
             */
            void moveEdge(Edge *elt, Graph *graph);

            Vertex *forwardEdge(const Edge *e) override;

            inline void visit(Visitor *visitor) override;

            /**
             * @brief Create a scenario for the given graph.
             * @param stack Stack on which to create the scenario.
             */
            void createScenario(StackID stack = StackID::PISDF);

            /* === Getter(s) === */

            inline VertexType subtype() const override;

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
            inline const spider::vector<Vertex *> &vertices() const;

            /**
            * @brief A const reference on the set of subgraphs. Useful for iterating on the subgraphs.
            * @return const reference to subgraph vector
            */
            inline const spider::vector<Graph *> &subgraphs() const;

            /**
            * @brief A const reference on the set of vertices. Useful for iterating on the vertices.
            * @return const reference to vertex vector
            */
            inline const spider::vector<ConfigVertex *> &configVertices() const;

            /**
            * @brief A const reference on the set of output interfaces. Useful for iterating on the input interfaces.
            * @return const reference to input interface array
            */
            inline const spider::Array<InputInterface *> &inputInterfaceArray() const;

            /**
            * @brief A const reference on the set of output interfaces. Useful for iterating on the output interfaces.
            * @return const reference to output interface array
            */
            inline const spider::Array<OutputInterface *> &outputInterfaceArray() const;

            /**
            * @brief A const reference on the set of edges. Useful for iterating on the edges.
            * @return const reference to edge vector
            */
            inline const spider::vector<Edge *> &edges() const;

            /**
            * @brief A const reference on the set of params. Useful for iterating on the params.
            * @return const reference to param vector
            */
            inline const spider::vector<Param *> &params() const;

            /**
             * @brief Retrieve a parameter from its name.
             * @param name Name of the parameter.
             * @return pointer to the @refitem Spider::PiSDF::Param if found, nullptr else.
             */
            Param *param(const std::string &name) const;

            /**
             * @brief Return the parameter corresponding to the ix.
             * @warning This method does not check for out of bound error.
             * @param ix Ix of the param.
             * @return @refitem Param pointer
             */
            inline Param *param(std::uint32_t ix) const;

            /**
             * @brief Return the vertex corresponding to the ix.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the vertex.
             * @return @refitem Vertex pointer
             */
            inline Vertex *vertex(std::uint32_t ix) const;

            /**
             * @brief Return the input interface corresponding to the port ix.
             * @warning There is no consistency assured between input edges and input interfaces.
             * It is up to the user to ensure this property.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the port
             * @return @refitem InputInterface pointer
             */
            inline InputInterface *inputInterface(std::uint32_t ix) const;

            /**
             * @brief Return the output interface corresponding to the port ix.
             * @warning There is no consistency assured between output edges and output interfaces.
             * It is up to the user to ensure this property.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the port
             * @return @refitem OutputInterface pointer
             */
            inline OutputInterface *outputInterface(std::uint32_t ix) const;

            /**
             * @brief Get the ix of the graph inside its containing graph subgraphVector_.
             * @return subgraph ix of the graph.
             */
            inline std::uint32_t subIx() const;

            /**
             * @brief Get the scenario of the graph.
             * @remark a Graph is not required to have a scenario and thus it may be nullptr.
             * @return scenario of the graph.
             */
            inline Scenario *scenario() const;

            /* === Setter(s) === */

        private:
            bool dynamic_ = false;
            std::uint32_t subIx_ = UINT32_MAX;

            /* === Contained elements of the graph === */

            spider::vector<Vertex *> vertexVector_;
            spider::vector<ConfigVertex *> configVertexVector_;
            spider::vector<Graph *> subgraphVector_;
            spider::vector<Edge *> edgeVector_;
            spider::Array<InputInterface *> inputInterfaceArray_;
            spider::Array<OutputInterface *> outputInterfaceArray_;
            spider::vector<Param *> paramVector_;

            /* === Scenario related to this graph === */

            Scenario *scenario_ = nullptr;

            /* === Private method(s) === */

            template<class T>
            void removeElement(spider::vector<T *> &eltVector, T *elt);
        };

        /* === Inline method(s) === */

        void Graph::moveVertex(Vertex *elt, Graph *graph) {
            if (graph) {
                removeElement(vertexVector_, elt);
                graph->addVertex(elt);
            }
        }

        void Graph::visit(Visitor *visitor) {
            visitor->visit(this);
        }

        VertexType Graph::subtype() const {
            return VertexType::GRAPH;
        }

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

        const spider::vector<Vertex *> &Graph::vertices() const {
            return vertexVector_;
        }

        const spider::vector<Graph *> &Graph::subgraphs() const {
            return subgraphVector_;
        }

        const spider::vector<ConfigVertex *> &Graph::configVertices() const {
            return configVertexVector_;
        }

        const spider::Array<InputInterface *> &Graph::inputInterfaceArray() const {
            return inputInterfaceArray_;
        }

        const spider::Array<OutputInterface *> &Graph::outputInterfaceArray() const {
            return outputInterfaceArray_;
        }

        const spider::vector<Edge *> &Graph::edges() const {
            return edgeVector_;
        }

        const spider::vector<Param *> &Graph::params() const {
            return paramVector_;
        }

        Param *Graph::param(std::uint32_t ix) const {
            return paramVector_[ix];
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

        std::uint32_t Graph::subIx() const {
            return subIx_;
        }

        Scenario *Graph::scenario() const {
            return scenario_;
        }
    }
}
#endif //SPIDER2_GRAPH_H
