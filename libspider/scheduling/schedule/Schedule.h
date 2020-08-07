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
#ifndef SPIDER2_SCHEDULE_H
#define SPIDER2_SCHEDULE_H

/* === Include(s) === */

#include <containers/vector.h>
#include <memory/unique_ptr.h>
#include "ScheduleStats.h"

namespace spider {

    namespace sched {

        class Task;

        /* === Class definition === */

        class Schedule {
        public:
            Schedule() : tasks_{ factory::vector<spider::unique_ptr<Task>>(StackID::SCHEDULE) } { };

            ~Schedule() = default;

            /* === Method(s) === */

            /**
             * @brief Clear schedule tasks.
             */
            void clear();

            /**
             * @brief Reset schedule tasks.
             * @remark Set all task state to @refitem TaskState::PENDING.
             * @remark Statistics of the platform are not modified.
             */
            void reset();

            /**
             * @brief Add a new schedule task to the schedule.
             * @remark if task has an index >= 0, nothing happens.
             * @remark if task is nullptr, nothing happens.
             * @param task Pointer to the task.
             */
            void addTask(spider::unique_ptr<Task> task);

            /**
             * @brief Updates a task information and set its state as JobState::READY
             * @param task      Pointer to the task.
             * @param slave     Slave (cluster and pe) to execute on.
             * @param startTime Start time of the task.
             * @param endTime   End time of the task.
             * @throw std::out_of_range if bad ix.
             */
            void updateTaskAndSetReady(Task *task, size_t slave, u64 startTime, u64 endTime);

            /**
             * @brief Send every tasks currently in JobState::READY.
             */
            void sendReadyTasks();

            /* === Getter(s) === */

            /**
             * @brief Get the list of scheduled tasks, obtained after the call to Scheduler::schedule method.
             * @return const reference to a vector of pointer to Task.
             */
            inline const spider::vector<spider::unique_ptr<Task>> &tasks() const { return tasks_; }

            inline const Stats &stats() const { return stats_; }

            /**
             * @brief Get the number of task in the schedule (including already launched tasks).
             * @return number of tasks in the schedule.
             */
            inline size_t taskCount() const {
                return tasks_.size();
            }

            /* === Setter(s) === */

        private:
            spider::vector<spider::unique_ptr<Task>> tasks_;
            spider::vector<Task *> readyTaskVector_;
            Stats stats_;
        };
    }
}
#endif //SPIDER2_SCHEDULE_H
