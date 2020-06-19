/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_SCHEDULETASK_H
#define SPIDER2_SCHEDULETASK_H

/* === Include(s) === */

#include <runtime/common/RTFifo.h>
#include <runtime/interface/Message.h>
#include <memory/unique_ptr.h>
#include <containers/array.h>
#include <containers/vector.h>
#include <common/Types.h>

namespace spider {

    class ScheduleTask;

    class TaskMemory;

    enum class TaskState : char {
        NOT_SCHEDULABLE = 0,
        PENDING,
        READY,
        RUNNING,
    };

    enum class TaskType : char {
        VERTEX = 0,
        SYNC_SEND,
        SYNC_RECEIVE,
    };

    struct ComTaskInformation {
        ScheduleTask *successor_{ nullptr };
        ufast64 size_{ 0 };
        size_t kernelIx_{ SIZE_MAX };
        i32 inputPortIx_{ 0 };
        i32 packetIx_{ 0 };
    };

    /* === Class definition === */

    class ScheduleTask {
    public:
        explicit ScheduleTask(TaskType type);

        explicit ScheduleTask(pisdf::Vertex *vertex);

        ~ScheduleTask();

        ScheduleTask(ScheduleTask &&) = default;

        ScheduleTask(const ScheduleTask &) = delete;

        ScheduleTask &operator=(ScheduleTask &&) = default;

        ScheduleTask &operator=(const ScheduleTask &) = delete;

        /* === Method(s) === */

        /**
         * @brief Set all notification flags to true.
         */
        void enableBroadcast();

        /**
         * @brief Export this task to xml format.
         * @param file  File to write to.
         */
        std::string name() const;

        /**
         * @brief Return a color value for the task.
         *        format is RGB with 8 bits per component in the lower part of the returned value.
         * @return  color of the task.
         */
        u32 color() const;

        /**
         * @brief Creates a job message out of the information of the task.
         * @return  JobMessage.
         */
        JobMessage createJobMessage() const;

        /* === Getter(s) === */

        /**
         * @brief Get the array of task dependencies.
         * @return const reference to the array of dependencies.
         */
        inline const bool *notificationFlags() const {
            return notificationFlags_.get();
        }

        /**
         * @brief Get the array of task dependencies.
         * @return const reference to the array of dependencies.
         */
        inline const array<ScheduleTask *> &dependencies() const {
            return dependenciesArray_;
        }

        /**
         * @brief Get the task execution constraint the task has on a given LRT.
         * @warning There are no checks perform on the the value of the index.
         * @param lrt Index of the LRT.
         * @return task ix of the constraint, -1 else.
         */
        inline i32 executionConstraint(size_t lrt) const {
            return executionConstraints_.get()[lrt];
        }

        /**
         * @brief Get the start time of the task.
         * @return mapping start time of the task, UINT64_MAX else
         */
        inline u64 startTime() const {
            return startTime_;
        }

        /**
         * @brief Get the end time of the task.
         * @return mapping end time of the task, UINT64_MAX else
         */
        inline u64 endTime() const {
            return endTime_;
        }

        /**
         * @brief Returns the LRT virtual ix on which the task is mapped.
         * @return LRT virtual ix
         */
        inline size_t mappedLrt() const {
            return mappedLrt_;
        }

        /**
         * @brief Returns the PE virtual ix on which the task is mapped.
         * @return PE virtual ix
         */
        inline size_t mappedPe() const {
            return mappedPe_;
        }

        /**
         * @brief Returns the ix of the task in the schedule.
         * @return ix of the task in the schedule, -1 else.
         */
        inline i32 ix() const {
            return ix_;
        }

        /**
         * @brief Returns the execution ix of the task in the schedule.
         * @return execution ix of the task in the schedule, -1 else.
         */
        inline i32 execIx() const {
            return execIx_;
        }

        /**
         * @brief Returns the state of the task.
         * @return @refitem TaskState of the task
         */
        inline TaskState state() const {
            return state_;
        }

        /**
         * @brief Returns the type of the task.
         * @return @refitem TaskType of the task
         */
        inline TaskType type() const {
            return type_;
        }

        /**
         * @brief Get the vertex associated to this task (if type() == TaskType::VERTEX).
         * @return pointer to the associated vertex, nullptr else.
         */
        pisdf::Vertex *vertex() const;

        /**
         * @brief Get the communication task information associated to this task (if type() != TaskType::VERTEX).
         * @return pointer to the associated information, nullptr else.
         */
        ComTaskInformation *comTaskInfo() const;

        /**
         * @brief Get the ix of the kernel associated with this task.
         * @return ix of the kernel of the associated vertex if type() == TaskType::VERTEX,
         *         ix of the kernel set by the method @refitem ScheduleTask::setKernelIx
         */
        size_t kernelIx() const;

        /**
         * @brief Gets the @refitem TaskMemory associated with this task.
         * @return pointer to the memory (input / output fifos) of the Task.
         */
        inline TaskMemory *taskMemory() const {
            return taskMemory_.get();
        }

