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
#include <containers/containers.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Param.h>

namespace spider {

    namespace pisdf {

        /* === Forward declaration(s) === */

        struct GraphAddVertexVisitor;

        struct GraphRemoveVertexVisitor;

        /* === Class definition === */

        class Graph final : public Vertex {
        public:

            explicit Graph(std::string name = "unnamed-graph",
                           uint32_t vertexCount = 0,
                           uint32_t edgeCount = 0,
                           uint32_t paramCount = 0,
                           uint32_t edgeINCount = 0,
                           uint32_t edgeOUTCount = 0,
                           uint32_t cfgVertexCount = 0,
                           StackID stack = StackID::PISDF);

            Graph(const Graph &other, StackID stack = StackID::PISDF) : Vertex(other, stack) {
                dynamic_ = other.dynamic_;
                vertexVector_ = containers::vector(other.vertexVector_, stack);
                configVertexVector_ = containers::vector(other.configVertexVector_, stack);
                subgraphVector_ = containers::vector(other.subgraphVector_, stack);
                edgeVector_ = containers::vector(other.edgeVector_, stack);
                paramVector_ = containers::vector(other.paramVector_, stack);
            };

            Graph(Graph &&other) noexcept : Vertex(std::move(other)) {
                std::swap(dynamic_, other.dynamic_);
                std::swap(subIx_, other.subIx_);
                std::swap(vertexVector_, other.vertexVector_);
                std::swap(configVertexVector_, other.configVertexVector_);
                std::swap(subgraphVector_, other.subgraphVector_);
                std::swap(edgeVector_, other.edgeVector_);
                std::swap(paramVector_, other.paramVector_);
                std::swap(inputInterfaceArray_, other.inputInterfaceArray_);
                std::swap(outputInterfaceArray_, other.outputInterfaceArray_);
            };

            ~Graph() noexcept override;

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
             * @remark If graph or vertex is nullptr, nothing happen.
             * @param elt    Vertex to move.
             * @param graph  Graph to move to.
             * @throws spider::Exception if failed to remove vertex from current graph or failed to add it to new graph.
             * Be aware that in the latter case the vertex has already been removed from the graph.
             */
            inline void moveVertex(Vertex *elt, Graph *graph) {
                if (!graph || !elt) {
                    return;
                }
                removeElement(vertexVector_, elt);
                graph->addVertex(elt);
            }

