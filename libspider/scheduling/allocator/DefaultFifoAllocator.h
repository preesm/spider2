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
#ifndef SPIDER2_DEFAULTFIFOALLOCATOR_H
#define SPIDER2_DEFAULTFIFOALLOCATOR_H

/* === Include(s) === */

#include <scheduling/allocator/FifoAllocator.h>
#include <archi/MemoryInterface.h>
#include <common/Exception.h>

namespace spider {

    namespace pisdf {
        class Delay;
    }

    /* === Class definition === */

    class DefaultFifoAllocator final : public FifoAllocator {
    public:
        DefaultFifoAllocator() : FifoAllocator() {
            traits_.jitAllocator_ = true;
            traits_.postSchedulingAllocator_ = true;
        };

        ~DefaultFifoAllocator() override = default;

        /* === Method(s) === */

        RTFifo allocate(int64_t size) override;

        void allocate(ScheduleTask *task) override;

        void clear() noexcept override;

        void allocatePersistentDelays(pisdf::Graph *graph) override;

        /* === Getter(s) === */

        inline FifoAllocatorType type() const override {
            return FifoAllocatorType::DEFAULT;
        }

        /* === Setter(s) === */

    private:
        int64_t virtualMemoryAddress_ = 0;
        int64_t reservedMemory_ = 0;

        /* === Private method(s) === */

        void allocateVertexTask(ScheduleTask *task);

        void allocateDefaultVertexTask(ScheduleTask *task);

        void allocateForkTask(ScheduleTask *task);

        void allocateDuplicateTask(ScheduleTask *task);

        void allocateExternInTask(ScheduleTask *task);

        RTFifo createFifoFromDelay(const pisdf::Delay *delay);

        void allocateReceiveTask(ScheduleTask *task);

        void allocateSendTask(ScheduleTask *task);
    };
}

#endif //SPIDER2_DEFAULTFIFOALLOCATOR_H
