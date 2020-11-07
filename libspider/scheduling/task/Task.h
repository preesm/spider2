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
#ifndef SPIDER2_TASK_H
#define SPIDER2_TASK_H

/* === Include(s) === */

#include <common/Types.h>
#include <containers/array.h>
#include <string>

namespace spider {

    class PE;

    namespace sched {

        class Task;

        class SyncTask;

        class Schedule;

        class TaskLauncher;

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

            virtual void visit(TaskLauncher *launcher) = 0;

            virtual inline void setOnFiring(u32) { }

            /**
             * @brief Update output params based on received values.
             * @param values Values of the params.
             */
            virtual bool receiveParams(const spider::array<i64> &values) = 0;

            /* === Getter(s) === */

            /**
             * @brief Get the input rate for the fifo of index ix.
             * @param ix  Index of the input fifo.
             * @return rate of the fifo.
             */
            virtual i64 inputRate(size_t ix) const = 0;

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
            virtual bool isMappableOnPE(const PE *pe) const = 0;

            /**
             * @brief Get the execution timing on a given PE.
             * @param pe  Pointer to the PE.
             * @return exec timing on the PE, UINT64_MAX else.
             */
            virtual u64 timingOnPE(const PE *pe) const = 0;

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
            virtual u64 startTime() const = 0;

            /**
             * @brief Get the end time of the vertexTask.
             * @return mapping end time of the vertexTask, UINT64_MAX else
             */
            virtual u64 endTime() const = 0;

            /**
             * @brief Returns the PE on which the vertexTask is mapped.
             * @return pointer to the PE onto which the vertexTask is mapped, nullptr else
             */
            virtual const PE *mappedPe() const = 0;

            /**
             * @brief Returns the LRT attached to the PE on which the vertexTask is mapped.
             * @return pointer to the LRT, nullptr else
             */
            virtual const PE *mappedLRT() const = 0;

            /**
             * @brief Returns the state of the vertexTask.
             * @return @refitem TaskState of the vertexTask
             */
            virtual TaskState state() const noexcept = 0;

            /**
             * @brief Returns the ix of the vertexTask in the schedule.
             * @return ix of the vertexTask in the schedule, -1 else.
             */
            virtual u32 ix() const noexcept = 0;

            /**
             * @brief Returns the executable job index value of the vertexTask in the job queue of the mapped PE.
             * @return ix value, SIZE_MAX else.
             */
            virtual u32 jobExecIx() const noexcept = 0;

            /**
             * @brief Get the dependency task ix on given LRT and for a given firing.
             * @param lrtIx    Index of the LRT to evaluate.
             * @return value of the task ix, UINT32_MAX else.
             */
            virtual u32 syncExecIxOnLRT(size_t lrtIx) const = 0;

            /**
             * @brief Get the exchanged rate with given LRT.
             * @param lrtIx  Index of the LRT.
             * @return value of the rate, 0 else.
             */
            virtual u32 syncRateOnLRT(size_t lrtIx) const = 0;

            virtual inline u32 firing() const { return 0; }

            /* === Setter(s) === */

            /**
             * @brief Set the start time of the job.
             * @remark This method will overwrite current value.
             * @param time  Value to set.
             */
            virtual void setStartTime(u64 time) = 0;

            /**
             * @brief Set the end time of the job.
             * @remark This method will overwrite current value.
             * @param time  Value to set.
             */
            virtual void setEndTime(u64 time) = 0;

            /**
            * @brief Set the processing element of the job.
            * @remark This method will overwrite current values.
            * @param mappedPE  Lrt ix inside spider.
            */
            virtual void setMappedPE(const PE *pe) = 0;

            /**
             * @brief Set the state of the job.
             * @remark This method will overwrite current value.
             * @param state State to set.
             */
            virtual void setState(TaskState state) noexcept = 0;

            /**
             * @brief Set the execution job index value of the vertexTask (that will be used for synchronization).
             * @remark This method will overwrite current values.
             * @param ix Ix to set.
             */
            virtual void setJobExecIx(u32 ix) noexcept = 0;

            /**
             * @brief Set the ix of the job.
             * @remark This method will overwrite current value.
             * @param ix Ix to set.
             */
            virtual void setIx(u32 ix) noexcept = 0;

            /**
            * @brief Set the execution job index value of the Task that will be used for synchronization.
            * @remark This method will overwrite current value only if value is greater than existing value.
            * @param lrtIx Index of the LRT of the sync task.
            * @param value Value of the job exec ix of the sync task.
            */
            virtual void setSyncExecIxOnLRT(size_t lrtIx, u32 value) = 0;

            /**
            * @brief Set the execution rate of the Task that will be used for synchronization.
            * @remark This method will overwrite current value.
            * @param lrtIx Index of the LRT of the sync task.
            * @param value Value of the rate of the sync task.
            */
            virtual void setSyncRateOnLRT(size_t lrtIx, u32 value) = 0;

        protected:
            using SyncInfo = std::pair<u32, u32>;
        };
    }
}

#endif //SPIDER2_TASK_H
