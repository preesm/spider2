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

#include <scheduling/scheduler/ListScheduler.h>
#include <scheduling/scheduler/GreedyScheduler.h>

#endif

#include <scheduling/scheduler/SRLessGreedyScheduler.h>
#include <scheduling/scheduler/SRLessListScheduler.h>
#include <scheduling/mapper/BestFitMapper.h>
#include <scheduling/memory/FifoAllocator.h>
#include <scheduling/memory/NoSyncFifoAllocator.h>
#include <scheduling/task/Task.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <archi/PE.h>
#include <common/Time.h>

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
    }
}

void spider::sched::ResourcesAllocator::execute(const pisdf::Graph *graph) {
    /* == Schedule the graph == */
    auto start = spider::time::now();
    scheduler_->schedule(graph);
    auto end = spider::time::now();
    auto duration = time::duration::nanoseconds(start, end);
    printer::fprintf(stderr, "sched-time: %lld ns\n", duration);
    /* == Map and execute the scheduled tasks == */
    applyExecPolicy();
}

void spider::sched::ResourcesAllocator::execute(srless::GraphHandler *graphHandler) {
    /* == Schedule the graph == */
    auto start = spider::time::now();
    scheduler_->schedule(graphHandler);
    auto end = spider::time::now();
    auto duration = time::duration::nanoseconds(start, end);
    printer::fprintf(stderr, "sched-time: %lld ns\n", duration);
    /* == Map and execute the scheduled tasks == */
    applyExecPolicy();
}

void spider::sched::ResourcesAllocator::clear() {
    allocator_->clear();
    schedule_->clear();
    scheduler_->clear();
}

/* === Private method(s) implementation === */

spider::sched::Scheduler *
spider::sched::ResourcesAllocator::allocateScheduler(SchedulingPolicy policy, bool legacy) const {
    switch (policy) {
        case SchedulingPolicy::LIST:
            if (legacy) {
#ifndef _NO_BUILD_LEGACY_RT
                return spider::make<sched::ListScheduler, StackID::SCHEDULE>();
#else
                return nullptr;
#endif
            }
            return spider::make<sched::SRLessListScheduler, StackID::SCHEDULE>();
        case SchedulingPolicy::GREEDY:
            if (legacy) {
#ifndef _NO_BUILD_LEGACY_RT
                return spider::make<sched::GreedyScheduler, StackID::SCHEDULE>();
#else
                return nullptr;
#endif
            } else {
                return spider::make<sched::SRLessGreedyScheduler, StackID::SCHEDULE>();
            }
        default:
            throwSpiderException("unsupported scheduling policy.");
    }
}

spider::sched::FifoAllocator *
spider::sched::ResourcesAllocator::allocateAllocator(FifoAllocatorType type, bool legacy) const {
    switch (type) {
        case spider::FifoAllocatorType::DEFAULT:
            return spider::make<spider::sched::FifoAllocator, StackID::RUNTIME>();
        case spider::FifoAllocatorType::DEFAULT_NOSYNC:
            if (!legacy) {
                return spider::make<spider::sched::FifoAllocator, StackID::RUNTIME>();
            }
            return spider::make<spider::sched::NoSyncFifoAllocator, StackID::RUNTIME>();
        case spider::FifoAllocatorType::ARCHI_AWARE:
        default:
            throwSpiderException("unsupported type of FifoAllocator.");
    }
}

spider::sched::Mapper *spider::sched::ResourcesAllocator::allocateMapper(MappingPolicy policy) const {
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

void spider::sched::ResourcesAllocator::applyExecPolicy() {
    mapper_->setStartTime(computeMinStartTime());
    switch (executionPolicy_) {
        case ExecutionPolicy::JIT:
            /* == Map, allocate fifos and execute tasks == */
            for (auto &task : scheduler_->tasks()) {
                /* == Map the task == */
                mapper_->map(task.get(), schedule_.get());
                /* == We are in JIT mode, we need to broadcast the job stamp == */
                task->enableBroadcast();
                /* == Allocate the fifos task == */
                allocator_->allocate(task.get());
                /* == Add and execute the task == */
                schedule_->addTask(std::move(task));
                schedule_->sendReadyTasks();
            }
            break;
        case ExecutionPolicy::DELAYED: {            /* == Map every tasks == */
            auto start = spider::time::now();
            for (auto &task : scheduler_->tasks()) {
                mapper_->map(task.get(), schedule_.get());
                schedule_->addTask(std::move(task));
            }
            auto end = spider::time::now();
            auto duration = time::duration::nanoseconds(start, end);
            printer::fprintf(stderr, "map-time:   %lld ns\n", duration);
            /* == Allocate fifos for every tasks == */
            start = spider::time::now();
            for (auto *task : schedule_->readyTasks()) {
                allocator_->allocate(task);
            }
            end = spider::time::now();
            duration = time::duration::nanoseconds(start, end);
            printer::fprintf(stderr, "mem-time:   %lld ns\n", duration);
            /* == Execute every tasks == */
            schedule_->sendReadyTasks();
        }
            break;
        default:
            throwSpiderException("unsupported execution policy.");
    }
}
