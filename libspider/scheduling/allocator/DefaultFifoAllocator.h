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

    class DefaultFifoAllocator : public FifoAllocator {
    public:
        DefaultFifoAllocator(FifoAllocatorTraits traits = { true, true }) : FifoAllocator(traits) { }

        ~DefaultFifoAllocator() override = default;

        /* === Method(s) === */

        RTFifo allocate(size_t size) override;

        void allocate(ScheduleTask *task) override;

        void clear() noexcept override;

        void allocatePersistentDelays(pisdf::Graph *graph) override;

        /* === Getter(s) === */

        inline FifoAllocatorType type() const override { return FifoAllocatorType::DEFAULT; }

        /* === Setter(s) === */

    protected:
        size_t reservedMemory_ = 0;
        size_t virtualMemoryAddress_ = 0;

        /* === Protected method(s) === */

        virtual void allocateVertexTask(ScheduleTask *task);

        virtual void allocateDefaultVertexTask(ScheduleTask *task);

        virtual RTFifo allocateDefaultVertexInputFifo(ScheduleTask *task, const pisdf::Edge *edge);

        virtual RTFifo allocateDefaultVertexOutputFifo(const pisdf::Edge *edge);

        virtual void allocateRepeatTask(ScheduleTask *task);

        virtual void allocateForkTask(ScheduleTask *task);

        virtual void allocateDuplicateTask(ScheduleTask *task);

        virtual void allocateExternInTask(ScheduleTask *task);

        virtual void allocateReceiveTask(ScheduleTask *task);

        virtual void allocateSendTask(ScheduleTask *task);
    };
}

#endif //SPIDER2_DEFAULTFIFOALLOCATOR_H
