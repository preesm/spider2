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
#ifndef SPIDER2_MERGETASK_H
#define SPIDER2_MERGETASK_H

/* === Include(s) === */

#include <scheduling/task/Task.h>
#include <graphs-tools/numerical/detail/DependencyIterator.h>
#include <runtime/special-kernels/specialKernels.h>

namespace spider {

    namespace pisdf {
        class Vertex;

        class Edge;

        class GraphFiring;
    }

    namespace sched {

        /* === Class definition === */

        class MergeTask final : public Task {
        public:
            MergeTask(const pisdf::DependencyIterator &dependencies,
                      i32 depCount,
                      Task *successor,
                      const Schedule *schedule);

            ~MergeTask() noexcept final = default;

            /* === Method(s) === */

            /* === Getter(s) === */

            inline i64 inputRate(size_t) const final { return fifos_->inputFifo(0).size_; }

            inline i64 outputRate(size_t) const final { return fifos_->outputFifo(0).size_; }

            Task *previousTask(size_t ix, const Schedule *schedule) const final;

            Task *nextTask(size_t, const Schedule *schedule) const final;

            inline u32 color() const final {
                /* == Studio Purple == */
                return 0x8e44ad;
            }

            inline std::string name() const final { return "merge"; }

            inline u32 ix() const noexcept final { return ix_; }

            u64 timingOnPE(const PE *) const final;

            inline size_t dependencyCount() const final { return depInCount_; }

            inline size_t successorCount() const final { return 1u; }

            /* === Setter(s) === */

            inline void setIx(u32 ix) noexcept final { ix_ = ix; }

        private:
            std::shared_ptr<JobFifos> fifos_;
            spider::unique_ptr<u32> inputs_;
            u32 successorIx_ = UINT32_MAX;
            u32 depInCount_ = 0;
            u32 ix_ = 0;

            /* === Private method(s) === */

            inline u32 getKernelIx() const final { return rt::JOIN_KERNEL_IX; }

            spider::unique_ptr<i64> buildInputParams() const final;

            inline std::shared_ptr<JobFifos> buildJobFifos(const Schedule *schedule) const final { return fifos_; }

        };
    }
}

#endif //SPIDER2_MERGETASK_H
