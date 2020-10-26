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
#ifndef SPIDER2_TASK_H
#define SPIDER2_TASK_H

/* === Include(s) === */

#include <memory/memory.h>
#include <common/Types.h>
#include <containers/array.h>
#include <archi/PE.h>
#include <scheduling/memory/JobFifos.h>
#include <runtime/message/JobMessage.h>

namespace spider {

    /* === Forward Declaration(s) === */

    class PE;

    namespace sched {

        class Task;

        class SyncTask;

        class Schedule;

        enum class TaskState : u8 {
            NOT_SCHEDULABLE = 0,
            NOT_RUNNABLE,
            PENDING,
            READY,
            RUNNING,
            SKIPPED,
        };

        /* === Class definition === */

        class Task {
        public:
            Task() = default;

            virtual ~Task() noexcept = default;

            Task(Task &&) noexcept = default;

            Task &operator=(Task &&) noexcept = default;

            Task(const Task &) = delete;

            Task &operator=(const Task &) noexcept = delete;

            /* === Method(s) === */

            /**
             * @brief Send the execution job associated with this vertex to its mapped LRT and set state as RUNNING.
             * @param schedule  Schedule to which the vertexTask is associated.
             */
            void send(const Schedule *schedule);

            /**
             * @brief Update output params based on received values.
             * @param values Values of the params.
             */
            inline virtual void receiveParams(const spider::array<i64> &/*values*/) { }

            /**
             * @brief Insert a synchronization tasks before this task.
             *        Only used to set the allocated address of the sync task.
             * @param sndTask  Pointer to the send task.
             * @param rcvTask  Pointer to the receive task.
             * @param schedule Pointer to the schedule.
             * @param ix       Index of the input dependency onto which the sync task is set.
             */
            inline virtual void insertSyncTasks(SyncTask */*sndTask*/,
                                                SyncTask */*rcvTask*/,
                                                size_t /*ix*/,
                                                const Schedule */*schedule*/) { }

            /* === Getter(s) === */

            /**
             * @brief Get the input rate for the fifo of index ix.
             * @param ix  Index of the input fifo.
             * @return rate of the fifo.
             */
            virtual i64 inputRate(size_t ix) const = 0;

            /**
             * @brief Get the output rate for the fifo of index ix.
             * @param ix  Index of the output fifo.
             * @return rate of the fifo.
             */
            virtual i64 outputRate(size_t ix) const = 0;

            /**
             * @brief Get the previous Task of a given index.
             * @param ix       Index of the Task.
             * @param schedule Schedule to wich the Task is associated.
             * @return pointer to the previous Task, nullptr else.
             * @throws @refitem spider::Exception if index out of bound (only in debug)
             */
            virtual Task *previousTask(size_t ix, const Schedule *schedule) const = 0;

            /**
             * @brief Get the next Task of a given index.
             * @param ix       Index of the Task.
             * @param schedule Schedule to wich the Task is associated.
             * @return pointer to the next Task, nullptr else.
             * @throws @refitem spider::Exception if index out of bound (only in debug)
             */
            virtual Task *nextTask(size_t ix, const Schedule *schedule) const = 0;

            /**
             * @brief Return a color value for the vertexTask.
             *        format is RGB with 8 bits per component in the lower part of the returned value.
             * @return  color of the vertexTask.
             */
            virtual u32 color() const = 0;

            /**
             * @brief Returns the name of the vertexTask
             * @return name of the vertexTask
             */
            virtual std::string name() const = 0;

            /**
             * @brief Check if the vertexTask is mappable on a given PE.
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
             * @brief Get the number of execution dependencies for this task.
             * @return number of dependencies.
             */
            virtual size_t dependencyCount() const = 0;


            /**
             * @brief Get the number of consumer dependencies for this task.
             * @return number of dependencies.
             */
            virtual size_t successorCount() const = 0;

            /**
             * @brief Get the start time of the vertexTask.
             * @return mapping start time of the vertexTask, UINT64_MAX else
             */
            inline u64 startTime() const { return startTime_; }

            /**
             * @brief Get the end time of the vertexTask.
             * @return mapping end time of the vertexTask, UINT64_MAX else
             */
            inline u64 endTime() const { return endTime_; }

            /**
             * @brief Returns the PE on which the vertexTask is mapped.
             * @return pointer to the PE onto which the vertexTask is mapped, nullptr else
             */
            const PE *mappedPe() const;

