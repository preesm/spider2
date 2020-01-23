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
#include <graphs-tools/helper/visitors/PiSDFVisitor.h>
#include <runtime/common/RTInfo.h>

namespace spider {

    namespace pisdf {

        /* === Forward declaration(s) === */

        class Graph;

        /* === Class definition === */

        class Vertex {
        public:

            explicit Vertex(std::string name, size_t edgeINCount = 0, size_t edgeOUTCount = 0);

            Vertex(std::string name,
                   size_t edgeINCount,
                   size_t edgeOUTCount,
                   size_t paramINCount,
                   size_t paramOUTCount,
                   const Vertex *reference);

            Vertex(const Vertex &other);

            Vertex(Vertex &&) noexcept = default;

            Vertex &operator=(const Vertex &) = default;

            Vertex &operator=(Vertex &&) = default;

            virtual ~Vertex() noexcept;

            /* === Method(s) === */

            /**
             * @brief Set the input edge of index ix.
             * @param edge Edge to set.
             * @param ix  Index of the edge.
             * @throw @refitem Spide::rException if out of bound or already existing edge.
             */
            virtual void connectInputEdge(Edge *edge, size_t ix);

            /**
             * @brief Set the output edge of index ix.
             * @param edge Edge to set.
             * @param ix  Index of the edge.
             * @throw @refitem Spider::Exception if out of bound or already existing edge.
             */
            virtual void connectOutputEdge(Edge *edge, size_t ix);

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
             * @brief Add an input parameter to the Vertex.
             * @param param  Pointer to the parameter to add.
             */
            virtual void addInputParameter(std::shared_ptr<Param> param);

            /**
             * @brief Add an input parameter for the refinement of the Vertex.
             * @warning a separate call to addInputParameter is needed.
             * @param param  Pointer to the parameter to add.
             */
            virtual void addRefinementParameter(std::shared_ptr<Param> param);

            /**
             * @brief Add an output parameter to the Vertex.
             * @param param  Pointer to the parameter to add.
             * @throw spider::Exception if subtype() is not @refitem VertexType::CONFIG.
             */
            virtual void addOutputParameter(std::shared_ptr<Param> param);

            /**
             * @brief Get the complete path of the Vertex.
             * @example: vertex name = "vertex_0", graph name = "top_graph"
             *           -> vertex path = "top_graph-vertex_0"
             * @return complete vertex path
             */
            std::string vertexPath() const;

            /* === Getter(s) === */

            /**
             * @brief Get the containing @refitem Graph of the vertex.
             * @return containing @refitem Graph
             */
            inline Graph *graph() const { return graph_; };

            /**
             * @brief Get the name string of the vertex.
             * @return name of the vertex.
             */
            const std::string &name() const;

            /**
             * @brief Get the ix of the vertex in the containing graph.
             * @return ix of the vertex (UINT32_MAX if no ix).
             */
            inline const size_t &ix() const { return ix_; };

            /**
             * @brief Test if the vertex is a graph.
             * @return true if vertex is a graph, false else.
             */
            inline virtual bool hierarchical() const { return false; };

            /**
             * @brief Get the repetition vector value of the vertex.
             * @return repetition vector value of the vertex. (UINT32_MAX if uninitialized)
             */
            inline uint32_t repetitionValue() const { return repetitionValue_; };

            /**
             * @brief A const reference on the array of input edges. Useful for iterating on the edges.
             * @return const reference to input edge array
             */
            inline const spider::vector<Edge *> &inputEdgeVector() const { return inputEdgeVector_; };

            /**
             * @brief Get input edge connected to port Ix.
             * @param ix Index of the input port.
             * @return @refitem Spider::PiSDF::Edge
             * @throw @refitem Spider::Exception if out of bound
             */
            inline Edge *inputEdge(size_t ix) const { return inputEdgeVector_.at(ix); };

            /**
             * @brief Get the number of input edges connected to the vertex.
             * @return number of input edges.
             */
            inline size_t inputEdgeCount() const { return inputEdgeVector_.size(); };

            /**
             * @brief A const reference on the array of output edges. Useful for iterating on the edges.
             * @return const reference to output edge array.
             */
            inline const spider::vector<Edge *> &outputEdgeVector() const { return outputEdgeVector_; };

            /**
             * @brief Get input edge connected to port Ix.
             * @param ix Index of the output port.
             * @return @refitem Spider::PiSDF::Edge
             * @throw @refitem Spider::Exception if out of bound.
             */
            inline Edge *outputEdge(size_t ix) const { return outputEdgeVector_.at(ix); };

