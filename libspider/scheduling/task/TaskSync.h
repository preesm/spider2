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
#ifndef SPIDER2_TASKSYNC_H
#define SPIDER2_TASKSYNC_H

/* === Include(s) === */

#include <scheduling/task/Task.h>

namespace spider {

    class MemoryBus;

    namespace sched {

        enum SyncType {
            SEND,
            RECEIVE
        };

        /* === Class definition === */

        class TaskSync final : public Task {
        public:
            explicit TaskSync(SyncType type);

            ~TaskSync() noexcept override = default;

            /* === Method(s) === */

            AllocationRule allocationRuleForInputFifo(size_t ix) const override;

            AllocationRule allocationRuleForOutputFifo(size_t ix) const override;

            Task *previousTask(size_t ix) const override;

            u32 color() const override;

            inline void updateTaskExecutionDependencies(const Schedule *) override { }

            /* === Getter(s) === */

            /* === Setter(s) === */

            /**
             * @brief Set the task succeeding to this task.
             * @param successor pointer to the successor.
             */
            void setSuccessor(Task *successor);

            /**
             * @brief Set the data size (in bytes) to send / receive.
             * @param size Size of the data to send / receive
             */
            void setSize(size_t size);

            /**
             * @brief Set the index of the output port of the previous task
             * @param ix index.
             */
            void setInputPortIx(u32 ix);

            /**
             * @brief Sets the memory bus attached to this synchronization task.
             * @param bus pointer to the memory bus.
             */
            inline void setMemoryBus(const MemoryBus *bus) {
                if (bus) {
                    bus_ = bus;
                }
            }

            void setExecutionDependency(size_t ix, Task *task) override;

        private:
            Task *successor_{ nullptr };
            const MemoryBus *bus_{ nullptr };
            size_t size_{ 0U };
            u32 inputPortIx_{ 0U };
            SyncType type_;
        };
    }
}

#endif //SPIDER2_TASKSYNC_H
