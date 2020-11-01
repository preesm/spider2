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
#ifndef SPIDER2_TASKLAUNCHER_H
#define SPIDER2_TASKLAUNCHER_H

/* === Include(s) === */

#include <memory/memory.h>
#include <common/Types.h>
#include <scheduling/memory/JobFifos.h>
#include <runtime/message/JobMessage.h>
#include <graphs-tools/numerical/dependencies.h>

namespace spider {
    namespace sched {

        class Task;

        class PiSDFTask;

        class SRDAGTask;

        class SyncTask;

        class Schedule;

        class FifoAllocator;

        /* === Class definition === */

        class TaskLauncher {
        public:
            explicit TaskLauncher(const Schedule *schedule, FifoAllocator *allocator) : schedule_{ schedule },
                                                                                        allocator_{ allocator } {
                deferedSyncTasks_ = factory::vector<std::pair<SyncTask *, u32>>(StackID::RUNTIME);
            }

            ~TaskLauncher() = default;

            /* === Method(s) === */

            inline void visit(sched::Task *) { }

#ifndef _NO_BUILD_LEGACY_RT

            void visit(sched::SRDAGTask *task);

#endif

            void visit(sched::SyncTask *task);

            void visit(sched::PiSDFTask *task);

        private:
            spider::vector<std::pair<SyncTask *, u32>> deferedSyncTasks_;
            const Schedule *schedule_ = nullptr;
            FifoAllocator *allocator_ = nullptr;

            /* === Private method(s) === */

            void sendTask(Task *task, JobMessage &message);

            void sendSyncTask(SyncTask *task, const JobMessage &message);

            /**
             * @brief Build the notification flags for this task.
             * @param task Pointer to the task.
             * @return array of only true booleans if this task need to broadcast its job stamp,
             *         empty array if no notifications are required,
             *         array with corresponding flags for each LRT else.
             */
            template<class ...Args>
            spider::unique_ptr<bool> buildJobNotificationFlags(const Task *task, Args &&...args) const;

            /**
             * @brief Build execution constraints for this task (needed job + lrt).
             * @param task Pointer to the task.
             * @return array of constraints if any, empty array else.
             */
            static spider::array<SyncInfo> buildExecConstraints(const Task *task);

            /**
             * @brief Based on current state of the mapping / scheduling, fill the boolean array "flags" with
             *        true if given LRT is to be notified, false else.
             * @param flags    Array of boolean, should be EXACTLY of size archi::platform()->LRTCount();
             * @param schedule Pointer to the schedule.
             * @return True if at least one LRT will be notified by this task.
             */
            void updateNotificationFlags(const Task *task, bool *flags) const;

            void updateNotificationFlags(const Task *task,
                                         bool *flags,
                                         const spider::vector<pisdf::DependencyIterator> &consDeps) const;

            static bool setFlagsFromSink(const Task *task, const Task *sinkTask, bool *flags);

        };
    }
}
#endif //SPIDER2_TASKLAUNCHER_H
