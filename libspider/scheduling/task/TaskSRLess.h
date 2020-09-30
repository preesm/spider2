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
#include <containers/vector.h>

namespace spider {

    namespace srless {
        class FiringHandler;

        struct ExecDependency;
        struct ExecDependencyInfo;
    }

    namespace pisdf {
        class Vertex;
    }
    namespace sched {

        /* === Class definition === */

        class TaskSRLess final : public Task {
        public:
            explicit TaskSRLess(srless::FiringHandler *handler, const pisdf::Vertex *vertex, u32 firing);

            ~TaskSRLess() noexcept override = default;

            /* === Virtual method(s) === */

            Task *previousTask(size_t ix) const override;

            spider::array_handle<Task *> getDependencies() const override;

            inline void updateTaskExecutionDependencies(const Schedule *schedule) override;

            void updateExecutionConstraints() override;

            void setExecutionDependency(size_t ix, Task *task) override;

            AllocationRule allocationRuleForInputFifo(size_t ix) const override;

            AllocationRule allocationRuleForOutputFifo(size_t ix) const override;

            JobMessage createJobMessage() const override;

            u32 color() const override;

            std::string name() const override;

            inline bool isSyncOptimizable() const noexcept override { return false; }

            std::pair<ufast64, ufast64> computeCommunicationCost(const PE *mappedPE) const override;

            bool isMappableOnPE(const PE *pe) const override;

            u64 timingOnPE(const PE *pe) const override;

            /* === Getter(s) === */

            DependencyInfo getDependencyInfo(size_t size) const override;

            void setIx(u32 ix) noexcept override;

            /* === Setter(s) === */

        private:
            srless::FiringHandler *handler_;
            const pisdf::Vertex *vertex_;
            u32 firing_;
            u32 dependenciesCount_;


            /* === private method(s) === */

            /* === Dependencies methods === */

            size_t updateTaskExecutionDependency(const Schedule *schedule,
                                                 const srless::ExecDependency &dep,
                                                 size_t index);

            size_t updateTaskExecutionDependency(const Schedule *schedule,
                                                 const srless::ExecDependencyInfo &dependencyInfo,
                                                 size_t index);

            /* === Input FIFO allocation methods === */

            template<typename T>
            AllocationRule allocateInputFifo(const T &dependencies, const pisdf::Edge *edge) const;

            AllocationRule allocateDefaultInputFifo(const spider::vector<srless::ExecDependency> &dependencies) const;

            AllocationRule allocateDefaultInputFifo(const srless::ExecDependency &dependencies) const;

            void setInputFifoExtraRules(const spider::vector<srless::ExecDependency> &dependencies,
                                        AllocationRule *rules) const;

            size_t setInputFifoExtraRules(const srless::ExecDependency &dependencies,
                                          AllocationRule *rules,
                                          size_t offset = 0u) const;

            u32 computeConsCount(const pisdf::Edge *edge) const;

            u32 recursiveConsCount(const pisdf::Edge *edge,
                                   const srless::FiringHandler *handler,
                                   u32 firstFiring,
                                   u32 lastFiring) const;
        };
    }
}

#endif //SPIDER2_TASKSRLESS_H
