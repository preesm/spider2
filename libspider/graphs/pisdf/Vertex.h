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
#ifndef SPIDER2_VERTEX_H
#define SPIDER2_VERTEX_H

/* === Include(s) === */

#include <cstdint>
#include <string>
#include <containers/array.h>
#include <graphs/pisdf/visitors/Visitor.h>
#include <runtime/info/RTInfo.h>

namespace spider {

    namespace pisdf {

        /* === Forward declaration(s) === */

        class Graph;

        struct CloneVisitor;

        /* === Class definition === */

        class Vertex {
        public:

            explicit Vertex(std::string name,
                            size_t edgeINCount = 0,
                            size_t edgeOUTCount = 0,
                            StackID stack = StackID::PISDF) :
                    name_{ std::move(name) },
                    inputEdgeArray_{ edgeINCount, nullptr, stack },
                    outputEdgeArray_{ edgeOUTCount, nullptr, stack } { }

            Vertex(const Vertex &other, StackID stack = StackID::PISDF);

            Vertex(Vertex &&other) noexcept = default;

            Vertex &operator=(const Vertex &) = delete;

            Vertex &operator=(Vertex &&) = delete;

            virtual ~Vertex() noexcept;

            friend CloneVisitor;

            /* === Method(s) === */

            /**
             * @brief Set the input edge of index ix.
             * @param edge Edge to set.
             * @param ix  Index of the edge.
             * @throw @refitem Spide::rException if out of bound or already existing edge.
             */
            inline virtual void connectInputEdge(Edge *edge, size_t ix) {
                connectEdge(inputEdgeArray_, edge, ix);
            }

            /**
             * @brief Set the output edge of index ix.
             * @param edge Edge to set.
             * @param ix  Index of the edge.
             * @throw @refitem Spider::Exception if out of bound or already existing edge.
             */
            inline virtual void connectOutputEdge(Edge *edge, size_t ix) {
                connectEdge(outputEdgeArray_, edge, ix);
            }

            /**
             * @brief Disconnect input edge on port ix. If no edge is connected, nothing happens
             * @param ix  Index of the input edge to disconnect.
             * @throws @refitem Spider::Exception if index out of bound.
             */
            virtual Edge *disconnectInputEdge(size_t ix);

            /**
             * @brief Disconnect output edge on port ix. If no edge is connected, nothing happens
             * @param ix  Index of the output edge to disconnect.
             * @throws @refitem Spider::Exception if index out of bound.
             */
            virtual Edge *disconnectOutputEdge(size_t ix);

            /**
             * @brief Generic method that accept a visitor for class specific treatments.
             * @remark This implement a double-dispatch visitor pattern.
             * @param visitor  Visitor to accept.
             */
            virtual void visit(Visitor *visitor) = 0;

            /**
             * @brief Create a runtime information structure for current vertex.
             * @remark if vertex already has constraints, nothing happens.
             * @return Created @refitem RTInfo.
             */
            inline RTInfo *createConstraints() {
                if (!rtInformation_) {
                    rtInformation_ = make<RTInfo>();
                }
                return rtInformation_;
            }

            /* === Getter(s) === */

            /**
             * @brief Get the containing @refitem Graph of the vertex.
             * @return containing @refitem Graph
             */
            inline Graph *graph() const {
                return graph_;
            }

            /**
             * @brief Get the name string of the vertex.
             * @return name of the vertex.
             */
            inline const std::string &name() const {
                return name_;
            }

            /**
             * @brief Get the ix of the vertex in the containing graph.
             * @return ix of the vertex (UINT32_MAX if no ix).
             */
            inline const uint32_t &ix() const {
                return ix_;
            }

            /**
             * @brief Test if the vertex is a graph.
             * @return true if vertex is a graph, false else.
             */
            inline virtual bool hierarchical() const {
                return false;
            }

            /**
             * @brief Get the repetition vector value of the vertex.
             * @return repetition vector value of the vertex. (UINT32_MAX if uninitialized)
             */
            inline uint32_t repetitionValue() const {
                return repetitionValue_;
            }

            /**
             * @brief A const reference on the array of input edges. Useful for iterating on the edges.
             * @return const reference to input edge array
             */
            inline const spider::array<Edge *> &inputEdgeArray() const {
                return inputEdgeArray_;
            }

