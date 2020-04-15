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
#ifndef SPIDER2_GRAPH_H
#define SPIDER2_GRAPH_H

/* === Include(s) === */

#include <string>
#include <cstdint>
#include <memory/unique_ptr.h>
#include <common/Exception.h>
#include <graphs/abstract/AbstractGraph.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/Interface.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Param.h>

namespace spider {

    namespace pisdf {

        /* === Class definition === */

        class Graph : public AbstractGraph<pisdf::Graph, pisdf::Vertex, pisdf::Edge>, public Vertex {
        public:

            explicit Graph(std::string name = "unnamed-graph",
                           size_t vertexCount = 0,
                           size_t edgeCount = 0,
                           size_t paramCount = 0,
                           size_t numberOfInputEdge = 0,
                           size_t numberOfOutputEdge = 0,
                           size_t cfgVertexCount = 0);

            Graph &operator=(Graph &&) = default;

            Graph(Graph &&) = default;

            ~Graph() noexcept override = default;

            /* === Disabling copy construction / assignment === */

            Graph(const Graph &) = delete;

            Graph &operator=(const Graph &) = delete;

            /* === Method(s) === */

            /**
             * @brief Clears a graph without destroying it.
             */
            void clear() override;

            /**
             * @brief Add a vertex to the graph.
             * @param vertex Vertex to add.
             */
            void addVertex(Vertex *vertex) override;

            /**
             * @brief Remove a vertex from the graph.
             * @remark If vertex is nullptr, nothing happens.
             * @param vertex Vertex to remove.
             * @throw @refitem Spider::Exception if vertex does not exist in the graph.
             */
            void removeVertex(Vertex *vertex) override;

            /**
             * @brief Move vertex ownership from this graph to another graph.
             * @remark If graph or vertex is nullptr, nothing happen.
             * @param elt    Vertex to move.
             * @param graph  Graph to move to.
             * @throws spider::Exception if failed to remove vertex from current graph or failed to add it to new graph.
             * Be aware that in the latter case the vertex has already been removed from the graph.
             */
            void moveVertex(Vertex *vertex, Graph *graph) override;

            /**
             * @brief Override automatic property of Graph.
             * @param value Value to set.
             */
            void overrideDynamicProperty(bool value);

            /**
             * @brief Adds an input interface to the graph.
             * @remark This will increase input edge vector size.
             * @remark If interface is nullptr, nothing happens.
             * @param interface Pointer to the interface to add.
             */
            void addInputInterface(Interface *interface);

            /**
             * @brief Adds an output interface to the graph.
             * @remark This will increase output edge vector size.
             * @remark If interface is nullptr, nothing happens.
             * @param interface Pointer to the interface to add.
             */
            void addOutputInterface(Interface *interface);

            /**
             * @brief Add an param to the graph.
             * @param param Param to add.
             */
            void addParam(std::shared_ptr<Param> param);

            /**
             * @brief Remove a parameter in the graph and update dynamic property accordingly.
             * @param param Param to be removed.
             */
            void removeParam(const std::shared_ptr<Param> &param);

            /**
             * @brief Search for a parameter from its name.
             * @param name Name of the parameter to find.
             * @return pointer to the found Param, nullptr else.
             */
            Param *paramFromName(const std::string &name);

            /**
             * @brief Checks if a graph is the top-level graph.
             * @return true if no upper graph exists, false else.
             */
            inline bool isTopGraph() const {
                return graph() == nullptr;
            }

            inline void visit(Visitor *visitor) override {
                visitor->visit(this);
            }

            /* === Getter(s) === */

            /**
             * @brief Return dynamic property of the graph.
             * @return true if graph is dynamic, false else.
             */
            inline bool dynamic() const {
                return dynamic_;
            }

            /**
             * @brief Get the number of config actors in the graph.
             * @return Total number of config actors.
             */
            inline size_t configVertexCount() const {
                return configVertexVector_.size();
            }

            /**
             * @brief Get the total number of actors of a graph, including actors in subgraphs.
             * @return total number of actors in the hierarchy.
             */
            size_t totalActorCount() const;

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
            * @brief A const reference on the set of subgraphs. Useful for iterating on the subgraphs.
            * @return const reference to subgraph vector
            */
            inline const vector<Graph *> &subgraphs() const {
                return subgraphVector_;
            }

            /**
            * @brief A const reference on the set of vertices. Useful for iterating on the vertices.
            * @return const reference to vertex vector
            */
            inline const vector<Vertex *> &configVertices() const {
                return configVertexVector_;
            }

            /**
            * @brief A const reference on the set of output interfaces. Useful for iterating on the input interfaces.
            * @return const reference to input interface vector
            */
            inline const vector<unique_ptr<Interface>> &inputInterfaceVector() const {
                return inputInterfaceVector_;
            }

            /**
            * @brief A const reference on the set of output interfaces. Useful for iterating on the output interfaces.
            * @return const reference to output interface vector
            */
            inline const vector<unique_ptr<Interface>> &outputInterfaceVector() const {
                return outputInterfaceVector_;
            }

            /**
            * @brief A const reference on the set of params. Useful for iterating on the params.
            * @return const reference to param vector
            */
            inline const vector<std::shared_ptr<Param>> &params() const {
                return paramVector_;
            }

            /**
             * @brief Return the parameter corresponding to the ix.
             * @warning This method does not check for out of bound error.
             * @param ix Ix of the param.
             * @return @refitem Param pointer
             */
            inline Param *param(size_t ix) const {
                return paramVector_[ix].get();
            }

            /**
             * @brief Return the input interface corresponding to the port ix.
             * @warning There is no consistency assured between input edges and input interfaces.
             * It is up to the user to ensure this property.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the port
             * @return @refitem InputInterface pointer
             */
            inline Interface *inputInterface(size_t ix) const {
                return inputInterfaceVector_[ix].get();
            }

            /**
             * @brief Return the output interface corresponding to the port ix.
             * @warning There is no consistency assured between output edges and output interfaces.
             * It is up to the user to ensure this property.
             * @warning This method does not check for out of bound error.
             * @param ix  Ix of the port
             * @return @refitem OutputInterface pointer
             */
            inline Interface *outputInterface(size_t ix) const {
                return outputInterfaceVector_[ix].get();
            }

            /**
             * @brief Get the ix of the graph inside its containing graph subgraphVector_.
             * @return subgraph ix of the graph.
             */
            inline size_t subIx() const {
                return subIx_;
            }

            /* === Setter(s) === */

        private:
            vector<Vertex *> configVertexVector_;                   /* = Vector of Vertices with VertexType::CONFIG. This is just a "viewer" vector. = */
            vector<Graph *> subgraphVector_;                        /* = Vector of Vertices with VertexType::GRAPH.  This is just a "viewer" vector. = */
            vector<std::shared_ptr<Param>> paramVector_;            /* = Vector of Param = */
            vector<unique_ptr<Interface>> inputInterfaceVector_;    /* = Vector of InputInterface = */
            vector<unique_ptr<Interface>> outputInterfaceVector_;   /* = Vector of OutputInterface = */
            size_t subIx_ = SIZE_MAX;  /* = Index of the Graph in containing Graph subgraphVector = */
            bool dynamic_ = false;     /* = Dynamic property of the Graph (false if static, true if dynamic) = */

            /* === Private structure(s) === */

            struct RemoveSubgraphVisitor;

            struct AddSubgraphVisitor;

        };
    }
}
#endif //SPIDER2_GRAPH_H
