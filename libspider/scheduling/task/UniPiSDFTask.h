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
#ifndef SPIDER2_UNIPISDFTASK_H
#define SPIDER2_UNIPISDFTASK_H

/* === Include(s) === */

#include <scheduling/task/PiSDFTask.h>
#include <memory/unique_ptr.h>
#include <graphs-tools/numerical/detail/DependencyIterator.h>

namespace spider {

    namespace pisdf {
        class Vertex;

        class Edge;

        class GraphFiring;
    }

    namespace sched {

        /* === Class definition === */

        class UniPiSDFTask final : public PiSDFTask {
        public:
            UniPiSDFTask(pisdf::GraphFiring *handler, const pisdf::Vertex *vertex);

            ~UniPiSDFTask() final = default;

            /* === Method(s) === */

            void reset() final;

            /* === Getter(s) === */

            inline u64 endTime() const final { return endTime_; }

            const PE *mappedPe() const final;

            inline u32 jobExecIx() const noexcept final { return jobExecIx_; }

            inline TaskState state() const noexcept final { return state_; }

            inline u32 syncExecIxOnLRT(size_t lrtIx) const final { return syncInfoArray_[lrtIx].first; }

            inline u32 syncRateOnLRT(size_t lrtIx) const final { return syncInfoArray_[lrtIx].second; }

            /* === Setter(s) === */

            inline void setEndTime(u64 time) final { endTime_ = time; }

            void setMappedPE(const PE *pe) final;

            inline void setState(TaskState state) noexcept final { state_ = state; }

            inline void setJobExecIx(u32 ix) noexcept final { jobExecIx_ = ix; }

            inline void setSyncExecIxOnLRT(size_t lrtIx, u32 value) final {
                syncInfoArray_[lrtIx].first = value;
            }

            inline void setSyncRateOnLRT(size_t lrtIx, u32 value) final {
                syncInfoArray_[lrtIx].second = value;
            }

        private:
            spider::unique_ptr<SyncInfo> syncInfoArray_;   /*!< Exec constraints array of the instances of the vertex*/
            u64 endTime_ = 0;                              /*!< Mapping end time array of the instances of the vertex */
            u32 mappedPEIx_ = UINT32_MAX;                  /*!< Mapping PE array of the instances of the vertex */
            u32 jobExecIx_ = UINT32_MAX;                   /*!< Index array of the job sent to the PE */
            TaskState state_ = TaskState::NOT_SCHEDULABLE; /*!< State array of the instances of the vertex */
        };
    }
}

#endif //SPIDER2_UNIPISDFTASK_H