            /**
             * @brief Returns the LRT attached to the PE on which the vertexTask is mapped.
             * @return pointer to the LRT, nullptr else
             */
            inline const PE *mappedLRT() const { return mappedPe()->attachedLRT(); }

            /**
             * @brief Returns the state of the vertexTask.
             * @return @refitem TaskState of the vertexTask
             */
            inline TaskState state() const noexcept { return state_; }

            /**
             * @brief Returns the ix of the vertexTask in the schedule.
             * @return ix of the vertexTask in the schedule, -1 else.
             */
            inline virtual u32 ix() const noexcept { return UINT32_MAX; }

            /**
             * @brief Returns the executable job index value of the vertexTask in the job queue of the mapped PE.
             * @return ix value, SIZE_MAX else.
             */
            inline u32 jobExecIx() const noexcept { return jobExecIx_; }

            /* === Setter(s) === */

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
            inline void setMappedPE(const PE *pe) { mappedPEIx_ = static_cast<u32>(pe->virtualIx()); }

            /**
             * @brief Set the state of the job.
             * @remark This method will overwrite current value.
             * @param state State to set.
             */
            inline void setState(TaskState state) noexcept { state_ = state; }

            /**
             * @brief Set the execution job index value of the vertexTask (that will be used for synchronization).
             * @remark This method will overwrite current values.
             * @param ix Ix to set.
             */
            inline void setJobExecIx(u32 ix) noexcept { jobExecIx_ = ix; }

            /**
             * @brief Set the ix of the job.
             * @remark This method will overwrite current value.
             * @param ix Ix to set.
             */
            virtual void setIx(u32 ix) noexcept = 0;

        protected:

            /**
             * @brief Get the number of output parameters of this task (default is 0).
             * @return number of output parameters.
             */
            inline virtual u32 getOutputParamsCount() const { return 0; }

            /**
             * @brief Get the kernel identifier for the task execution.
             * @return Kernel index, UINT32_MAX else.
             */
            inline virtual u32 getKernelIx() const { return UINT32_MAX; }

            /**
             * @brief Build the input parameters needed by the task execution.
             * @return input parameters array (if any), empty array else.
             */
            inline virtual spider::unique_ptr<i64> buildInputParams() const { return spider::unique_ptr<i64>(); }

            /**
             * @brief Build the FIFOs needed by the task execution.
             * @param schedule Pointer to the schedule.
             * @return @refitem std::shared_ptr of @refitem JobFifos
             */
            virtual std::shared_ptr<JobFifos> buildJobFifos(const Schedule *schedule) const = 0;

        private:
            u64 startTime_{ UINT64_MAX };                   /*!< Mapping start time of the vertexTask */
            u64 endTime_{ UINT64_MAX };                     /*!< Mapping end time of the vertexTask */
            u32 mappedPEIx_ = UINT32_MAX;                   /*!< Mapping PE of the vertexTask */
            u32 jobExecIx_{ UINT32_MAX };                   /*!< Index of the job sent to the PE */
            TaskState state_{ TaskState::NOT_SCHEDULABLE }; /*!< State of the vertexTask */

            /* === Private method(s) === */

            /**
             * @brief Build the notification flags for this task.
             * @param schedule  Pointer to the schedule.
             * @return array of only true booleans if this task need to broadcast its job stamp,
             *         empty array if no notifications are required,
             *         array with corresponding flags for each LRT else.
             */
            spider::unique_ptr<bool> buildJobNotificationFlags(const Schedule *schedule) const;

            /**
             * @brief Build execution constraints for this task (needed job + lrt).
             * @param schedule Pointer to the schedule.
             * @return array of constraints if any, empty array else.
             */
            spider::array<SyncInfo> buildExecConstraints(const Schedule *schedule) const;

            /**
             * @brief Based on current state of the mapping / scheduling, fill the boolean array "flags" with
             *        true if given LRT is to be notified, false else.
             * @param flags    Array of boolean, should be EXACTLY of size archi::platform()->LRTCount();
             * @param schedule Pointer to the schedule.
             * @return True if at least one LRT will be notified by this task.
             */
            bool updateNotificationFlags(bool *flags, const Schedule *schedule) const;
        };
    }
}

#endif //SPIDER2_TASK_H
