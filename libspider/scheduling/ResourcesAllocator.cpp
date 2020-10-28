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

/* === Include(s) === */

#include <scheduling/ResourcesAllocator.h>

#ifndef _NO_BUILD_LEGACY_RT

#include <scheduling/scheduler/srdag-based/ListScheduler.h>
#include <scheduling/scheduler/srdag-based/GreedyScheduler.h>
#include <scheduling/task/SRDAGTask.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <graphs/srdag/SRDAGEdge.h>

#endif

#include <scheduling/scheduler/pisdf-based/PiSDFGreedyScheduler.h>
#include <scheduling/scheduler/pisdf-based/PiSDFListScheduler.h>
#include <scheduling/mapper/BestFitMapper.h>
#include <scheduling/memory/FifoAllocator.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs-tools/numerical/dependencies.h>
#include <scheduling/task/pisdf-based/PiSDFTask.h>
#include <scheduling/task/pisdf-based/MergeTask.h>
#include <api/archi-api.h>
#include <archi/PE.h>
#include <common/Time.h>
#include <scheduling/launcher/TaskLauncher.h>

/* === Static function === */

static void checkFifoAllocatorTraits(const spider::sched::FifoAllocator *allocator, spider::ExecutionPolicy policy) {
    switch (policy) {
        case spider::ExecutionPolicy::JIT:
            if (!allocator->traits_.jitAllocator_) {
                throwSpiderException("Using a scheduler in JIT_SEND mode with incompatible fifo allocator.");
            }
            break;
        case spider::ExecutionPolicy::DELAYED:
            if (!allocator->traits_.postSchedulingAllocator_) {
                throwSpiderException("Using a scheduler in DELAYED_SEND mode with incompatible fifo allocator.");
            }
            break;
    }
}

/* === Method(s) implementation === */

spider::sched::ResourcesAllocator::ResourcesAllocator(SchedulingPolicy schedulingPolicy,
                                                      MappingPolicy mappingPolicy,
                                                      ExecutionPolicy executionPolicy,
                                                      FifoAllocatorType allocatorType,
                                                      bool legacy) :
        scheduler_{ spider::make_unique(allocateScheduler(schedulingPolicy, legacy)) },
        mapper_{ spider::make_unique(allocateMapper(mappingPolicy)) },
        schedule_{ spider::make_unique<Schedule, StackID::SCHEDULE>() },
        allocator_{ spider::make_unique(allocateAllocator(allocatorType, legacy)) },
        executionPolicy_{ executionPolicy } {
    if (allocator_) {
        checkFifoAllocatorTraits(allocator_.get(), executionPolicy);
        allocator_->setSchedule(schedule_.get());
    }
}

#ifndef _NO_BUILD_LEGACY_RT

void spider::sched::ResourcesAllocator::execute(const srdag::Graph *graph) {
    /* == Schedule the graph == */
    const auto result = scheduler_->schedule(graph);
    auto launcher = TaskLauncher{ schedule_.get(), allocator_.get() };
    mapper_->setStartTime(computeMinStartTime());
    if (executionPolicy_ == ExecutionPolicy::JIT) {
        for (auto *vertex : result) {
            /* == Create and allocate the task == */
            auto *task = spider::make<sched::SRDAGTask, StackID::SCHEDULE>(vertex);
            /* == Map the task == */
            const auto currentTaskCount = schedule_->taskCount();
            mapper_->map(task, schedule_.get());
            /* == Allocate the task == */
            allocator_->allocate(vertex);
            /* == Add the task == */
            if (schedule_->taskCount() > currentTaskCount) {
                /* == We added synchronization == */
                for (auto i = currentTaskCount; i < schedule_->taskCount(); ++i) {
                    auto *syncTask = schedule_->task(i);
                    syncTask->visit(&launcher);
                }
            }
            schedule_->addTask(task);
            /* == Send the task == */
            task->visit(&launcher);
        }
    } else if (executionPolicy_ == ExecutionPolicy::DELAYED) {
        const auto currentTaskCount = schedule_->taskCount();
        for (auto *vertex : result) {
            /* == Create and allocate the task == */
            auto *task = spider::make<sched::SRDAGTask, StackID::SCHEDULE>(vertex);
            /* == Map the task == */
            mapper_->map(task, schedule_.get());
            /* == Allocate the task == */
            allocator_->allocate(vertex);
            /* == Add the task == */
            schedule_->addTask(task);
        }
        for (auto i = currentTaskCount; i < schedule_->taskCount(); ++i) {
            /* == Send the task == */
            auto *task = schedule_->task(i);
            task->visit(&launcher);
        }
    } else {
        throwSpiderException("unexpected execution policy.");
    }
}

#endif

