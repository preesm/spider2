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

        class PiSDFTask : public Task {
        public:
            PiSDFTask(pisdf::GraphFiring *handler, const pisdf::Vertex *vertex);

            ~PiSDFTask() override = default;

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

            virtual void reset() = 0;

            spider::vector<pisdf::DependencyIterator> computeExecDependencies() const;

            /* === Getter(s) === */

            inline i64 inputRate(size_t) const final { return 0; };

            inline Task *previousTask(size_t, const Schedule *) const final { return nullptr; }

            inline Task *nextTask(size_t, const Schedule *) const final { return nullptr; }

            inline size_t dependencyCount() const final { return 0u; }

            inline size_t successorCount() const final { return 0u; }

            u32 color() const final;

            std::string name() const final;

            inline u64 startTime() const final {
                return endTime() - timingOnPE(mappedPe());
            }

            bool isMappableOnPE(const PE *pe) const final;

            u64 timingOnPE(const PE *pe) const final;

            const PE *mappedLRT() const final;

            u32 ix() const noexcept final;

            const pisdf::Vertex *vertex() const;

            /**
             * @brief Get the base associated with the task.
             * @return pointer to the base.
             */
            inline pisdf::GraphFiring *handler() const { return handler_; }

            /**
             * @brief Get the current firing of this task.
             * @return firing of the vertex associated to the task.
             */
            u32 firing() const;

            /* === Setter(s) === */

            inline void setStartTime(u64) final { }

            void setIx(u32 ix) noexcept final;

        private:
            pisdf::GraphFiring *handler_{ nullptr };
            u32 vertexIx_{ UINT32_MAX };
            u32 currentFiring_{ 0 };
        };
    }
}

#endif //SPIDER2_PISDFTASK_H
