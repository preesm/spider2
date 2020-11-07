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
#ifndef SPIDER2_PISDFFIFOALLOCATOR_H
#define SPIDER2_PISDFFIFOALLOCATOR_H

/* === Include(s) === */

#include <scheduling/memory/FifoAllocator.h>
#include <containers/vector.h>

namespace spider {

    /* === Forward declaration(s) === */

    namespace sched {

        class Task;

        /* === Class definition === */

        class PiSDFFifoAllocator final : public FifoAllocator {
        public:
            PiSDFFifoAllocator() : FifoAllocator({ true, true }),
                                   dynamicBuffers_{ factory::vector<dynaBuffer_t>(StackID::SCHEDULE) } {

            }

            ~PiSDFFifoAllocator() noexcept final = default;

            /* === Method(s) === */

            void clear() noexcept final;

            void updateDynamicBuffersCount() final;

            /**
             * @brief Creates the fifos needed for the runtime execution of a task.
             * @param task     Pointer to the task.
             * @return @refitem unique_ptr of @refitem JobFifos
             */
            spider::unique_ptr<JobFifos> buildJobFifos(PiSDFTask *task, JobFifos *fifos) final;

        private:
            struct dynaBuffer_t {
                const PiSDFTask *task_;
                u32 edgeIx_;
                u32 firing_;
            };
            spider::vector<dynaBuffer_t> dynamicBuffers_;

            /**
             * @brief Allocate Fifos of a given task.
             * @param task  Pointer to the task.
             * @param fifos Fifos of the task.
             */
            void allocate(PiSDFTask *task, const JobFifos *fifos);

            /**
             * @brief Creates a single input fifo.
             * @param fifos Pointer to the fifo array (should be offsetted to the current fifo to be set).
             * @param dep   Execution dependency.
             */
            static void buildSingleFifo(Fifo *fifos,
                                        const pisdf::GraphFiring *handler,
                                        const pisdf::Edge *edge,
                                        u32 firing);

            /**
             * @brief Creates a merged input fifo.
             * @param fifos        Pointer to the fifo array (should be offsetted to the current fifo to be set).
             * @param mergedSize   Merged size of the fifo to allocate.
             * @param dependencies Execution dependencies of the task.
             */
            void buildMergeFifo(Fifo *fifos,
                                const pisdf::GraphFiring *handler,
                                const pisdf::Edge *edge,
                                u32 firing);

            /**
             * @brief Creates an input @refitem spider::Fifo from raw information.
             * @param edge    Pointer to the input edge.
             * @param size    Size of the fifo.
             * @param offset  Offset to be applied with respect to the allocated virtual address of the fifo.
             * @param firing  Firing of the original producer of the edge.
             * @param handler Handler associated with the original producer of the edge.
             * @return created Fifo.
             */
            static Fifo buildInputFifo(const pisdf::Edge *edge,
                                       u32 size,
                                       u32 offset,
                                       u32 firing,
                                       const pisdf::GraphFiring *handler);

            /**
             * @brief Set every attribute members of an output fifo (except of the count which should already be set by now).
             * @param fifo  Reference to the fifo to be built.
             * @param edge  Pointer to the output edge/
             * @param task  Pointer to the task.
             */
            void buildOutputFifo(Fifo &fifo, const pisdf::Edge *edge, const PiSDFTask *task);

        };
    }
}
#endif //SPIDER2_PISDFFIFOALLOCATOR_H
