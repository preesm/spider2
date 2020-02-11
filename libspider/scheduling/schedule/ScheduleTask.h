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

        ~ScheduleTask() = default;

        /* === Method(s) === */

        /**
         * @brief Set all notification flags to true.
         */
        void enableBroadcast();

        /* === Getter(s) === */

        inline array<ScheduleTask *> dependencies() const {
            return dependenciesArray_;
        }

        inline int32_t executionConstraint(size_t lrt) const {
            return executionConstraints_.get()[lrt];
        }

        inline uint64_t startTime() const {
            return startTime_;
        }

        inline uint64_t endTime() const {
            return endTime_;
        }

        inline size_t mappedLrt() const {
            return mappedLRT_;
        }

        inline size_t mappedPE() const {
            return mappedPE_;
        }

        inline int32_t ix() const {
            return ix_;
        }

        inline TaskState state() const {
            return state_;
        }

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
        inline void setExecutionConstraint(size_t lrt, int32_t job) const {
            executionConstraints_.get()[lrt] = job;
        }

        /**
         * @brief Set the start time of the job.
         * @remark This method will overwrite current value.
         * @param time  Value to set.
         */
        inline void setStartTime(uint64_t time) {
            startTime_ = time;
        }

        /**
         * @brief Set the end time of the job.
         * @remark This method will overwrite current value.
         * @param time  Value to set.
         */
        inline void setEndTime(uint64_t time) {
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
        unique_ptr <int32_t> executionConstraints_;
        unique_ptr<bool> notificationFlags_;
        uint64_t startTime_ = SIZE_MAX;
        uint64_t endTime_ = SIZE_MAX;
        size_t mappedLRT_ = SIZE_MAX;
        size_t mappedPE_ = SIZE_MAX;
        int32_t ix_ = -1;
        TaskState state_ = TaskState::NOT_SCHEDULABLE;
        TaskType type_;
    };
}
#endif //SPIDER2_SCHEDULETASK_H
