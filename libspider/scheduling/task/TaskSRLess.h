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
#ifndef SPIDER2_TASKSRLESS_H
#define SPIDER2_TASKSRLESS_H

/* === Include(s) === */

#include <scheduling/task/Task.h>

namespace spider {

    namespace srless {
        class FiringHandler;
    }

    namespace pisdf {
        class Vertex;
    }
    namespace sched {

        /* === Class definition === */

        class TaskSRLess final : public Task {
        public:
            explicit TaskSRLess(const srless::FiringHandler *handler,
                                const pisdf::Vertex *vertex,
                                u32 firing);

            ~TaskSRLess() noexcept override = default;

            /* === Virtual method(s) === */

            AllocationRule allocationRuleForInputFifo(size_t ix) const override;

            AllocationRule allocationRuleForOutputFifo(size_t ix) const override;

            Task *previousTask(size_t ix) const override;

            u32 color() const override;

            std::string name() const override;

            inline void updateTaskExecutionDependencies(const Schedule *) override { }

            void updateExecutionConstraints() override;

            JobMessage createJobMessage() const override;

            inline bool isSyncOptimizable() const noexcept override { return false; }

            spider::array_handle<Task *> getDependencies() const override;

            std::pair<ufast64, ufast64> computeCommunicationCost(const PE *mappedPE) const override;

            void setExecutionDependency(size_t ix, Task *task) override;

            bool isMappableOnPE(const PE *pe) const override;

            u64 timingOnPE(const PE *pe) const override;

            /* === Getter(s) === */
            DependencyInfo getDependencyInfo(size_t size) const override;

            /* === Setter(s) === */

        private:
            const srless::FiringHandler *handler_;
            const pisdf::Vertex *vertex_;
            u32 firing_;
        };
    }
}

#endif //SPIDER2_TASKSRLESS_H
