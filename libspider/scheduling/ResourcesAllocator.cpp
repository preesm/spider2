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
#include <scheduling/scheduler/ListScheduler.h>
#include <scheduling/scheduler/GreedyScheduler.h>
#include <scheduling/mapper/BestFitMapper.h>
#include <scheduling/memory/FifoAllocator.h>
#include <scheduling/memory/NoSyncFifoAllocator.h>
#include <scheduling/task/TaskVertex.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <archi/PE.h>

/* === Static function === */

static spider::sched::FifoAllocator *makeFifoAllocator(spider::FifoAllocatorType type) {
    switch (type) {
        case spider::FifoAllocatorType::DEFAULT:
            return spider::make<spider::sched::FifoAllocator, StackID::RUNTIME>();
        case spider::FifoAllocatorType::DEFAULT_NOSYNC:
            return spider::make<spider::sched::NoSyncFifoAllocator, StackID::RUNTIME>();
        case spider::FifoAllocatorType::ARCHI_AWARE:
            break;
        default:
            throwSpiderException("unsupported type of FifoAllocator.");
    }
    return nullptr;
}

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
                                                      FifoAllocatorType allocatorType) :
        scheduler_{ spider::make_unique(allocateScheduler(schedulingPolicy)) },
        mapper_{ spider::make_unique(allocateMapper(mappingPolicy)) },
        schedule_{ spider::make_unique<Schedule, StackID::SCHEDULE>() },
        allocator_{ spider::make_unique(makeFifoAllocator(allocatorType)) },
        executionPolicy_{ executionPolicy } {
    if (allocator_) {
        checkFifoAllocatorTraits(allocator_.get(), executionPolicy);
    }
}

void spider::sched::ResourcesAllocator::execute(const pisdf::Graph *graph) {
    /* == Schedule the graph == */
    scheduler_->schedule(graph);

    /* == Map and execute the scheduled tasks == */
    mapper_->setStartTime(computeMinStartTime());
    switch (executionPolicy_) {
        case ExecutionPolicy::JIT:
            jitExecutionPolicy<TaskVertex *>();
            break;
        case ExecutionPolicy::DELAYED:
            delayedExecutionPolicy<TaskVertex *>();
            break;
        default:
            throwSpiderException("unsupported execution policy.");
    }
}

void spider::sched::ResourcesAllocator::clear() {
    schedule_->clear();
    scheduler_->clear();
}

/* === Private method(s) implementation === */

spider::sched::Scheduler *spider::sched::ResourcesAllocator::allocateScheduler(SchedulingPolicy policy) {
    switch (policy) {
        case SchedulingPolicy::LIST:
            return spider::make<sched::ListScheduler, StackID::SCHEDULE>();
        case SchedulingPolicy::GREEDY:
            return spider::make<sched::GreedyScheduler, StackID::SCHEDULE>();
        default:
            throwSpiderException("unsupported scheduling policy.");
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

template<class T>
void spider::sched::ResourcesAllocator::jitExecutionPolicy() {
    /* == Map, allocate fifos and execute tasks == */
    for (auto &task : scheduler_->tasks()) {
        /* == Map the task == */
        mapper_->map(static_cast<T>(task.get()), schedule_.get());

        /* == We are in JIT mode, we need to broadcast the job stamp == */
        task->enableBroadcast();

        /* == Allocate the fifos task == */
        allocator_->allocate(task.get());

        /* == Execute the task == */
        schedule_->addTask(std::move(task));
        schedule_->sendReadyTasks();
    }
}

template<class T>
void spider::sched::ResourcesAllocator::delayedExecutionPolicy() {
    /* == Map every tasks == */
    for (auto &task : scheduler_->tasks()) {
        auto castTask = static_cast<T>(task.get());
        mapper_->map(castTask, schedule_.get());
        schedule_->addTask(std::move(task));
    }

    /* == Allocate fifos for every tasks == */
    for (auto *task : schedule_->readyTasks()) {
        allocator_->allocate(task);
    }

    /* == Execute every tasks == */
    schedule_->sendReadyTasks();
}
