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
#ifndef SPIDER2_NOSYNCFIFOALLOCATOR_H
#define SPIDER2_NOSYNCFIFOALLOCATOR_H
#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <scheduling/memory/FifoAllocator.h>

namespace spider {
    namespace sched {

        /* === Class definition === */

        class NoSyncFifoAllocator final : public FifoAllocator {
        public:
            NoSyncFifoAllocator() : FifoAllocator({ false, true }) { }

            ~NoSyncFifoAllocator() noexcept override = default;

            /* === Method(s) === */

            /* === Getter(s) === */

            /**
             * @brief Get the type of the FifoAllocator
             * @return @refitem FifoAllocatorType
             */
            FifoAllocatorType type() const override { return FifoAllocatorType::DEFAULT_NOSYNC; };

            /* === Setter(s) === */

        private:

            spider::Fifo
            allocateDefaultVertexInputFifo(sched::VertexTask *task, const srdag::Edge *edge) const override;

            void allocateForkTask(sched::VertexTask *task) const override;

            void allocateDuplicateTask(sched::VertexTask *task) const override;

            static void updateForkDuplicateInputTask(sched::VertexTask *task);

            static void updateFifoCount(const sched::Task *task, const sched::Task *inputTask, u32 count);

            static bool replaceInputTask(sched::Task *task, const sched::Task *inputTask, size_t ix);
        };
    }
}
#endif
#endif //SPIDER2_NOSYNCFIFOALLOCATOR_H