            /**
             * @brief Get the number of output edges connected to the vertex.
             * @return number of output edges.
             */
            inline size_t outputEdgeCount() const { return outputEdgeVector_.size(); };

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
             * @brief Get the subtype of the vertex.
             * @return @refitem Spider::PiSDF::VertexType corresponding to the subtype
             */
            virtual inline VertexType subtype() const { return VertexType::NORMAL; };

            /**
             * @brief Return the reference vertex attached to current copy.
             * @remark If vertex is not a copy, this return the vertex itself.
             * @warning There is a potential risk here. If the reference is freed before the copy,
             * there are no possibilities to know it.
             * @return pointer to @refitem Vertex reference.
             */
            inline const Vertex *reference() const { return reference_; };

            /**
             * @brief Return the executable property of a vertex.
             * @return true if vertex is executable, false else.
             */
            inline virtual bool executable() const { return false; };

            /**
             * @brief Returns the @refitem RTInfo structure associated with this vertex.
             * @remark If the vertex is non-executable, it should return nullptr.
             * @return pointer to the @refitem RTInfo of the vertex, nullptr if !(this->executable()).
             */
            inline RTInfo *runtimeInformation() const { return rtInformation_.get(); };

            /**
             * @brief Get the schedule job ix associated to this vertex.
             * @return ix of the job, SIZE_MAX if not set.
             */
            inline size_t scheduleJobIx() const { return scheduleJobIx_; };

            /**
             * @brief Get the instance value associated to this clone vertex (0 if original).
             * @return instance value of the vertex, 0 by default.
             */
            inline size_t instanceValue() const { return instanceValue_; };

            /* === Setter(s) === */

            /**
             * @brief Set the name of the vertex.
             * @remark This method will replace current name of the vertex.
             * @warning No check on the name is performed to see if it is unique in the graph.
             * @param name  Name to set.
             */
            inline void setName(std::string name) { name_ = std::move(name); };

            /**
             * @brief Set the ix of the vertex in the containing graph.
             * @param ix Ix to set.
             */
            inline void setIx(size_t ix) { ix_ = ix; };

            /**
             * @brief Set the repetition vector value of the vertex;
             * @param value Repetition value to set.
             */
            inline virtual void setRepetitionValue(uint32_t value) { repetitionValue_ = value; };

            /**
             * @brief Set the graph of the vertex.
             * @remark This method changes current value.
             * @remark If graph is nullptr, nothing happen.
             * @param graph  Graph to set.
             */
            void setGraph(Graph *graph);

            /**
             * @brief Set the schedule job ix of the vertex.
             * @param ix  Ix to set.
             */
            inline void setScheduleJobIx(size_t ix) { scheduleJobIx_ = ix; };

            /**
             * @brief Set the instance value of the vertex.
             * @param value  Value to set.
             * @throws spider::Exception if value is invalid relative to reference vertex repetition value.
             */
            void setInstanceValue(size_t value);

        protected:
            std::string name_ = "unnamed-vertex"; /* =  Name of the Vertex (uniqueness is not required) = */
            stack_vector(inputEdgeVector_, Edge *, StackID::PISDF);         /* = Vector of input Edge        = */
            stack_vector(outputEdgeVector_, Edge *, StackID::PISDF);        /* = Vector of output Edge       = */
            /* = Vector of input Param = */
            stack_vector(inputParamVector_, std::shared_ptr<pisdf::Param>, StackID::PISDF);
            /* = Vector of refinement Params = */
            stack_vector(refinementParamVector_, std::shared_ptr<Param>, StackID::PISDF);
            /* = Vector of output Param = */
            stack_vector(outputParamVector_, std::shared_ptr<Param>, StackID::PISDF);
            std::shared_ptr<RTInfo> rtInformation_;  /* = Runtime information of the Vertex (timing, mappable, etc.) = */
            const Vertex *reference_ = this;   /* =
                                                * Pointer to the reference Vertex.
                                                * Default is this, in case of copy, point to the original Vertex.
                                                * = */
            Graph *graph_ = nullptr;           /* = Containing Graph of the Vertex (can be nullptr) = */
            size_t scheduleJobIx_ = SIZE_MAX;  /* =
                                                * Index of the schedule job associated to this Vertex.
                                                * Needed in case of deletion of vertex between successive.
                                                * schedule pass in order to maintain coherence.
                                                * = */
            size_t instanceValue_ = 0;         /* = Value of the instance relative to reference Vertex = */
            size_t ix_ = SIZE_MAX;             /* = Index of the Vertex in the containing Graph = */
            uint32_t repetitionValue_ = 1;     /* = Repetition value of the Vertex, default is 1 but it can be set to 0. = */
            mutable uint32_t copyCount_ = 0;   /* = Number of copy of the Vertex = */
        };
    }
}
#endif //SPIDER2_VERTEX_H
