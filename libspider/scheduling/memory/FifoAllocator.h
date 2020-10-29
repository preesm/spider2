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
#ifndef SPIDER2_FIFOALLOCATOR_H
#define SPIDER2_FIFOALLOCATOR_H

/* === Include(s) === */

#include <runtime/common/Fifo.h>
#include <scheduling/memory/JobFifos.h>
#include <graphs-tools/numerical/dependencies.h>

namespace spider {

    /* === Forward declaration(s) === */

    namespace srdag {
        class Vertex;

        class Edge;
    }

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

        class FifoAllocator final {
        public:
            struct FifoAllocatorTraits {
                bool jitAllocator_;
                bool postSchedulingAllocator_;
            };

            /* === Allocator traits === */

            FifoAllocatorTraits traits_;

            FifoAllocator() : FifoAllocator({ true, true }) { }

            ~FifoAllocator() noexcept = default;

            /* === Method(s) === */

            /**
             * @brief Clears the allocator.
             */
            virtual void clear() noexcept;


#ifndef _NO_BUILD_LEGACY_RT

            /**
             * @brief Allocate Fifos of a given task.
             * @param task Pointer to the task.
             */
            void allocate(SRDAGTask *task);

#endif


            void allocate(PiSDFTask *task);

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

            spider::unique_ptr<JobFifos> buildJobFifos(SRDAGTask *task) const;

#endif

            spider::unique_ptr<JobFifos> buildJobFifos(SyncTask *task) const;

            spider::unique_ptr<JobFifos> buildJobFifos(PiSDFTask *task,
                                                       const spider::vector<pisdf::DependencyIterator> &execDeps,
                                                       const spider::vector<pisdf::DependencyIterator> &consDeps);

            /* === Getter(s) === */

            /**
             * @brief Get the type of the FifoAllocator
             * @return @refitem FifoAllocatorType
             */
            inline FifoAllocatorType type() const { return FifoAllocatorType::DEFAULT; };

            /* === Setter(s) === */

            /**
             * @brief Set the schedule that can be used by the FifoAllocator for additionnal information.
             * @param schedule Pointer to the schedule to set.
             */
            inline void setSchedule(const Schedule *schedule) {
                schedule_ = schedule;
            }

        protected:
            const Schedule *schedule_ = nullptr;
            size_t reservedMemory_ = 0;
            size_t virtualMemoryAddress_ = 0;

            explicit FifoAllocator(FifoAllocatorTraits traits) noexcept: traits_{ traits } { }

        private:

#ifndef _NO_BUILD_LEGACY_RT

            static Fifo buildInputFifo(const srdag::Edge *edge);

            static Fifo buildOutputFifo(const srdag::Edge *edge);

#endif

            static Fifo buildInputFifo(const pisdf::Edge *edge,
                                       u32 size,
                                       u32 offset,
                                       u32 firing,
                                       const pisdf::GraphFiring *handler);

            static Fifo buildOutputFifo(const JobFifos *fifos,
                                        const pisdf::Edge *edge,
                                        pisdf::GraphFiring *handler,
                                        const pisdf::DependencyIterator &depIt,
                                        u32 firing);

            static i32 getFifoCount(const pisdf::DependencyIterator &depIt);

            static size_t getFifoAddress(const pisdf::Edge *edge, u32 firing, const pisdf::GraphFiring *handler);

            Fifo buildMergeFifo(Fifo *fifos,
                                const PiSDFTask *task,
                                const pisdf::Edge *edge,
                                const pisdf::DependencyIterator &depIt);
        };
    }
}
#endif //SPIDER2_FIFOALLOCATOR_H
