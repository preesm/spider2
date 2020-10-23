/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_SCHEDVERTEX_H
#define SPIDER2_SCHEDVERTEX_H

/* === Include(s) === */

#include <containers/vector.h>
#include <memory/unique_ptr.h>
#include <runtime/common/RTInfo.h>
#include <scheduling/memory/JobFifos.h>

namespace spider {

    namespace sched {

        class Edge;

        class Graph;

        enum class Type {
            NORMAL,
            MERGE,
            FORK,
            DUPLICATE,
            SEND,
            RECEIVE,
        };

        enum class State : u8 {
            NOT_SCHEDULABLE = 0,
            NOT_RUNNABLE,
            SKIPPED,
            PENDING,
            READY,
            RUNNING,
        };

        /* === Class definition === */

        class Vertex {
        public:
            explicit Vertex(size_t edgeINCount = 0,
                            size_t edgeOUTCount = 0);

            Vertex(Vertex &&) noexcept = default;

            Vertex &operator=(Vertex &&) = default;

            Vertex(const Vertex &) = delete;

            Vertex &operator=(const Vertex &) = delete;

            virtual ~Vertex();

            /* === Method(s) === */

            /**
            * @brief Connect an input edge at given position.
            * @param edge  Pointer to the edge to connect.
            * @param pos   Input position where to connect the edge.
            * @throw @refitem std::out_of_range.
            * @throw @refitem spider::Exception if an edge already exists at this position.
            */
            void connectInputEdge(sched::Edge *edge, size_t pos);

            /**
             * @brief Connect an output edge at given position.
             * @param edge  Pointer to the edge to connect.
             * @param pos   Output position where to connect the edge.
             * @throw @refitem std::out_of_range.
             * @throw @refitem spider::Exception if an edge already exists at this position.
             */
            void connectOutputEdge(sched::Edge *edge, size_t pos);

            /**
             * @brief Disconnect input edge on port ix. If no edge is connected, nothing happens.
             * @remark Call @refitem Edge::setSink to reset the edge if found.
             * @param ix  Index of the input edge to disconnect.
             */
            sched::Edge *disconnectInputEdge(size_t ix);

            /**
             * @brief Disconnect output edge on port ix. If no edge is connected, nothing happens
             * @remark Call @refitem Edge::setSource to reset the edge if found.
             * @param ix  Index of the output edge to disconnect.
             */
            sched::Edge *disconnectOutputEdge(size_t ix);

            /**
             * @brief Compute the communication cost and the data size that would need to be send if a task is mapped
             *        on a given PE.
             * @param mappedPE  PE on which the task is currently mapped.
             * @return pair containing the communication cost as first and the total size of data to send as second.
             */
            std::pair<ufast64, ufast64> computeCommunicationCost(const PE *mappedPE) const;

            /**
             * @brief Check if the task is mappable on a given PE.
             * @param pe  Pointer to the PE.
             * @return true if mappable on PE, false else.
             */
            virtual inline bool isMappableOnPE(const PE */* pe */) const { return true; }

            /**
             * @brief Get the execution timing on a given PE.
             * @param pe  Pointer to the PE.
             * @return exec timing on the PE, UINT64_MAX else.
             */
            virtual inline u64 timingOnPE(const PE */* pe */) const { return UINT64_MAX; }

            /**
             * @brief Try to remove this synchronization point (if possible).
             * @param graph Pointer to the schedule graph.
             */
            inline virtual bool reduce(sched::Graph */*graph*/) { return false; };

            /**
             * @brief Send the execution job associated with this vertex to its mapped LRT and set state as RUNNING.
             */
            void send();

            /**
             * @brief Update output params based on received values.
             * @param values Values of the params.
             */
            inline virtual void receiveParams(const spider::array<i64> &/*values*/) { }

            /* === Getter(s) === */

            /**
             * @brief Get the sched type of the task.
             * @return Type of the task
             */
            virtual sched::Type type() const = 0;

            /**
             * @brief Get the name string of the vertex.
             * @return name of the vertex.
             */
            virtual std::string name() const = 0;

            /**
             * @brief Return a color value for the task.
             *        format is RGB with 8 bits per component in the lower part of the returned value.
             * @return  color of the task.
             */
            virtual u32 color() const = 0;

            /**
             * @brief Get the ix of the vertex in the containing graph.
             * @return ix of the vertex (SIZE_MAX if no ix).
             */
            inline u32 ix() const { return ix_; }

            /**
             * @brief A const reference on the array of input edges. Useful for iterating on the edges.
             * @return const reference to input edge array
             */
            inline spider::array_handle<sched::Edge *> inputEdges() const {
                return spider::make_handle(inputEdgeArray_, nINEdges_);
            };