        /**
         * @brief Get the input fifo at index ix of the task.
         * @param ix Index of the fifo.
         * @return empty fifo if no @refitem TaskMemory is attached to this task, corresponding input fifo else.
         * @throws std::out_of_range
         */
        RTFifo getInputFifo(size_t ix) const;

        /**
         * @brief Get the output fifo at index ix of the task.
         * @param ix Index of the fifo.
         * @return empty fifo if no @refitem TaskMemory is attached to this task, corresponding output fifo else.
         * @throws std::out_of_range
         */
        RTFifo getOutputFifo(size_t ix) const;

        /* === Setter(s) === */

        /**
         * @brief Set the number of dependencies of this task.
         * @remark If this task already has dependencies, nothing happens.
         * @param count Number of dependencies to set.
         */
        void setNumberOfDependencies(size_t count);

        /**
         * @brief Set the task dependency at given position.
         * @param task Pointer to the task to set (if nullptr, nothing happens).
         * @param pos  Position of the task to set.
         */
        void setDependency(ScheduleTask *task, size_t pos);

        /**
         * @brief Set the job constraint on a given lrt.
         * @warning There is no check on the value of lrt.
         * @param lrt  Ix of the lrt.
         * @param job  Ix of the job.
         */
        inline void setExecutionConstraint(size_t lrt, i32 job) {
            executionConstraints_.get()[lrt] = job;
        }

        /**
         * @brief Set the notification flag for this lrt.
         * @warning There is no check on the value of lrt.
         * @param lrt   Index of the lrt.
         * @param value Value to set: true = should notify, false = should not notify.
         */
        inline void setNotificationFlag(size_t lrt, bool value) {
            notificationFlags_.get()[lrt] = value;
        }

        /**
         * @brief Set the start time of the job.
         * @remark This method will overwrite current value.
         * @param time  Value to set.
         */
        inline void setStartTime(u64 time) {
            startTime_ = time;
        }

        /**
         * @brief Set the end time of the job.
         * @remark This method will overwrite current value.
         * @param time  Value to set.
         */
        inline void setEndTime(u64 time) {
            endTime_ = time;
        }

        /**
        * @brief Set the processing element of the job.
        * @remark This method will overwrite current values.
        * @param mappedLrt  Lrt ix inside spider.
        */
        inline void setMappedLrt(size_t mappedLrt) {
            mappedLrt_ = mappedLrt;
        }

        /**
        * @brief Set the processing element of the job.
        * @remark This method will overwrite current values.
        * @param mappedPE  Lrt ix inside spider.
        */
        inline void setMappedPE(size_t mappedPE) {
            mappedPe_ = mappedPE;
        }

        /**
         * @brief Set the ix of the job.
         * @remark This method will overwrite current value.
         * @param ix Ix to set.
         */
        inline void setIx(int32_t ix) {
            ix_ = ix;
        }

        /**
        * @brief Set the execution ix of the task (that will be used for synchronization).
        * @remark This method will overwrite current values.
         * @param execIx Ix to set.
        */
        inline void setExecIx(i32 execIx) {
            execIx_ = execIx;
        }

        /**
         * @brief Set the state of the job.
         * @remark This method will overwrite current value.
         * @param state State to set.
         */
        inline void setState(TaskState state) {
            state_ = state;
        }

        /**
         * @brief Set the value of the internal data pointer (@refitem pisdf::Vertex or @refitem ComTaskInformation).
         * @param information  Pointer to the information to set.
         */
        void setInternal(void *information);

        /**
         * @brief Set the internal task memory of the ScheduleTask and replace current one.
         * @param taskMemory  Unique pointer to the task memory.
         */
        void setTaskMemory(spider::unique_ptr<TaskMemory> taskMemory);

        /**
         * @brief Set the successor of a send task (should be a receive task).
         * @param task pointer to the task to set.
         */
        void setSendSuccessor(ScheduleTask *task);

    protected:
        spider::array<ScheduleTask *> dependenciesArray_;
        spider::unique_ptr<TaskMemory> taskMemory_;
        spider::unique_ptr<i32> executionConstraints_;
        spider::unique_ptr<bool> notificationFlags_;
        void *internal_{ nullptr };
        u64 startTime_{ UINT64_MAX };
        u64 endTime_{ UINT64_MAX };
        size_t mappedLrt_{ SIZE_MAX };
        size_t mappedPe_{ SIZE_MAX };
        i32 execIx_{ -1 };
        i32 ix_{ -1 };
        TaskState state_{ TaskState::NOT_SCHEDULABLE };
        TaskType type_;

        /* === Private method(s) === */

        /**
         * @brief Set JobMessage input parameters (if any).
         * @param message Reference to the jobMessage to update.
         */
        void setJobMessageInputParameters(JobMessage &message) const;
    };
}
#endif //SPIDER2_SCHEDULETASK_H