void spider::sched::ResourcesAllocator::execute(pisdf::GraphHandler *graphHandler) {
    /* == Schedule the graph == */
    const auto result = scheduler_->schedule(graphHandler);
    auto launcher = TaskLauncher{ schedule_.get(), allocator_.get() };
    mapper_->setStartTime(computeMinStartTime());
    if (executionPolicy_ == ExecutionPolicy::JIT) {
        for (auto &schedVertex : result) {
            auto *handler = schedVertex.handler_;
            const auto *vertex = schedVertex.handler_->vertex(schedVertex.vertexIx_);
            auto *task = spider::make<sched::PiSDFTask, StackID::SCHEDULE>(handler, vertex, schedVertex.firing_);
            /* == Map the task == */
            const auto currentTaskCount = schedule_->taskCount();
            mapper_->map(task, schedule_.get());
            /* == Allocate the task == */
            allocator_->allocate(handler, vertex);
            /* == Add the task == */
            if (schedule_->taskCount() > currentTaskCount) {
                /* == We added synchronization == */
                for (auto i = currentTaskCount; i < schedule_->taskCount(); ++i) {
                    auto *syncTask = schedule_->task(i);
                    syncTask->visit(&launcher);
                }
            }
            schedule_->addTask(task);
            /* == Send the task == */
            task->visit(&launcher);
        }
    } else if (executionPolicy_ == ExecutionPolicy::DELAYED) {
        const auto currentTaskCount = schedule_->taskCount();
        for (const auto &schedVertex : result) {
            /* == Create and allocate the task == */
            auto *handler = schedVertex.handler_;
            const auto *vertex = schedVertex.handler_->vertex(schedVertex.vertexIx_);
            auto *task = spider::make<sched::PiSDFTask, StackID::SCHEDULE>(handler, vertex, schedVertex.firing_);
            /* == Map the task == */
            mapper_->map(task, schedule_.get());
            /* == Allocate the task == */
            allocator_->allocate(handler, vertex);
            /* == Add the task == */
            schedule_->addTask(task);
        }
        for (auto i = currentTaskCount; i < schedule_->taskCount(); ++i) {
            /* == Send the task == */
            auto *task = schedule_->task(i);
            task->visit(&launcher);
        }
    } else {
        throwSpiderException("unexpected execution policy.");
    }
}

void spider::sched::ResourcesAllocator::clear() {
    allocator_->clear();
    schedule_->clear();
    scheduler_->clear();
}

/* === Private method(s) implementation === */

spider::sched::Scheduler *
spider::sched::ResourcesAllocator::allocateScheduler(SchedulingPolicy policy, bool legacy) {
    switch (policy) {
        case SchedulingPolicy::LIST:
            if (legacy) {
#ifndef _NO_BUILD_LEGACY_RT
                return spider::make<sched::ListScheduler, StackID::SCHEDULE>();
#else
                return nullptr;
#endif
            }
            return spider::make<sched::PiSDFListScheduler, StackID::SCHEDULE>();
        case SchedulingPolicy::GREEDY:
            if (legacy) {
#ifndef _NO_BUILD_LEGACY_RT
                return spider::make<sched::GreedyScheduler, StackID::SCHEDULE>();
#else
                return nullptr;
#endif
            } else {
                return spider::make<sched::PiSDFGreedyScheduler, StackID::SCHEDULE>();
            }
        default:
            throwSpiderException("unsupported scheduling policy.");
    }
}

spider::sched::FifoAllocator *
spider::sched::ResourcesAllocator::allocateAllocator(FifoAllocatorType type, bool legacy) {
    switch (type) {
        case spider::FifoAllocatorType::DEFAULT:
            if (!legacy) {
                return spider::make<spider::sched::FifoAllocator, StackID::RUNTIME>();
            }
#ifndef _NO_BUILD_LEGACY_RT
            return spider::make<spider::sched::FifoAllocator, StackID::RUNTIME>();
#else
            printer::fprintf(stderr, "Default allocator is part of the legacy runtime which was not built.\n"
                                     "Rebuild the Spider 2.0 library with the cmake flag -DBUILD_LEGACY_RUNTIME=ON.\n");
            return nullptr;
#endif
        case spider::FifoAllocatorType::DEFAULT_NOSYNC:
            if (!legacy) {
                return spider::make<spider::sched::FifoAllocator, StackID::RUNTIME>();
            }
#ifndef _NO_BUILD_LEGACY_RT
            return spider::make<spider::sched::FifoAllocator, StackID::RUNTIME>();
#else
            printer::fprintf(stderr, "NO_SYNC allocator is part of the legacy runtime which was not built.\n"
                                     "Rebuild the Spider 2.0 library with the cmake flag -DBUILD_LEGACY_RUNTIME=ON.\n");
            return nullptr;
#endif
        case spider::FifoAllocatorType::ARCHI_AWARE:
        default:
            throwSpiderException("unsupported type of FifoAllocator.");
    }
}

spider::sched::Mapper *spider::sched::ResourcesAllocator::allocateMapper(MappingPolicy policy) {
    switch (policy) {
        case MappingPolicy::BEST_FIT:
            return spider::make<sched::BestFitMapper, StackID::SCHEDULE>();
        default:
            throwSpiderException("unsupported mapping policy.");
    }
}

ufast64 spider::sched::ResourcesAllocator::computeMinStartTime() const {
    ufast64 minStartTime = UINT_FAST64_MAX;
    for (const auto *pe : archi::platform()->peArray()) {
        minStartTime = std::min(minStartTime, schedule_->stats().endTime(pe->virtualIx()));
    }
    return minStartTime;
}
