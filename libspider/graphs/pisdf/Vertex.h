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
#ifndef SPIDER2_VERTEX_H
#define SPIDER2_VERTEX_H

/* === Include(s) === */

#include <containers/array.h>
#include <memory/unique_ptr.h>
#include <runtime/common/RTInfo.h>
#include <graphs-tools/helper/visitors/PiSDFVisitor.h>

namespace spider {

    namespace pisdf {

        /* === Forward declaration(s) === */

        class Graph;

        class Edge;

        class Visitor;

        /* === Class definition === */

        class Vertex {
        public:

            explicit Vertex(VertexType type = VertexType::NORMAL,
                            std::string name = "unnamed-vertex",
                            size_t edgeINCount = 0,
                            size_t edgeOUTCount = 0);

            Vertex(Vertex &&) noexcept = default;

            Vertex &operator=(Vertex &&) = default;

            /* === Disabling copy construction / assignment === */

            Vertex(const Vertex &) = delete;

            Vertex &operator=(const Vertex &) = delete;

            virtual ~Vertex() noexcept = default;

            /* === Method(s) === */

            /**
             * @brief Connect an input edge at given position.
             * @param edge  Pointer to the edge to connect.
             * @param pos   Input position where to connect the edge.
             * @throw @refitem std::out_of_range.
             * @throw @refitem spider::Exception if an edge already exists at this position.
             */
            void connectInputEdge(Edge *edge, size_t pos);

            /**
             * @brief Connect an output edge at given position.
             * @param edge  Pointer to the edge to connect.
             * @param pos   Output position where to connect the edge.
             * @throw @refitem std::out_of_range.
             * @throw @refitem spider::Exception if an edge already exists at this position.
             */
            void connectOutputEdge(Edge *edge, size_t pos);

            /**
             * @brief Disconnect input edge on port ix. If no edge is connected, nothing happens.
             * @remark Call @refitem Edge::setSink to reset the edge if found.
             * @param ix  Index of the input edge to disconnect.
             */
            Edge *disconnectInputEdge(size_t ix);

            /**
             * @brief Disconnect output edge on port ix. If no edge is connected, nothing happens
             * @remark Call @refitem Edge::setSource to reset the edge if found.
             * @param ix  Index of the output edge to disconnect.
             */
            Edge *disconnectOutputEdge(size_t ix);

            /**
             * @brief Generic method that accept a visitor for class specific treatments.
             * @remark This implement a double-dispatch visitor pattern.
             * @param visitor  Pointer to the visitor to accept.
             */
            virtual void visit(Visitor *visitor);

            /**
             * @brief Add an input parameter to the Vertex.
             * @param param  Pointer to the parameter to add.
             */
            void addInputParameter(const std::shared_ptr<Param> &param);

            /**
             * @brief Add an input parameter for the refinement of the Vertex.
             * @warning a separate call to addInputParameter is needed.
             * @param param  Pointer to the parameter to add.
             */
            void addRefinementParameter(const std::shared_ptr<Param> &param);

            /**
             * @brief Add an output parameter to the Vertex.
             * @param param  Pointer to the parameter to add.
             * @throw spider::Exception if subtype() is not @refitem VertexType::CONFIG.
             */
            void addOutputParameter(const std::shared_ptr<Param> &param);

            /**
             * @brief Get the complete path of the Vertex.
             * @example: vertex name = "vertex_0", graph name = "top_graph"
             *           -> vertex path = "top_graph-vertex_0"
             * @return complete vertex path
             */
            std::string vertexPath() const;

            /**
             * @brief Convert a vertex to the desired type (if possible) using static_cast.
             * @warning This a static_cast so it is your responsibility to ensure the type compatibility.
             * @tparam T Type to convert to.
             * @return pointer to the converted type.
             */
            template<class T>
            inline T *convertTo() { return static_cast<T *>(this); }

            /**
             * @brief Convert a vertex to the desired type (if possible) using static_cast.
             * @warning This a static_cast so it is your responsibility to ensure the type compatibility.
             * @tparam T Type to convert to.
             * @return pointer to the converted type.
             */
            template<class T>
            inline const T *convertTo() const { return static_cast<const T *>(this); }

            /* === Getter(s) === */

            /**
             * @brief Get the name string of the vertex.
             * @return name of the vertex.
             */
            inline std::string name() const { return name_.get(); };

            /**
             * @brief Get the ix of the vertex in the containing graph.
             * @return ix of the vertex (SIZE_MAX if no ix).
             */
            inline size_t ix() const { return ix_; };

            /**
             * @brief Returns the graph of the vertex (if any)
             * @return Pointer to the containing graph, nullptr else.
             */
            inline Graph *graph() const { return graph_; }

            /**
             * @brief A const reference on the array of input edges. Useful for iterating on the edges.
             * @return const reference to input edge array
             */
            inline spider::array_view<Edge *> inputEdges() const {
                return spider::make_view(inputEdgeArray_.get(), nINEdges_);
            };

