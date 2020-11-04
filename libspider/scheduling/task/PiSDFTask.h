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
#ifndef SPIDER2_PISDFTASK_H
#define SPIDER2_PISDFTASK_H

/* === Include(s) === */

#include <scheduling/task/Task.h>
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

        class PiSDFTask final : public Task {
        public:
            PiSDFTask(pisdf::GraphFiring *handler, const pisdf::Vertex *vertex);

            ~PiSDFTask() final = default;

            /* === Method(s) === */

            void visit(sched::TaskLauncher *launcher) final;

            /**
             * @brief Update output params based on received values.
             * @param values Values of the params.
             */
            bool receiveParams(const spider::array<i64> &values) final;

            /**
             * @brief Set task on a given firing (can be used anywhere as long as you know what you're doing).
             * @param firing Firing value to set.
             */
            void setOnFiring(u32 firing) final;

            spider::vector<pisdf::DependencyIterator> computeExecDependencies() const;

            spider::vector<pisdf::DependencyIterator> computeConsDependencies() const;

            /* === Getter(s) === */

            inline i64 inputRate(size_t) const final { return 0; };

            inline Task *previousTask(size_t, const Schedule *) const final { return nullptr; }

            inline Task *nextTask(size_t, const Schedule *) const final { return nullptr; }

            inline size_t dependencyCount() const final { return 0u; }

            inline size_t successorCount() const final { return 0u; }

            /**
             * @brief Return a color value for the vertexTask.
             *        format is RGB with 8 bits per component in the lower part of the returned value.
             * @return  color of the vertexTask.
             */
            u32 color() const final;

            /**
             * @brief Returns the name of the vertexTask
             * @return name of the vertexTask
             */
            std::string name() const final;

            /**
             * @brief Check if the vertexTask is mappable on a given PE.
             * @param pe  Pointer to the PE.
             * @return true if mappable on PE, false else.
             */
            bool isMappableOnPE(const PE *pe) const final;

            /**
             * @brief Get the execution timing on a given PE.
             * @param pe  Pointer to the PE.
             * @return exec timing on the PE, UINT64_MAX else.
             */
            u64 timingOnPE(const PE *pe) const final;

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
             * @brief Returns the LRT attached to the PE on which the given firing is mapped.
             * @param firing  Firing value.
             * @return pointer to the LRT, nullptr else
             */
            const PE *mappedLRT() const final;

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
             * @brief Returns the ix of the vertexTask in the schedule.
             * @param firing  Firing value.
             * @return ix of the vertexTask in the schedule, -1 else.
             */
            u32 ix() const noexcept final;

            /**
             * @brief Get the dependency task ix on given LRT and for a given firing.
             * @param lrtIx    Index of the LRT to evaluate.
             * @param firing   Firing value.
             * @return value of the task ix, UINT32_MAX else.
             */
            u32 syncExecIxOnLRT(size_t lrtIx) const final;

            /**
             * @brief Get the vertex associated with the task.
             * @return pointer to the vertex.
             */
            const pisdf::Vertex *vertex() const;

            /**
             * @brief Get the base associated with the task.
             * @return pointer to the base.
             */
            inline pisdf::GraphFiring *handler() const { return handler_; }

            u32 firing() const;

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

            /**
             * @brief Set the ix of the job.
             * @remark This method will overwrite current value.
             * @param ix Ix to set.
             * @param firing  Firing value.
             */
            void setIx(u32 ix) noexcept final;

            void setSyncExecIxOnLRT(size_t lrtIx, u32 value) final;

        private:
            spider::unique_ptr<u32> syncExecTaskIxArray_; /*!< Exec constraints array of the instances of the vertex*/
            spider::unique_ptr<u64> endTimeArray_;        /*!< Mapping end time array of the instances of the vertex */
            spider::unique_ptr<u32> mappedPEIxArray_;     /*!< Mapping PE array of the instances of the vertex */
            spider::unique_ptr<u32> jobExecIxArray_;      /*!< Index array of the job sent to the PE */
            spider::unique_ptr<TaskState> stateArray_;    /*!< State array of the instances of the vertex */
            pisdf::GraphFiring *handler_{ nullptr };
            u32 vertexIx_{ UINT32_MAX };
            u32 currentFiring_{ 0 };
        };
    }
}

#endif //SPIDER2_PISDFTASK_H
