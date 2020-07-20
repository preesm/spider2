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
#include <scheduling/task/TaskFifos.h>

namespace spider {
    /* === Forward Declaration(s) === */

    class PE;

    namespace sched {
        namespace detail {
            struct MappingInfo {
                u64 startTime_{ UINT64_MAX };
                u64 endTime_{ UINT64_MAX };
                const PE *mappedPE_{ nullptr };
            };
        }

        enum TaskState : u8 {
            NOT_SCHEDULABLE = 0,
            NOT_RUNNABLE,
            PENDING,
            READY,
            RUNNING,
        };

        enum TaskType : u8 {
            VERTEX = 0,
            SYNC_SEND,
            SYNC_RECEIVE,
        };

        /* === Class definition === */

        class Task {
        public:
            Task();

            ~Task() = default;

            /* === Method(s) === */

            /* === Getter(s) === */

            inline TaskFifos &fifos() const {
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
             * @brief Returns the PE virtual ix on which the task is mapped.
             * @return pointer to the PE onto which the task is mapped, nullptr else
             */
            const PE *mappedPe() const;

            /**
             * @brief Returns the state of the task.
             * @return @refitem TaskState of the task
             */
            inline TaskState state() const { return state_; }

            /**
             * @brief Returns the type of the task.
             * @return @refitem TaskType of the task
             */
            inline TaskType type() const { return type_; }

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
            void setMappedPE(const PE *const pe);

            /**
             * @brief Set the state of the job.
             * @remark This method will overwrite current value.
             * @param state State to set.
             */
            inline void setState(TaskState state) { state_ = state; }

        private:
            std::shared_ptr<TaskFifos> fifos_;
            spider::unique_ptr<detail::MappingInfo> mappingInfo_;
            TaskState state_{ NOT_SCHEDULABLE };
            TaskType type_{ VERTEX };
        };
    }
}

#endif //SPIDER2_TASK_H