            /**
             * @brief Get input edge connected to port Ix.
             * @param ix Index of the input port.
             * @return @refitem Spider::PiSDF::Edge
             * @throw @refitem Spider::Exception if out of bound
             */
            inline Edge *inputEdge(size_t ix) const {
                return inputEdgeArray_.at(ix);
            }

            /**
             * @brief Get the number of input edges connected to the vertex.
             * @return number of input edges.
             */
            inline size_t inputEdgeCount() const {
                return inputEdgeArray_.size();
            }

            /**
             * @brief A const reference on the array of output edges. Useful for iterating on the edges.
             * @return const reference to output edge array
             */
            inline const spider::array<Edge *> &outputEdgeArray() const {
                return outputEdgeArray_;
            }

            /**
             * @brief Get input edge connected to port Ix.
             * @param ix Index of the output port.
             * @return @refitem Spider::PiSDF::Edge
             * @throw @refitem Spider::Exception if out of bound.
             */
            inline Edge *outputEdge(size_t ix) const {
                return outputEdgeArray_.at(ix);
            }

            /**
             * @brief Get the number of output edges connected to the vertex.
             * @return number of output edges.
             */
            inline size_t outputEdgeCount() const {
                return outputEdgeArray_.size();
            }

            /**
             * @brief Get the subtype of the vertex.
             * @return @refitem Spider::PiSDF::VertexType corresponding to the subtype
             */
            virtual VertexType subtype() const = 0;

            /**
             * @brief Return the reference vertex attached to current copy.
             * @remark If vertex is not a copy, this return the vertex itself.
             * @warning There is a potential risk here. If the reference is freed before the copy,
             * there are no possibilities to know it.
             * @return pointer to @refitem Vertex reference.
             */
            inline const Vertex *reference() const {
                return reference_;
            }

            /**
             * @brief Return the executable property of a vertex.
             * @return true if vertex is executable, false else.
             */
            inline virtual bool executable() const {
                return false;
            }

            inline RTInfo *runtimeInformation() const {
                return rtInformation_;
            }

            /* === Setter(s) === */

            /**
             * @brief Set the name of the vertex.
             * @remark This method will replace current name of the vertex.
             * @warning No check on the name is performed to see if it is unique in the graph.
             * @param name  Name to set.
             */
            inline void setName(std::string name) {
                name_ = std::move(name);
            }

            /**
             * @brief Set the ix of the vertex in the containing graph.
             * @param ix Ix to set.
             */
            inline void setIx(uint32_t ix) {
                ix_ = ix;
            }

            /**
             * @brief Set the repetition vector value of the vertex;
             * @param value Repetition value to set.
             */
            inline virtual void setRepetitionValue(uint32_t value) {
                repetitionValue_ = value;
            }

            /**
             * @brief Set the graph of the vertex.
             * @remark This method changes current value.
             * @remark If graph is nullptr, nothing happen.
             * @param graph  Graph to set.
             */
            inline void setGraph(Graph *graph) {
                if (graph) {
                    graph_ = graph;
                }
            }

            /**
             * @brief Set RTInfo of this vertex.
             * @remark if nullptr, nothing happens.
             * @remark if vertex already has constraints, nothing happens.
             * @param rtInfo Constraints to set.
             */
            inline void setConstraints(RTInfo *rtInfo) {
                if (!rtInfo || rtInformation_) {
                    return;
                }
                rtInformation_ = rtInfo;
            }

        protected:
            std::string name_ = "unnamed-vertex";
            spider::array<Edge *> inputEdgeArray_;
            spider::array<Edge *> outputEdgeArray_;
            const Vertex *reference_ = this;
            Graph *graph_ = nullptr;
            RTInfo *rtInformation_ = nullptr;
            uint32_t ix_ = UINT32_MAX;
            uint32_t repetitionValue_ = 1;
            mutable uint32_t copyCount_ = 0;

            /* === Private method(s) === */

            static Edge *disconnectEdge(spider::array<Edge *> &edges, size_t ix);

            static void connectEdge(spider::array<Edge *> &edges, Edge *edge, size_t ix);
        };
    }
}
#endif //SPIDER2_VERTEX_H