            /**
             * @brief Move param ownership from this graph to another graph.
             * @remark If graph or param is nullptr, nothing happen.
             * @param elt    Param to move.
             * @param graph  Graph to move to.
             * @throws spider::Exception if failed to remove param from current graph or failed to add it to new graph.
             * Be aware that in the latter case the param has already been removed from the graph.
             */
            inline void moveParam(Param *elt, Graph *graph) {
                if (!graph || !elt) {
                    return;
                }
                removeElement(paramVector_, elt);
                graph->addParam(elt);
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
            inline void moveEdge(Edge *elt, Graph *graph) {
                if (!graph || !elt) {
                    return;
                }
                removeElement(edgeVector_, elt);
                graph->addEdge(elt);
            }

            inline void visit(Visitor *visitor) override {
                visitor->visit(this);
            }

            /* === Getter(s) === */

            inline VertexType subtype() const override {
                return VertexType::GRAPH;
            }

            inline bool hierarchical() const override {
                return true;
            }

            /**
             * @brief Return dynamic property of the graph.
             * @return true if graph is dynamic, false else.
             */
            inline bool dynamic() const {
                return dynamic_;
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
             * @brief Get the number of config actors in the graph.
             * @return Total number of config actors.
             */
            inline size_t configVertexCount() const {
                return configVertexVector_.size();
            }

            /**
             * @brief Get the number of edges contained in the graph.
             * @return Number of edges.
             */
            inline size_t edgeCount() const {
                return edgeVector_.size();
            }

            /**
             * @brief Get the number of params contained in the graph.
             * @return Number of params.
             */
            inline size_t paramCount() const {
                return paramVector_.size();
            }

            /**
             * @brief Get the number of subgraphs.
             * @return Number of subgraphs.
             */
            inline size_t subgraphCount() const {
                return subgraphVector_.size();
            }

            /**
            * @brief A const reference on the set of vertices. Useful for iterating on the vertices.
            * @return const reference to exec vertex vector
            */
            inline const spider::vector<Vertex *> &vertices() const {
                return vertexVector_;
            }

            /**
            * @brief A const reference on the set of subgraphs. Useful for iterating on the subgraphs.
            * @return const reference to subgraph vector
            */
            inline const spider::vector<Graph *> &subgraphs() const {
                return subgraphVector_;
            }

            /**
            * @brief A const reference on the set of vertices. Useful for iterating on the vertices.
            * @return const reference to vertex vector
            */
            inline const spider::vector<ConfigVertex *> &configVertices() const {
                return configVertexVector_;
            }

            /**
            * @brief A const reference on the set of output interfaces. Useful for iterating on the input interfaces.
            * @return const reference to input interface array
            */
            inline const spider::array<InputInterface *> &inputInterfaceArray() const {
                return inputInterfaceArray_;
            }

            /**
            * @brief A const reference on the set of output interfaces. Useful for iterating on the output interfaces.
            * @return const reference to output interface array
            */
            inline const spider::array<OutputInterface *> &outputInterfaceArray() const {
                return outputInterfaceArray_;
            }

            /**
            * @brief A const reference on the set of edges. Useful for iterating on the edges.
            * @return const reference to edge vector
            */
            inline const spider::vector<Edge *> &edges() const {
                return edgeVector_;
            }

            /**
            * @brief A const reference on the set of params. Useful for iterating on the params.
            * @return const reference to param vector
            */
            inline const spider::vector<Param *> &params() const {
                return paramVector_;
            }

            /**
             * @brief Return the parameter corresponding to the ix.
             * @warning This method does not check for out of bound error.
             * @param ix Ix of the param.
             * @return @refitem Param pointer
             */
            inline Param *param(size_t ix) const {
                return paramVector_[ix];
            }

            /**
             * @brief Return the vertex corresponding to the ix.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the vertex.
             * @return @refitem Vertex pointer
             */
            inline Vertex *vertex(size_t ix) const {
                return vertexVector_[ix];
            }

            /**
             * @brief Return the input interface corresponding to the port ix.
             * @warning There is no consistency assured between input edges and input interfaces.
             * It is up to the user to ensure this property.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the port
             * @return @refitem InputInterface pointer
             */
            inline InputInterface *inputInterface(size_t ix) const {
                return inputInterfaceArray_[ix];
            }

            /**
             * @brief Return the output interface corresponding to the port ix.
             * @warning There is no consistency assured between output edges and output interfaces.
             * It is up to the user to ensure this property.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the port
             * @return @refitem OutputInterface pointer
             */
            inline OutputInterface *outputInterface(size_t ix) const {
                return outputInterfaceArray_[ix];
            }

            /**
             * @brief Get the ix of the graph inside its containing graph subgraphVector_.
             * @return subgraph ix of the graph.
             */
            inline uint32_t subIx() const {
                return subIx_;
            }

            /* === Setter(s) === */

        private:
            bool dynamic_ = false;
            uint32_t subIx_ = UINT32_MAX;

            /* === Contained elements of the graph === */

            spider::vector<Vertex *> vertexVector_;
            spider::vector<ConfigVertex *> configVertexVector_;
            spider::vector<Graph *> subgraphVector_;
            spider::vector<Edge *> edgeVector_;
            spider::vector<Param *> paramVector_;
            spider::array<InputInterface *> inputInterfaceArray_;
            spider::array<OutputInterface *> outputInterfaceArray_;

            /* === Private method(s) === */

            template<class T>
            void removeElement(spider::vector<T *> &eltVector, T *elt);
        };
    }
}
#endif //SPIDER2_GRAPH_H
