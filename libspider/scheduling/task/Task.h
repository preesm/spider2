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
#include <scheduling/memory/AllocatedFifos.h>
#include <scheduling/memory/AllocationRule.h>
#include <runtime/message/JobMessage.h>

namespace spider {
    /* === Forward Declaration(s) === */

    class PE;

    namespace sched {

        class Task;

        class Schedule;

        namespace detail {
            struct MappingInfo {
                u64 startTime_{ UINT64_MAX };
                u64 endTime_{ UINT64_MAX };
                const PE *mappedPE_{ nullptr };
            };

            struct ExecInfo {
                spider::unique_ptr<Task *> dependencies_;
                spider::unique_ptr<bool> notifications_;
                spider::unique_ptr<size_t> constraints_;
            };
        }

        enum TaskState : u8 {
            NOT_SCHEDULABLE = 0,
            NOT_RUNNABLE,
            PENDING,
            READY,
            RUNNING,
        };

        /* === Class definition === */

        class Task {
        public:
            Task();

            virtual ~Task() noexcept = default;

            Task(Task &&) noexcept = default;

            Task &operator=(Task &&) noexcept = default;

            Task(const Task &) = delete;

            Task &operator=(const Task &) noexcept = delete;

            /* === Method(s) === */

            /**
             * @brief Return the memory allocation rule for a given input fifo.
             * @param ix Index of the Fifo.
             * @return @refitem AllocationRule
             * @throws @refitem spider::Exception if index out of bound (only in debug)
             */
            virtual AllocationRule allocationRuleForInputFifo(size_t ix) const = 0;


            /**
             * @brief Return the memory allocation rule for a given output fifo.
             * @param ix Index of the Fifo.
             * @return @refitem AllocationRule
             * @throws @refitem spider::Exception if index out of bound (only in debug)
             */
            virtual AllocationRule allocationRuleForOutputFifo(size_t ix) const = 0;

            /**
             * @brief Get the previous task of a given index.
             * @param ix Index of the task.
             * @return pointer to the previous task, nullptr else.
             * @throws @refitem spider::Exception if index out of bound (only in debug)
             */
            virtual Task *previousTask(size_t ix) const = 0;

            /**
             * @brief Return a color value for the task.
             *        format is RGB with 8 bits per component in the lower part of the returned value.
             * @return  color of the task.
             */
            virtual u32 color() const = 0;

            /**
             * @brief Update task execution dependencies based on schedule information.
             * @param schedule pointer to the schedule.
             */
            virtual void updateTaskExecutionDependencies(const Schedule *schedule) = 0;

            /**
             * @brief Returns the name of the task
             * @return name of the task
             */
            virtual std::string name() const = 0;

            /**
             * @brief Update execution constraints based on task dependencies.
             */
            virtual void updateExecutionConstraints() = 0;

            /**
             * @brief Creates a job message out of the information of the task.
             * @return  JobMessage.
             */
            virtual JobMessage createJobMessage() const = 0;

            /**
             * @brief Set all notification flags to true.
             */
            void enableBroadcast();

            /* === Getter(s) === */

            inline AllocatedFifos &fifos() const {
#ifndef NDEBUG
                if (!fifos_) {
                    throwSpiderException("Nullptr TaskFifos.");
                }
#endif
                return *(fifos_.get());
            }

            /**
             * @brief Get the start time of the task.
             * @return mapping start time of the task, UINT64_MAX else
             */
            u64 startTime() const;

            /**
             * @brief Get the end time of the task.
             * @return mapping end time of the task, UINT64_MAX else
             */
            u64 endTime() const;

            /**
             * @brief Returns the PE on which the task is mapped.
             * @return pointer to the PE onto which the task is mapped, nullptr else
             */
            const PE *mappedPe() const;

            /**
             * @brief Returns the LRT attached to the PE on which the task is mapped.
             * @return pointer to the LRT, nullptr else
             */
            const PE *mappedLRT() const;

            /**
             * @brief Returns the state of the task.
             * @return @refitem TaskState of the task
             */
            inline TaskState state() const noexcept { return state_; }

            /**
             * @brief Returns the ix of the task in the schedule.
             * @return ix of the task in the schedule, -1 else.
             */
            inline u32 ix() const noexcept { return ix_; }

            /**
             * @brief Returns the executable job index value of the task in the job queue of the mapped PE.
             * @return ix value, SIZE_MAX else.
             */
            inline u32 jobExecIx() const noexcept { return jobExecIx_; }

            /* === Setter(s) === */

            /**
             * @brief Set the start time of the job.
             * @remark This method will overwrite current value.
             * @param time  Value to set.
             */
            void setStartTime(u64 time);

            /**
             * @brief Set the end time of the job.
             * @remark This method will overwrite current value.
             * @param time  Value to set.
             */
            void setEndTime(u64 time);

            /**
            * @brief Set the processing element of the job.
            * @remark This method will overwrite current values.
            * @param mappedPE  Lrt ix inside spider.
            */
            void setMappedPE(const PE *pe);

            /**
             * @brief Set the state of the job.
             * @remark This method will overwrite current value.
             * @param state State to set.
             */
            inline void setState(TaskState state) noexcept { state_ = state; }

            /**
             * @brief Set the ix of the job.
             * @remark This method will overwrite current value.
             * @param ix Ix to set.
             */
            virtual inline void setIx(u32 ix) noexcept { ix_ = ix; }

            /**
             * @brief Override the current execution dependency at given position.
             * @param ix   position of the dependency to set.
             * @param task pointer to the task to set.
             * @throws @refitem spider::Exception if index out of bound (only in debug)
             */
            virtual void setExecutionDependency(size_t ix, Task *task) = 0;

            /**
             * @brief Set the execution job index value of the task (that will be used for synchronization).
             * @remark This method will overwrite current values.
             * @param ix Ix to set.
             */
            inline void setJobExecIx(u32 ix) noexcept { jobExecIx_ = ix; }

            /**
             * @brief Set the notification flag for this lrt.
             * @warning There is no check on the value of lrt.
             * @param lrt   Index of the lrt.
             * @param value Value to set: true = should notify, false = should not notify.
             */
            inline void setNotificationFlag(size_t lrt, bool value) {
                execInfo_.notifications_.get()[lrt] = value;
            }

        protected:
            detail::ExecInfo execInfo_;                            /*!< Execution information (constraints and notifs) */
            std::shared_ptr<AllocatedFifos> fifos_;                     /*!< Fifo(s) attached to the task */
            spider::unique_ptr<detail::MappingInfo> mappingInfo_;  /*!< Mapping information of the task */
            u32 ix_{ UINT32_MAX };                                 /*!< Index of the task in the schedule */
            u32 jobExecIx_{ UINT32_MAX };                          /*!< Index of the job sent to the PE */
            TaskState state_{ NOT_SCHEDULABLE };                   /*!< State of the task */
        };
    }
}

#endif //SPIDER2_TASK_H
