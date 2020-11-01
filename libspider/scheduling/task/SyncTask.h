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
#ifndef SPIDER2_SYNCTASK_H
#define SPIDER2_SYNCTASK_H

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

        class SyncTask final : public Task {
        public:
            explicit SyncTask(SyncType type, const MemoryBus *bus);

            ~SyncTask() noexcept final = default;

            /* === Method(s) === */

            void visit(TaskLauncher *launcher) final;

            /* === Getter(s) === */

            inline Task *previousTask(size_t, const Schedule *) const final { return dependency_; }

            inline Task *nextTask(size_t, const Schedule *) const final { return successor_; }

            inline u32 color() const final {
                /* ==  SEND    -> vivid tangerine color == */
                /* ==  RECEIVE -> Studio purple color == */
                return type_ == SyncType::SEND ? 0xff9478 : 0x8e44ad;
            }

            inline std::string name() const final { return type_ == SyncType::SEND ? "send" : "receive"; }

            inline u32 ix() const noexcept final { return ix_; }

            u64 timingOnPE(const PE *) const final;

            inline size_t dependencyCount() const final { return 1u; }

            inline size_t successorCount() const final { return 1u; }

            inline SyncType syncType() const { return type_; }

            inline u32 getDepIx() const { return depIx_; }

            inline const MemoryBus *getMemoryBus() const { return bus_; }

            /* === Setter(s) === */

            /**
             * @brief Set the task succeeding to this task.
             * @param successor pointer to the successor.
             */
            inline void setSuccessor(Task *task) {
                if (task) {
                    successor_ = task;
                }
            }

            /**
             * @brief Set the task succeeding to this task.
             * @param successor pointer to the successor.
             */
            inline void setPredecessor(Task *task) {
                if (task) {
                    dependency_ = task;
                }
            }

            inline void setIx(u32 ix) noexcept final { ix_ = ix; }

            inline void setDepIx(u32 depIx) { depIx_ = depIx; }

        private:
            Task *successor_{ nullptr };      /*!< Successor task */
            Task *dependency_{ nullptr };     /*!< Successor task */
            const MemoryBus *bus_{ nullptr }; /*!< Memory bus used by the task */
            u32 depIx_ = UINT32_MAX;
            u32 ix_ = UINT32_MAX;
            SyncType type_;
        };
    }
}

#endif //SPIDER2_SYNCTASK_H
