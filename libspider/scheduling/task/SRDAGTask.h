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
#ifndef SPIDER2_SRDAGTASK_H
#define SPIDER2_SRDAGTASK_H

#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <scheduling/task/Task.h>
#include <memory/unique_ptr.h>
#include <containers/array.h>

namespace spider {

    namespace srdag {
        class Vertex;
    }

    namespace sched {

        /* === Class definition === */

        class SRDAGTask final : public Task {
        public:
            explicit SRDAGTask(srdag::Vertex *vertex);

            ~SRDAGTask() noexcept override = default;

            /* === Method(s) === */

            void visit(TaskLauncher *launcher) final;

            bool receiveParams(const spider::array<i64> &values) final;

            /* === Getter(s) === */

            i64 inputRate(size_t ix) const final;

            Task *previousTask(size_t ix, const Schedule *schedule) const final;

            Task *nextTask(size_t ix, const Schedule *schedule) const final;

            u32 color() const final;

            std::string name() const final;

            u32 ix() const noexcept final;

            bool isMappableOnPE(const PE *pe) const final;

            u64 timingOnPE(const PE *pe) const final;

            size_t dependencyCount() const final;

            size_t successorCount() const final;

            u64 startTime() const final;

            inline u64 endTime() const final { return endTime_; }

            const PE *mappedPe() const final;

            const PE *mappedLRT() const final;

            inline u32 jobExecIx() const noexcept final { return jobExecIx_; }

            inline TaskState state() const noexcept final { return state_; }

            inline u32 syncExecIxOnLRT(size_t lrtIx) const final { return syncExecTaskIxArray_[lrtIx]; }

            inline srdag::Vertex *vertex() const { return vertex_; }

            /* === Setter(s) === */

            inline void setStartTime(u64) final {  }

            inline void setEndTime(u64 time) final { endTime_ = time; }

            void setIx(u32 ix) noexcept final;

            inline void setState(TaskState state) noexcept final { state_ = state; }

            inline void setJobExecIx(u32 ix) noexcept final { jobExecIx_ = ix; }

            inline void setMappedPE(const spider::PE *pe) final;

            inline void setSyncExecIxOnLRT(size_t lrtIx, u32 value) override {
                if (syncExecTaskIxArray_[lrtIx] == UINT32_MAX || value > syncExecTaskIxArray_[lrtIx]) {
                    syncExecTaskIxArray_[lrtIx] = value;
                }
            }

        private:
            u64 endTime_{ UINT64_MAX };                     /*!< Mapping end time of the vertexTask */
            spider::unique_ptr<u32> syncExecTaskIxArray_;
            srdag::Vertex *vertex_ = nullptr;
            u32 mappedPEIx_ = UINT32_MAX;                   /*!< Mapping PE of the vertexTask */
            u32 jobExecIx_{ UINT32_MAX };                   /*!< Index of the job sent to the PE */
            TaskState state_{ TaskState::NOT_SCHEDULABLE }; /*!< State of the vertexTask */
        };
    }
}
#endif
#endif //SPIDER2_SRDAGTASK_H
