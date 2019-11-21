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
#include <graphs/pisdf/common/Types.h>
#include <graphs/pisdf/visitors/Visitor.h>

namespace spider {

    namespace pisdf {

        /* === Forward declaration(s) === */

        class Graph;

        struct CloneVertexVisitor;

        /* === Class definition === */

        class Vertex {
        public:

            explicit Vertex(std::string name = "unnamed-vertex",
                            std::uint32_t edgeINCount = 0,
                            std::uint32_t edgeOUTCount = 0,
                            StackID stack = StackID::PISDF);

            Vertex(const Vertex &other) = delete;

            virtual ~Vertex();

            friend CloneVertexVisitor;

            /* === Method(s) === */

            /**
             * @brief Set the input edge of index ix.
             * @param edge Edge to set.
             * @param ix  Index of the edge.
             * @throw @refitem Spide::rException if out of bound or already existing edge.
             */
            virtual void connectInputEdge(Edge *edge, std::uint32_t ix);

            /**
             * @brief Set the output edge of index ix.
             * @param edge Edge to set.
             * @param ix  Index of the edge.
             * @throw @refitem Spider::Exception if out of bound or already existing edge.
             */
            virtual void connectOutputEdge(Edge *edge, std::uint32_t ix);

            /**
             * @brief Disconnect input edge on port ix. If no edge is connected, nothing happens
             * @param ix  Index of the input edge to disconnect.
             * @throws @refitem Spider::Exception if index out of bound.
             */
            virtual Edge *disconnectInputEdge(std::uint32_t ix);

            /**
             * @brief Disconnect output edge on port ix. If no edge is connected, nothing happens
             * @param ix  Index of the output edge to disconnect.
             * @throws @refitem Spider::Exception if index out of bound.
             */
            virtual Edge *disconnectOutputEdge(std::uint32_t ix);

            /**
             * @brief Forward the connection of an edge. It should return this except for @refitem Interface vertices.
             * @return this or vertex connected to edge.
             */
            inline virtual Vertex *forwardEdge(const Edge *);

            virtual void visit(Visitor *) = 0;

            /* === Getter(s) === */

            /**
             * @brief Get the containing @refitem Graph of the vertex.
             * @return containing @refitem Graph
             */
            inline Graph *containingGraph() const;

            /**
             * @brief Get the name string of the vertex.
             * @return name of the vertex.
             */
            inline const std::string &name() const;

            /**
             * @brief Get the ix of the vertex in the containing graph.
             * @return ix of the vertex (UINT32_MAX if no ix).
             */
            inline const std::uint32_t &ix() const;

            /**
             * @brief Test if the vertex is a graph.
             * @return true if vertex is a graph, false else.
             */
            inline virtual bool hierarchical() const;

            /**
             * @brief Get the repetition vector value of the vertex.
             * @return repetition vector value of the vertex. (UINT32_MAX if uninitialized)
             */
            inline std::uint32_t repetitionValue() const;

            /**
             * @brief A const reference on the array of input edges. Useful for iterating on the edges.
             * @return const reference to input edge array
             */
            inline const spider::array<Edge *> &inputEdgeArray() const;

            /**
             * @brief Get input edge connected to port Ix.
             * @param ix Index of the input port.
             * @return @refitem Spider::PiSDF::Edge
             * @throw @refitem Spider::Exception if out of bound
             */
            inline Edge *inputEdge(std::uint32_t ix) const;

            /**
             * @brief Get the number of input edges connected to the vertex.
             * @return number of input edges.
             */
            inline std::uint32_t inputEdgeCount() const;

            /**
             * @brief A const reference on the array of output edges. Useful for iterating on the edges.
             * @return const reference to output edge array
             */
            inline const spider::array<Edge *> &outputEdgeArray() const;

            /**
             * @brief Get input edge connected to port Ix.
             * @param ix Index of the output port.
             * @return @refitem Spider::PiSDF::Edge
             * @throw @refitem Spider::Exception if out of bound.
             */
            inline Edge *outputEdge(std::uint32_t ix) const;

            /**
             * @brief Get the number of output edges connected to the vertex.
             * @return number of output edges.
             */
            inline std::uint32_t outputEdgeCount() const;

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
            inline const Vertex *reference() const;

            /**
             * @brief Return the executable property of a vertex.
             * @return true if vertex is executable, false else.
             */
            inline virtual bool executable() const;

            /* === Setter(s) === */

            /**
             * @brief Set the name of the vertex.
             * @remark This method will replace current name of the vertex.
             * @warning No check on the name is performed to see if it is unique in the graph.
             * @param name  Name to set.
             */
            inline void setName(std::string name);

            /**
             * @brief Set the ix of the vertex in the containing graph.
             * @param ix Ix to set.
             */
            inline void setIx(std::uint32_t ix);

            /**
             * @brief Set the repetition vector value of the vertex;
             * @param rv Repetition value to set.
             */
            inline virtual void setRepetitionValue(std::uint32_t rv);

            /**
             * @brief Set the graph of the vertex.
             * @remark This method changes current value.
             * @remark If graph is nullptr, nothing happen.
             * @param graph  Graph to set.
             */
            void setGraph(Graph *graph);

        protected:
            Graph *graph_ = nullptr;
            std::string name_ = "unnamed-vertex";
            std::uint32_t ix_ = UINT32_MAX;
            std::uint32_t repetitionValue_ = 1;
            std::uint32_t copyCount_ = 0;
            Vertex *reference_ = this;

            spider::array<Edge *> inputEdgeArray_;
            spider::array<Edge *> outputEdgeArray_;

            /* === Private method(s) === */

            static Edge *disconnectEdge(spider::array<Edge *> &edges, std::uint32_t ix);

            static void connectEdge(spider::array<Edge *> &edges, Edge *edge, std::uint32_t ix);
        };

        /* === Inline method(s) === */

        Vertex *Vertex::forwardEdge(const Edge *) {
            return this;
        }

        Graph *Vertex::containingGraph() const {
            return graph_;
        }

        const std::string &Vertex::name() const {
            return name_;
        }

        const std::uint32_t &Vertex::ix() const {
            return ix_;
        }

        bool Vertex::hierarchical() const {
            return false;
        }

        std::uint32_t Vertex::repetitionValue() const {
            return repetitionValue_;
        }

        const spider::array<Edge *> &Vertex::inputEdgeArray() const {
            return inputEdgeArray_;
        }

        Edge *Vertex::inputEdge(std::uint32_t ix) const {
            return inputEdgeArray_.at(ix);
        }

        std::uint32_t Vertex::inputEdgeCount() const {
            return inputEdgeArray_.size();
        }

        const spider::array<Edge *> &Vertex::outputEdgeArray() const {
            return outputEdgeArray_;
        }

        Edge *Vertex::outputEdge(std::uint32_t ix) const {
            return outputEdgeArray_.at(ix);
        }

        std::uint32_t Vertex::outputEdgeCount() const {
            return outputEdgeArray_.size();
        }

        bool Vertex::executable() const {
            return false;
        }

        const Vertex *Vertex::reference() const {
            return reference_;
        }

        void Vertex::setName(std::string name) {
            name_ = std::move(name);
        }

        void Vertex::setIx(std::uint32_t ix) {
            ix_ = ix;
        }

        void Vertex::setRepetitionValue(std::uint32_t rv) {
            repetitionValue_ = rv;
        }
    }
}
#endif //SPIDER2_VERTEX_H
