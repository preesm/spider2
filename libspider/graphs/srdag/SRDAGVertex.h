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
#ifndef SPIDER2_SRDAGVERTEX_H
#define SPIDER2_SRDAGVERTEX_H

#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <containers/vector.h>
#include <runtime/common/RTInfo.h>
#include <scheduling/task/SRDAGTask.h>

namespace spider {

    namespace pisdf {
        class Vertex;

        class Param;
    }

    namespace srdag {

        class Edge;

        class Graph;

        /* === Class definition === */

        class Vertex {
        public:
            explicit Vertex(const pisdf::Vertex *reference,
                            size_t instanceValue,
                            size_t edgeINCount = 0,
                            size_t edgeOUTCount = 0);

            Vertex(Vertex &&) noexcept = default;

            Vertex &operator=(Vertex &&) = default;

            Vertex(const Vertex &) = delete;

            Vertex &operator=(const Vertex &) = delete;

            ~Vertex();

            /* === Method(s) === */

            /**
            * @brief Connect an input edge at given position.
            * @param edge  Pointer to the edge to connect.
            * @param pos   Input position where to connect the edge.
            * @throw @refitem std::out_of_range.
            * @throw @refitem spider::Exception if an edge already exists at this position.
            */
            void connectInputEdge(srdag::Edge *edge, size_t pos);

            /**
             * @brief Connect an output edge at given position.
             * @param edge  Pointer to the edge to connect.
             * @param pos   Output position where to connect the edge.
             * @throw @refitem std::out_of_range.
             * @throw @refitem spider::Exception if an edge already exists at this position.
             */
            void connectOutputEdge(srdag::Edge *edge, size_t pos);

            /**
             * @brief Disconnect input edge on port ix. If no edge is connected, nothing happens.
             * @remark Call @refitem Edge::setSink to reset the edge if found.
             * @param ix  Index of the input edge to disconnect.
             */
            srdag::Edge *disconnectInputEdge(size_t ix);

            /**
             * @brief Disconnect output edge on port ix. If no edge is connected, nothing happens
             * @remark Call @refitem Edge::setSource to reset the edge if found.
             * @param ix  Index of the output edge to disconnect.
             */
            srdag::Edge *disconnectOutputEdge(size_t ix);

            /**
             * @brief Add an input parameter to the Vertex.
             * @param param  Pointer to the parameter to add.
             */
            void addInputParameter(std::shared_ptr<pisdf::Param> param);

            /**
             * @brief Add an input parameter for the refinement of the Vertex.
             * @warning a separate call to addInputParameter is needed.
             * @param param  Pointer to the parameter to add.
             */
            void addRefinementParameter(std::shared_ptr<pisdf::Param> param);

            /**
             * @brief Add an output parameter to the Vertex.
             * @param param  Pointer to the parameter to add.
             * @throw spider::Exception if subtype() is not @refitem VertexType::CONFIG.
             */
            void addOutputParameter(std::shared_ptr<pisdf::Param> param);

            /**
             * @brief Get the complete path of the Vertex.
             * @example: vertex name = "vertex_0", graph name = "top_graph"
             *           -> vertex path = "top_graph-vertex_0"
             * @return complete vertex path
             */
            std::string vertexPath() const;

            /* === Getter(s) === */

            /**
             * @brief Get the name string of the vertex.
             * @return name of the vertex.
             */
            std::string name() const;

            /**
             * @brief Get the ix of the vertex in the containing graph.
             * @return ix of the vertex (SIZE_MAX if no ix).
             */
            inline size_t ix() const { return ix_; };

            /**
             * @brief Returns the graph of the vertex (if any)
             * @return Pointer to the containing graph, nullptr else.
             */
            inline srdag::Graph *graph() { return const_cast<srdag::Graph *>(graph_); }

            /**
             * @brief Returns the graph of the vertex (if any)
             * @return Pointer to the containing graph, nullptr else.
             */
            inline const srdag::Graph *graph() const { return graph_; }

            /**
             * @brief A const reference on the array of input edges. Useful for iterating on the edges.
             * @return const reference to input edge array
             */
            inline spider::array_view<srdag::Edge *> inputEdges() const {
                return spider::make_view(inputEdgeArray_, nINEdges_);
            };

            /**
             * @brief Get input edge connected to port Ix.
             * @param ix Index of the input port.
             * @return @refitem spider::pisdf::Edge
             * @throw std::out_of_range.
             */
            inline srdag::Edge *inputEdge(size_t ix) const {
#ifndef NDEBUG
                if (ix >= nINEdges_) {
                    throwSpiderException("index out of bound");
                }
#endif
                return inputEdgeArray_[ix];
            };

            /**
             * @brief Get the number of input edges connected to the vertex.
             * @return number of input edges.
             */
            inline size_t inputEdgeCount() const { return nINEdges_; };

            /**
             * @brief A const reference on the array of output edges. Useful for iterating on the edges.
             * @return const reference to output edge array.
             */
            inline spider::array_view<srdag::Edge *> outputEdges() const {
                return spider::make_view(outputEdgeArray_, nOUTEdges_);
            };

            /**
             * @brief Get input edge connected to port Ix.
             * @param ix Index of the output port.
             * @return @refitem spider::pisdf::Edge
             * @throw std::out_of_range.
             */
            inline srdag::Edge *outputEdge(size_t ix) const {
#ifndef NDEBUG
                if (ix >= nOUTEdges_) {
                    throwSpiderException("index out of bound");
                }
#endif
                return outputEdgeArray_[ix];
            };

