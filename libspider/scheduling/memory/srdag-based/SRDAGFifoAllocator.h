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
#ifndef SPIDER2_SRDAGFIFOALLOCATOR_H
#define SPIDER2_SRDAGFIFOALLOCATOR_H

#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <scheduling/memory/FifoAllocator.h>

namespace spider {

    namespace sched {

        /* === Class definition === */

        class SRDAGFifoAllocator final : public FifoAllocator {
        public:
            SRDAGFifoAllocator() : FifoAllocator({ true, true }) {

            }

            ~SRDAGFifoAllocator() noexcept override = default;

            /* === Method(s) === */

            /**
             * @brief Allocate Fifos of a given task.
             * @param task Pointer to the task.
             */
            void allocate(SRDAGTask *task) final;

            /**
             * @brief Creates the fifos needed for the runtime execution of a task.
             * @param task Pointer to the task.
             * @return @refitem unique_ptr of @refitem JobFifos
             */
            spider::unique_ptr<JobFifos> buildJobFifos(SRDAGTask *task) final;

            /* === Getter(s) === */

            /**
             * @brief Get the type of the FifoAllocator
             * @return @refitem FifoAllocatorType
             */
            FifoAllocatorType type() const final { return FifoAllocatorType::DEFAULT; };

        private:

            static Fifo buildInputFifo(const srdag::Edge *edge);

            static Fifo buildOutputFifo(const srdag::Edge *edge);

        };
    }
}
#endif
#endif //SPIDER2_SRDAGFIFOALLOCATOR_H
