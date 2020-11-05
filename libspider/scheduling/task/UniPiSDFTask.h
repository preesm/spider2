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

            /**
             * @brief Get the start time of the given firing.
             * @param firing  Firing value.
             * @return mapping start time of the vertexTask, UINT64_MAX else
             */
            u64 startTime() const final;

            /**
             * @brief Get the end time of the given firing.
             * @param firing  Firing value.
             * @return mapping end time of the vertexTask, UINT64_MAX else
             */
            u64 endTime() const final;

            /**
             * @brief Returns the PE on which the given firing is mapped.
             * @param firing  Firing value.
             * @return pointer to the PE onto which the vertexTask is mapped, nullptr else
             */
            const PE *mappedPe() const final;

            /**
             * @brief Returns the state of the given firing.
             * @param firing  Firing value.
             * @return @refitem TaskState of the vertexTask
             */
            TaskState state() const noexcept final;

            /**
             * @brief Returns the executable job index value of the vertexTask in the job queue of the mapped PE.
             * @param firing  Firing value.
             * @return ix value, SIZE_MAX else.
             */
            u32 jobExecIx() const noexcept final;

            /**
             * @brief Get the dependency task ix on given LRT and for a given firing.
             * @param lrtIx    Index of the LRT to evaluate.
             * @param firing   Firing value.
             * @return value of the task ix, UINT32_MAX else.
             */
            u32 syncExecIxOnLRT(size_t lrtIx) const final;

            /* === Setter(s) === */

            inline void setStartTime(u64) final { }

            /**
             * @brief Set the end time of the task for a given firing.
             * @remark This method will overwrite current value.
             * @param time  Value to set.
             * @param firing  Firing value.
             */
            void setEndTime(u64 time) final;

            /**
            * @brief Set the processing element of the job.
            * @remark This method will overwrite current values.
            * @param mappedPE  Lrt ix inside spider.
             * @param firing  Firing value.
            */
            void setMappedPE(const PE *pe) final;

            /**
             * @brief Set the state of the job.
             * @remark This method will overwrite current value.
             * @param state State to set.
             * @param firing  Firing value.
             */
            void setState(TaskState state) noexcept final;

            /**
             * @brief Set the execution job index value of the vertexTask (that will be used for synchronization).
             * @remark This method will overwrite current values.
             * @param ix Ix to set.
             * @param firing  Firing value.
             */
            void setJobExecIx(u32 ix) noexcept final;

            void setSyncExecIxOnLRT(size_t lrtIx, u32 value) final;

        private:
            spider::unique_ptr<u32> syncExecTaskIxArray_;  /*!< Exec constraints array of the instances of the vertex*/
            u64 endTime_ = 0;                              /*!< Mapping end time array of the instances of the vertex */
            u32 mappedPEIx_ = UINT32_MAX;                  /*!< Mapping PE array of the instances of the vertex */
            u32 jobExecIx_ = UINT32_MAX;                   /*!< Index array of the job sent to the PE */
            TaskState state_ = TaskState::NOT_SCHEDULABLE; /*!< State array of the instances of the vertex */
        };
    }
}

#endif //SPIDER2_UNIPISDFTASK_H
