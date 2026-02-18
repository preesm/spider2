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
#ifndef SPIDER2_FIFOALLOCATOR_H
#define SPIDER2_FIFOALLOCATOR_H

/* === Include(s) === */

#include <runtime/common/Fifo.h>
#include <scheduling/memory/JobFifos.h>
#include <graphs-tools/numerical/dependencies.h>

namespace spider {

    /* === Forward declaration(s) === */

#ifndef _NO_BUILD_LEGACY_RT
    namespace srdag {
        class Edge;
    }
#endif

    namespace pisdf {
        class Vertex;

        class GraphFiring;

        class Graph;
    }

    namespace sched {

        class PiSDFTask;

        class SRDAGTask;

        class SyncTask;

        class Schedule;

        /* === Class definition === */

        class FifoAllocator {
        public:
            struct FifoAllocatorTraits {
                bool jitAllocator_;
                bool postSchedulingAllocator_;
            };

            /* === Allocator traits === */

            FifoAllocatorTraits traits_;

            FifoAllocator() : FifoAllocator({ true, true }) {

            }

            virtual ~FifoAllocator() noexcept = default;

            /* === Method(s) === */

            /**
             * @brief Clears the allocator.
             */
            virtual void clear() noexcept;

            /**
             * @brief Allocate size bytes.
             * @param size Size to allocate.
             * @return address of allocated buffer
             */
            size_t allocate(size_t size);

            /**
             * @brief Reserve memory for permanent delays.
             * @param graph pointer to the graph.
             */
            void allocatePersistentDelays(pisdf::Graph *graph);

#ifndef _NO_BUILD_LEGACY_RT

            inline virtual spider::unique_ptr<JobFifos> buildJobFifos(SRDAGTask *) {
                return spider::make_unique<JobFifos>(0u, 0u);
            }

#endif

            inline virtual void updateDynamicBuffersCount() { }

            inline virtual spider::unique_ptr<JobFifos> buildJobFifos(PiSDFTask *, JobFifos *) {
                return spider::make_unique<JobFifos>(0u, 0u);
            }

            /* === Getter(s) === */

            /**
             * @brief Get the type of the FifoAllocator
             * @return @refitem FifoAllocatorType
             */
            inline virtual FifoAllocatorType type() const { return FifoAllocatorType::DEFAULT; };

            /* === Setter(s) === */

            /**
             * @brief Set the schedule that can be used by the FifoAllocator for additionnal information.
             * @param schedule Pointer to the schedule to set.
             */
            inline void setSchedule(const Schedule *schedule) { schedule_ = schedule; }

        private:
            const Schedule *schedule_ = nullptr;
            size_t reservedMemory_ = 0u;
            size_t virtualMemoryAddress_ = 0u;

        protected:
            explicit FifoAllocator(FifoAllocatorTraits traits) noexcept: traits_{ traits } {

            }
        };
    }
}
#endif //SPIDER2_FIFOALLOCATOR_H
