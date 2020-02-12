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
#ifndef SPIDER2_SCHEDULETASK_H
#define SPIDER2_SCHEDULETASK_H

/* === Include(s) === */

#include <memory/unique_ptr.h>
#include <containers/array.h>
#include <common/Types.h>

namespace spider {

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

    /* === Class definition === */

    class ScheduleTask {
    public:
        explicit ScheduleTask(TaskType type);

        virtual ~ScheduleTask() = default;

        /* === Method(s) === */

        /**
         * @brief Set all notification flags to true.
         */
        void enableBroadcast();

        /**
         * @brief Export this task to xml format.
         * @param file  File to write to.
         */
        virtual std::string name() const;

        /* === Getter(s) === */

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
            return mappedLRT_;
        }

        /**
         * @brief Returns the PE virtual ix on which the task is mapped.
         * @return PE virtual ix
         */
        inline size_t mappedPE() const {
            return mappedPE_;
        }

        /**
         * @brief Returns the ix of the task in the schedule.
         * @return ix of the task in the schedule, SIZE_MAX else.
         */
        inline i32 ix() const {
            return ix_;
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
            mappedLRT_ = mappedLrt;
        }

        /**
        * @brief Set the processing element of the job.
        * @remark This method will overwrite current values.
        * @param mappedPE  Lrt ix inside spider.
        */
        inline void setMappedPE(size_t mappedPE) {
            mappedPE_ = mappedPE;
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
         * @brief Set the state of the job.
         * @remark This method will overwrite current value.
         * @param state State to set.
         */
        inline void setState(TaskState state) {
            state_ = state;
        }

    protected:
        array<ScheduleTask *> dependenciesArray_;
        unique_ptr <i32> executionConstraints_;
        unique_ptr<bool> notificationFlags_;
        u64 startTime_ = UINT64_MAX;
        u64 endTime_ = UINT64_MAX;
        size_t mappedLRT_ = SIZE_MAX;
        size_t mappedPE_ = SIZE_MAX;
        i32 ix_ = -1;
        TaskState state_ = TaskState::NOT_SCHEDULABLE;
        TaskType type_;
    };
}
#endif //SPIDER2_SCHEDULETASK_H