            /**
             * @brief Get input edge connected to port Ix.
             * @param ix Index of the input port.
             * @return @refitem spider::pisdf::Edge
             * @throw std::out_of_range.
             */
            inline Edge *inputEdge(size_t ix) const {
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
            inline spider::array_view<Edge *> outputEdges() const {
                return spider::make_view(outputEdgeArray_.get(), nOUTEdges_);
            };

            /**
             * @brief Get input edge connected to port Ix.
             * @param ix Index of the output port.
             * @return @refitem spider::pisdf::Edge
             * @throw std::out_of_range.
             */
            inline Edge *outputEdge(size_t ix) const {
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
             * @brief Get the subtype of the vertex.
             * @return @refitem spider::pisdf::VertexType corresponding to the subtype
             */
            inline VertexType subtype() const { return subtype_; };

            /**
             * @brief Test if the vertex is a graph.
             * @return true if vertex is a graph, false else.
             */
            inline bool hierarchical() const { return subtype_ == VertexType::GRAPH; };

            /**
             * @brief Get the repetition vector value of the vertex.
             * @return repetition vector value of the vertex. (UINT32_MAX if uninitialized)
             */
            inline uint32_t repetitionValue() const { return repetitionValue_; };

            /**
             * @brief A const reference on the vector of refinement input params.
             * @return const reference to input params vector.
             */
            inline spider::array_view<u32> refinementParamIxVector() const {
                return spider::make_view(refinementParamArray_.get(), nRefinementParams_);
            };

            /**
             * @brief A const reference on the vector of input params.
             * @return const reference to input params vector.
             */
            inline spider::array_view<u32> inputParamIxVector() const {
                return spider::make_view(inputParamArray_.get(), nINParams_);
            };

            /**
             * @brief Get the number of input params connected to the vertex.
             * @return number of input params.
             */
            inline size_t inputParamCount() const { return nINParams_;; };

            /**
             * @brief A const reference on the vector of output params.
             * @return const reference to output params vector.
             */
            inline spider::array_view<u32> outputParamIxVector() const {
                return spider::make_view(outputParamArray_.get(), nOUTParams_);
            };

            /**
             * @brief Get the number of output params connected to the vertex.
             * @return number of output params.
             */
            inline size_t outputParamCount() const { return nOUTParams_; };

            /**
             * @brief Return the executable property of a vertex.
             * @return true if vertex is executable, false else.
             */
            inline virtual bool executable() const { return true; };

            /**
             * @brief Returns the @refitem RTInfo structure associated with this vertex.
             * @remark If the vertex is non-executable, it should return nullptr.
             * @return pointer to the @refitem RTInfo of the vertex, nullptr if !(this->executable()).
             */
            inline RTInfo *runtimeInformation() const { return rtInformation_.get(); };

            /* === Setter(s) === */

            /**
             * @brief Set the repetition vector value of the vertex;
             * @param value Repetition value to set.
             */
            void setRepetitionValue(uint32_t value);

            /**
             * @brief Set the name of the vertex.
             * @remark This method will replace current name of the vertex.
             * @warning No check on the name is performed to see if it is unique in the graph.
             * @param name  Name to set.
             */
            void setName(std::string name);

            /**
             * @brief Set the ix of the vertex in the containing graph.
             * @param ix Ix to set.
             */
            inline void setIx(size_t ix) { ix_ = ix; };

            /**
             * @brief Set the containing graph of the vertex.
             * @remark override current value.
             * @remark if graph is nullptr, nothing happens.
             * @param graph  Pointer to the graph to set.
             */
            void setGraph(Graph *graph);

        protected:
            spider::unique_ptr<u32> inputParamArray_;      /* = Array of input Params = */
            spider::unique_ptr<u32> outputParamArray_;     /* = Array of output Params = */
            spider::unique_ptr<u32> refinementParamArray_; /* = Array of refinement Params = */
            spider::unique_ptr<pisdf::Edge *> inputEdgeArray_;  /* = Array of input Edge = */
            spider::unique_ptr<pisdf::Edge *> outputEdgeArray_; /* = Array of output Edge = */
            spider::unique_ptr<char> name_;            /* = Name of the Vertex (uniqueness is not required) = */
            spider::unique_ptr<RTInfo> rtInformation_; /* = Runtime information of the Vertex (timing, mappable, etc.) = */
            Graph *graph_ = nullptr;           /* = Graph of the vertex = */
            size_t ix_ = SIZE_MAX;             /* = Index of the Vertex in the containing Graph = */
            u32 repetitionValue_ = 1;          /* = Repetition value of the Vertex, default is 1 but it can be set to 0. = */
            u32 nINEdges_ = 0;
            u32 nOUTEdges_ = 0;
            u32 nINParams_ = 0;
            u32 nOUTParams_ = 0;
            u32 nRefinementParams_ = 0;
            VertexType subtype_ = VertexType::NORMAL;

            /**
             * @brief Verify that VertexType and vertex properties are coherent.
             */
            void checkTypeConsistency() const;

        private:

            /**
             * @brief Disconnect an edge from the given edge vector (input or output).
             * @param edges  Vector of edges (input or output).
             * @param ix     Index of the edge to disconnect.
             * @return pointer to the disconnected edge, nullptr else.
             */
            Edge *disconnectEdge(Edge **edges, size_t ix);

            /**
             * @brief Connect an edge in the given edge vector (input or output).
             * @param edges  Vector of edges (input or output).
             * @param edge   Pointer to the edge to be connected.
             * @param ix     Index of the edge to connect.
             * @throw spider::Exception if edge already exists in the vector at given index.
             */
            void connectEdge(Edge **edges, Edge *edge, size_t ix);
        };
    }
}
#endif //SPIDER2_VERTEX_H
