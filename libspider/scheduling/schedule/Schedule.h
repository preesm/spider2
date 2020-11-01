/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
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
#ifndef SPIDER2_SCHEDULE_H
#define SPIDER2_SCHEDULE_H

/* === Include(s) === */

#include <containers/vector.h>
#include <memory/unique_ptr.h>
#include <scheduling/schedule/ScheduleStats.h>
#include <scheduling/task/Task.h>

namespace spider {

    namespace sched {

        class FifoAllocator;

        /* === Class definition === */

        class Schedule {
        public:
            Schedule() : tasks_{ factory::vector<spider::unique_ptr<sched::Task>>(StackID::SCHEDULE) } {

            };

            ~Schedule() = default;

            Schedule(const Schedule &) = delete;

            Schedule &operator=(const Schedule &) = delete;

            Schedule(Schedule &&) = default;

            Schedule &operator=(Schedule &&) = default;

            /* === Method(s) === */

            /**
             * @brief Reserve memory for task insertion.
             * @param size Size to reserve.
             */
            void reserve(size_t size);

            /**
             * @brief Clear schedule tasks.
             */
            void clear();

            /**
             * @brief Reset schedule tasks.
             * @remark Set all vertexTask state to @refitem sched::State::PENDING.
             * @remark Statistics of the platform are not modified.
             */
            void reset();

            /**
             * @brief Updates a vertexTask information and set its state as JobState::READY
             * @param task      Pointer to the vertexTask.
             * @param slave     Slave (cluster and pe) to execute on.
             * @param startTime Start time of the vertexTask.
             * @param endTime   End time of the vertexTask.
             * @throw std::out_of_range if bad ix.
             */
            void updateTaskAndSetReady(sched::Task *task, const PE *slave, u64 startTime, u64 endTime);

            /**
             * @brief Add a task to the schedule
             * @remark Once added, memory of the task is handled by the schedule, DO NOT FREE it yourself.
             * @param task  Pointer to the task.
             */
            inline void addTask(sched::Task *task) {
                task->setIx(static_cast<u32>(tasks_.size()));
                tasks_.emplace_back(task);
            }

            /* === Getter(s) === */

            inline spider::vector<spider::unique_ptr<Task>> &tasks() {
                return tasks_;
            }

            inline const spider::vector<spider::unique_ptr<Task>> &tasks() const {
                return tasks_;
            }

            inline sched::Task *task(size_t ix) const {
                if (ix >= tasks_.size()) {
                    return nullptr;
                }
                return tasks_.at(ix).get();
            }

            /**
             * @brief Get the different statistics of the platform.
             * @return const reference to @refitem Stats
             */
            inline const Stats &stats() const { return stats_; }

            /**
             * @brief Return the scheduled start time of a given PE.
             * @param ix  PE to check.
             * @return start time of given PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline uint64_t startTime(size_t ix) const {
                return stats().startTime(ix);
            }

            /**
             * @brief Return the scheduled end time of a given PE.
             * @param ix  PE to check.
             * @return end time of given PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline uint64_t endTime(size_t ix) const {
                return stats().endTime(ix);
            }

            /**
             * @brief Get the number of vertexTask in the schedule (including already launched tasks).
             * @return number of tasks in the schedule.
             */
            inline size_t taskCount() const {
                return tasks_.size();
            }

        private:
            spider::vector<spider::unique_ptr<sched::Task>> tasks_;
            Stats stats_;
        };
    }
}
#endif //SPIDER2_SCHEDULE_H