            /**
             * @brief Get input edge connected to port Ix.
             * @param ix Index of the input port.
             * @return @refitem spider::pisdf::Edge
             * @throw std::out_of_range.
             */
            inline sched::Edge *inputEdge(size_t ix) const {
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
            inline spider::array_handle<sched::Edge *> outputEdges() const {
                return spider::make_handle(outputEdgeArray_, nOUTEdges_);
            };

            /**
             * @brief Get input edge connected to port Ix.
             * @param ix Index of the output port.
             * @return @refitem spider::pisdf::Edge
             * @throw std::out_of_range.
             */
            inline sched::Edge *outputEdge(size_t ix) const {
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
             * @brief Get the start time of the task.
             * @return mapping start time of the task, UINT64_MAX else
             */
            inline u64 startTime() const { return startTime_; }

            /**
             * @brief Get the end time of the task.
             * @return mapping end time of the task, UINT64_MAX else
             */
            inline u64 endTime() const { return endTime_; }

            /**
             * @brief Returns the PE on which the task is mapped.
             * @return pointer to the PE onto which the task is mapped, nullptr else
             */
            inline const PE *mappedPe() const { return mappedPE_; }

            /**
             * @brief Returns the LRT attached to the PE on which the task is mapped.
             * @return pointer to the LRT, nullptr else
             */
            inline const PE *mappedLRT() const { return mappedPE_->attachedLRT(); }

            /**
             * @brief Returns the state of the task.
             * @return @refitem SchedState of the task
             */
            inline State state() const noexcept { return state_; }

            /**
             * @brief Get notification flag for given LRT.
             * @remark no boundary check is performed.
             * @return boolean flag indicating if this task is notifying given LRT.
             */
            inline bool getNotificationFlagForLRT(size_t ix) const { return notifications_.get()[ix]; }

            /**
             * @brief Returns the executable job index value of the task in the job queue of the mapped PE.
             * @return ix value, SIZE_MAX else.
             */
            inline u32 jobExecIx() const noexcept { return jobExecIx_; }

            /* === Setter(s) === */

            /**
             * @brief Set the ix of the vertex in the containing graph.
             * @param ix Ix to set.
             */
            virtual inline void setIx(u32 ix) { ix_ = ix; };

            /**
             * @brief Set the start time of the job.
             * @remark This method will overwrite current value.
             * @param time  Value to set.
             */
            inline void setStartTime(u64 time) { startTime_ = time; }

            /**
             * @brief Set the end time of the job.
             * @remark This method will overwrite current value.
             * @param time  Value to set.
             */
            inline void setEndTime(u64 time) { endTime_ = time; }

            /**
            * @brief Set the processing element of the job.
            * @remark This method will overwrite current values.
            * @param mappedPE  Lrt ix inside spider.
            */
            inline void setMappedPE(const PE *pe) { mappedPE_ = pe; }

            /**
             * @brief Set the state of the job.
             * @remark This method will overwrite current value.
             * @param state State to set.
             */
            inline void setState(State state) noexcept { state_ = state; }

            /**
             * @brief Set the notification flag for this lrt.
             * @warning There is no check on the value of lrt.
             * @param lrt   Index of the lrt.
             * @param value Value to set: true = should notify, false = should not notify.
             */
            inline void setNotificationFlag(size_t lrt, bool value) { notifications_.get()[lrt] = value; }

            /**
             * @brief Set the execution job index value of the task (that will be used for synchronization).
             * @remark This method will overwrite current values.
             * @param ix Ix to set.
             */
            inline void setJobExecIx(u32 ix) noexcept { jobExecIx_ = ix; }

        protected:

            inline virtual u32 getOutputParamsCount() const { return 0; }

            inline virtual u32 getKernelIx() const { return UINT32_MAX; }

            inline virtual spider::unique_ptr<i64> buildInputParams() const { return spider::unique_ptr<i64>(); }

        private:
            spider::unique_ptr<bool> notifications_;  /*!< Notification flags of the task */
            sched::Edge **inputEdgeArray_ = nullptr;  /*!< Array of input Edge = */
            sched::Edge **outputEdgeArray_ = nullptr; /*!< Array of output Edge = */
            const PE *mappedPE_{ nullptr };           /*!< Mapping PE of the task */
            u64 startTime_{ UINT64_MAX };             /*!< Mapping start time of the task */
            u64 endTime_{ UINT64_MAX };               /*!< Mapping end time of the task */
            u32 ix_{ UINT32_MAX };                    /*!< Index of the task in the schedule */
            u32 jobExecIx_{ UINT32_MAX };             /*!< Index of the job sent to the PE */
            u32 nINEdges_ = 0;
            u32 nOUTEdges_ = 0;
            State state_{ State::NOT_SCHEDULABLE }; /*!< State of the task */

            /**
             * @brief Disconnect an edge from the given edge vector (input or output).
             * @param edges  Vector of edges (input or output).
             * @param ix     Index of the edge to disconnect.
             * @return pointer to the disconnected edge, nullptr else.
             */
            static sched::Edge *disconnectEdge(sched::Edge **edges, size_t ix);

            /**
             * @brief Connect an edge in the given edge vector (input or output).
             * @param edges  Vector of edges (input or output).
             * @param edge   Pointer to the edge to be connected.
             * @param ix     Index of the edge to connect.
             * @throw spider::Exception if edge already exists in the vector at given index.
             */
            static void connectEdge(sched::Edge **edges, Edge *edge, size_t ix);

            /**
             * @brief Based on current state of the schedule graph and of the vertex, build an array of booleans
             *        corresponding to the LRT to notify.
             *        The array may be empty if no notifications are required at all.
             * @return @refitem spider::unique_ptr of booleans
             */
            spider::unique_ptr<bool> buildJobNotificationFlags() const;

            /**
             * @brief Build and set the fifos needed by the job to run.
             * @return @refitem std::shared_ptr of JobFifos.
             */
            std::shared_ptr<spider::JobFifos> buildJobFifos() const;
        };

    }
}
#endif //SPIDER2_SCHEDVERTEX_H