            /**
             * @brief Get the number of output edges connected to the vertex.
             * @return number of output edges.
             */
            inline size_t outputEdgeCount() const { return nOUTEdges_; };

            /**
             * @brief A const reference on the vector of refinement input params.
             * @return const reference to input params vector.
             */
            inline const spider::vector<std::shared_ptr<pisdf::Param>> &
            refinementParamVector() const { return refinementParamVector_; };

            /**
             * @brief A const reference on the vector of input params.
             * @return const reference to input params vector.
             */
            inline const spider::vector<std::shared_ptr<pisdf::Param>> &
            inputParamVector() const { return inputParamVector_; };

            /**
             * @brief Get the number of input params connected to the vertex.
             * @return number of input params.
             */
            inline size_t inputParamCount() const { return inputParamVector_.size(); };

            /**
             * @brief A const reference on the vector of output params.
             * @return const reference to output params vector.
             */
            inline const spider::vector<std::shared_ptr<pisdf::Param>> &
            outputParamVector() const { return outputParamVector_; };

            /**
             * @brief Get the number of output params connected to the vertex.
             * @return number of output params.
             */
            inline size_t outputParamCount() const { return outputParamVector_.size(); };

            /**
             * @brief Return the reference vertex attached to current copy.
             * @remark If vertex is not a copy, this return the vertex itself.
             * @warning There is a potential risk here. If the reference is freed before the copy,
             * there are no possibilities to know it.
             * @return pointer to @refitem Vertex reference.
             */
            inline const pisdf::Vertex *reference() const { return reference_; };

            /**
             * @brief Return the executable property of a vertex.
             * @return true if vertex is executable, false else.
             */
            inline bool executable() const { return executable_; };

            /**
             * @brief Returns the @refitem RTInfo structure associated with this vertex.
             * @remark If the vertex is non-executable, it should return nullptr.
             * @return pointer to the @refitem RTInfo of the vertex, nullptr if !(this->executable()).
             */
            RTInfo *runtimeInformation() const;

            /**
             * @brief Get the schedule job ix associated to this vertex.
             * @return ix of the job, SIZE_MAX if not set.
             */
            inline size_t scheduleTaskIx() const { return scheduleJobIx_; };

            inline sched::SRDAGTask *scheduleTask() const { return scheduleTask_.get(); };

            /**
             * @brief Get the instance value associated to this clone vertex (0 if original).
             * @return instance value of the vertex, 0 by default.
             */
            inline size_t instanceValue() const { return instanceValue_; };

            /**
             * @brief Get the subtype of the vertex.
             * @return @refitem spider::pisdf::VertexType corresponding to the subtype
             */
            pisdf::VertexType subtype() const;

            /* === Setter(s) === */

            /**
             * @brief Set the schedule job ix of the vertex.
             * @param ix  Ix to set.
             */
            inline void setScheduleTaskIx(size_t ix) { scheduleJobIx_ = ix; };

            /**
             * @brief Set the ix of the vertex in the containing graph.
             * @param ix Ix to set.
             */
            inline void setIx(size_t ix) { ix_ = ix; };

            inline void setExecutable(bool executable) { executable_ = executable; }

            /**
             * @brief Set the containing graph of the vertex.
             * @remark override current value.
             * @remark if graph is nullptr, nothing happens.
             * @param graph  Pointer to the graph to set.
             */
            inline void setGraph(srdag::Graph *graph) {
                if (graph) {
                    graph_ = graph;
                }
            }

        private:
            spider::vector<std::shared_ptr<pisdf::Param>> inputParamVector_;      /* = Vector of input Params = */
            spider::vector<std::shared_ptr<pisdf::Param>> refinementParamVector_; /* = Vector of refinement Params = */
            spider::vector<std::shared_ptr<pisdf::Param>> outputParamVector_;     /* = Vector of output Params = */
            spider::unique_ptr<sched::SRDAGTask> scheduleTask_;                   /* = Scheduled task associated with this vertex = */
            srdag::Edge **inputEdgeArray_ = nullptr;                              /* = Vector of input Edge = */
            srdag::Edge **outputEdgeArray_ = nullptr;                             /* = Vector of output Edge = */
            const pisdf::Vertex *reference_ = nullptr;                            /* = Pointer to the reference Vertex. = */
            const srdag::Graph *graph_ = nullptr;                                 /* = Graph of the vertex = */
            size_t ix_ = SIZE_MAX;                                                /* = Index of the Vertex in the containing Graph = */
            size_t scheduleJobIx_ = SIZE_MAX;  /* = Index of the schedule job associated to this Vertex. = */
            size_t instanceValue_ = 0;         /* = Value of the instance relative to reference Vertex = */
            u32 nINEdges_ = 0;
            u32 nOUTEdges_ = 0;
            bool executable_ = true;

            /**
             * @brief Disconnect an edge from the given edge vector (input or output).
             * @param edges  Vector of edges (input or output).
             * @param ix     Index of the edge to disconnect.
             * @return pointer to the disconnected edge, nullptr else.
             */
            static srdag::Edge *disconnectEdge(srdag::Edge **edges, size_t ix);

            /**
             * @brief Connect an edge in the given edge vector (input or output).
             * @param edges  Vector of edges (input or output).
             * @param edge   Pointer to the edge to be connected.
             * @param ix     Index of the edge to connect.
             * @throw spider::Exception if edge already exists in the vector at given index.
             */
            static void connectEdge(srdag::Edge **edges, Edge *edge, size_t ix);
        };

    }
}

#endif
#endif //SPIDER2_SRDAGVERTEX_H
