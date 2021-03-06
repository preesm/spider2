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
#ifndef SPIDER2_RESOURCESALLOCATOR_H
#define SPIDER2_RESOURCESALLOCATOR_H

/* === Include(s) === */

#include <memory/unique_ptr.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/scheduler/Scheduler.h>
#include <scheduling/mapper/Mapper.h>
#include <global-api.h>

namespace spider {
#ifndef _NO_BUILD_LEGACY_RT
    namespace srdag {
        class Graph;
    }
#endif
    namespace pisdf {
        class GraphHandler;
    }

    namespace sched {

        class FifoAllocator;

        /* === Class definition === */

        class ResourcesAllocator final {
        public:
            explicit ResourcesAllocator(SchedulingPolicy schedulingPolicy,
                                        MappingPolicy mappingPolicy,
                                        ExecutionPolicy executionPolicy,
                                        FifoAllocatorType allocatorType,
                                        bool legacy);

            ~ResourcesAllocator() noexcept = default;

            /* === Method(s) === */

#ifndef _NO_BUILD_LEGACY_RT

            void execute(const srdag::Graph *graph);

#endif

            void execute(pisdf::GraphHandler *graphHandler);

            void clear();

            /* === Getter(s) === */

            inline const Mapper *mapper() const noexcept { return mapper_.get(); }

            inline const Scheduler *scheduler() const noexcept { return scheduler_.get(); }

            inline const Schedule *schedule() const noexcept { return schedule_.get(); }

            inline FifoAllocator *allocator() const noexcept { return allocator_.get(); }

            /* === Setter(s) === */

        private:
            spider::unique_ptr<Scheduler> scheduler_;
            spider::unique_ptr<Mapper> mapper_;
            spider::unique_ptr<Schedule> schedule_;
            spider::unique_ptr<FifoAllocator> allocator_;
            ExecutionPolicy executionPolicy_;

            /* === Private method(s) === */

            template<class T>
            void execute(size_t offset);

            /**
             * @brief Allocates the scheduler corresponding to the given policy.
             * @param policy  Scheduling policy to use.
             * @param legacy  Flag indicating if we are using the legacy intermediate representation (i.e SRDAG).
             * @return pointer to the created @refitem Scheduler.
             * @throw @refitem spider::Exception if the scheduling policy is not supported.
             */
            static Scheduler *allocateScheduler(SchedulingPolicy policy, bool legacy);

            /**
             * @brief Allocates the fifo allocator corresponding to the given type.
             * @param type    Allocator type to use.
             * @param legacy  Flag indicating if we are using the legacy intermediate representation (i.e SRDAG).
             * @return pointer to the created @refitem FifoAllocator.
             * @throw @refitem spider::Exception if the allocating type is not supported.
             */
            static FifoAllocator *allocateAllocator(FifoAllocatorType type, bool legacy);

            /**
             * @brief Allocates the mapper corresponding to the given policy.
             * @param policy  Mapping policy to use.
             * @return pointer to the created @refitem Mapper.
             * @throw @refitem spider::Exception if the mapping policy is not supported.
             */
            static Mapper *allocateMapper(MappingPolicy policy);

            /**
             * @brief Compute the minimum start time for the mapper.
             * @return minimum start time.
             */
            ufast64 computeMinStartTime() const;
        };
    }
}
#endif //SPIDER2_RESOURCESALLOCATOR_H
