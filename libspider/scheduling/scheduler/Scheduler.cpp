/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
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

#include <scheduling/scheduler/Scheduler.h>
#include <scheduling/scheduler/GreedyScheduler.h>
#include <scheduling/scheduler/BestFitScheduler.h>
#include <scheduling/scheduler/RoundRobinScheduler.h>
#include <scheduling/allocator/DefaultFifoAllocator.h>
#include <scheduling/schedule/ScheduleTask.h>
#include <runtime/common/RTKernel.h>
#include <api/archi-api.h>
#include <archi/PE.h>
#include <archi/Platform.h>
#include <archi/MemoryBus.h>
#include <scheduling/scheduler/srdagless/SRLessBestFitScheduler.h>

/* === Static function(s) definition === */

static void checkFifoAllocatorTraits(spider::FifoAllocator *allocator, spider::Scheduler::ScheduleMode mode) {
    switch (mode) {
        case spider::Scheduler::JIT_SEND:
            if (!allocator->traits_.jitAllocator_) {
                throwSpiderException("Using a scheduler in JIT_SEND mode with incompatible fifo allocator.");
            }
            break;
        case spider::Scheduler::DELAYED_SEND:
            if (!allocator->traits_.postSchedulingAllocator_) {
                throwSpiderException("Using a scheduler in DELAYED_SEND mode with incompatible fifo allocator.");
            }
            break;
    }
}

/* === Function(s) definition === */

spider::Scheduler::Scheduler(pisdf::Graph *graph, ScheduleMode mode, FifoAllocator *allocator) :
        graph_{ graph },
        mode_{ mode },
        allocator_{ allocator } {
    if (allocator) {
        checkFifoAllocatorTraits(allocator, mode);
    }
}

void spider::Scheduler::clear() {
    schedule_.clear();
    if (allocator_) {
        allocator_->clear();
    }
}

void spider::Scheduler::setMode(spider::Scheduler::ScheduleMode mode) {
    mode_ = mode;
    if (allocator_) {
        checkFifoAllocatorTraits(allocator_, mode_);
    }
}

void spider::Scheduler::setAllocator(spider::FifoAllocator *allocator) {
    allocator_ = allocator;
    if (allocator_) {
        checkFifoAllocatorTraits(allocator_, mode_);
    }
}

/* === Protected method(s) === */

ufast64 spider::Scheduler::computeMinStartTime(ScheduleTask *task) const {
    ufast64 minimumStartTime = minStartTime_;
    task->setState(TaskState::PENDING);
    for (const auto *dependency : task->dependencies()) {
        if (dependency) {
            minimumStartTime = std::max(minimumStartTime, dependency->endTime());
        }
    }
    return minimumStartTime;
}

template<class SkipPredicate, class TimePredicate>
spider::PE *spider::Scheduler::findBestPEFit(Cluster *cluster,
                                             ufast64 minStartTime,
                                             TimePredicate execTimePredicate,
                                             SkipPredicate skipPredicate) {
    auto bestFitIdleTime = UINT_FAST64_MAX;
    auto bestFitEndTime = UINT_FAST64_MAX;
    PE *foundPE = nullptr;
    for (const auto &pe : cluster->peArray()) {
        if (!pe->enabled() || skipPredicate(pe)) {
            continue;
        }
        const auto readyTime = schedule_.endTime(pe->virtualIx());
        const auto startTime = std::max(readyTime, minStartTime);
        const auto idleTime = startTime - readyTime;
        const auto endTime = startTime + execTimePredicate(pe);
        if (endTime < bestFitEndTime) {
            foundPE = pe;
            bestFitEndTime = endTime;
            bestFitIdleTime = std::min(idleTime, bestFitIdleTime);
        } else if ((endTime == bestFitEndTime) && (idleTime < bestFitIdleTime)) {
            foundPE = pe;
            bestFitEndTime = endTime;
            bestFitIdleTime = idleTime;
        }
    }
    return foundPE;
}

spider::ScheduleTask *spider::Scheduler::insertCommunicationTask(Cluster *cluster,
                                                                 Cluster *distCluster,
                                                                 ufast64 dataSize,
                                                                 ScheduleTask *previousTask,
                                                                 TaskType type,
                                                                 i32 portIx) {
    const auto *bus = archi::platform()->getClusterToClusterMemoryBus(cluster, distCluster);
    const auto busSpeed = type == TaskType::SYNC_SEND ? bus->writeSpeed() : bus->readSpeed();
    const auto *busKernel = type == TaskType::SYNC_SEND ? bus->sendKernel() : bus->receiveKernel();
    const auto comTime = busSpeed / dataSize;

    /* == Search for the first PE able to run the send task == */
    const auto minStartTime = previousTask->endTime();
    auto *mappedPe = findBestPEFit(cluster, minStartTime, [&comTime](PE *) -> ufast64 { return comTime; },
                                   [](PE *) -> bool { return false; });
    if (!mappedPe) {
        throwSpiderException("could not find any processing element to map task.");
    }

    /* == Create the com task == */
    auto *comTask = make<ScheduleTask, StackID::SCHEDULE>(type);
    comTask->setDependency(previousTask, 0);
    comTask->setExecutionConstraint(previousTask->mappedLrt(), previousTask->execIx());
    schedule_.addScheduleTask(comTask);

    /* == Creates the com task information == */
    auto *comTaskInfo = make<ComTaskInformation, StackID::SCHEDULE>();
    comTaskInfo->size_ = dataSize;
    comTaskInfo->kernelIx_ = busKernel->ix();
    comTaskInfo->inputPortIx_ = portIx;
    comTaskInfo->packetIx_ = type == TaskType::SYNC_SEND ? comTask->execIx() : previousTask->execIx();
    comTask->setInternal(comTaskInfo);

    /* == Set job information and update schedule == */
    const auto mappedPeIx{ mappedPe->virtualIx() };
    const auto mappingSt{ std::max(schedule_.endTime(mappedPeIx), minStartTime) };
    const auto mappingEt{ mappingSt + comTime };
    schedule_.updateTaskAndSetReady(static_cast<size_t>(comTask->ix()), mappedPeIx, mappingSt, mappingEt);
    return comTask;
}

void spider::Scheduler::scheduleCommunications(ScheduleTask *task,
                                               vector<DataDependency> &dependencies,
                                               Cluster *cluster) {
    /* == Lamba used to set com task information == */
    for (auto &dependency : dependencies) {
        const auto sendPe = dependency.sender_;
        const auto dataSize = dependency.size_;
        const auto pos = dependency.position_;
        auto *sendCluster = sendPe->cluster();
        if (sendCluster != cluster) {
            /* == Insert send on source cluster == */
            auto *sendTask =
                    insertCommunicationTask(sendCluster, cluster, dataSize, dependency.task_, TaskType::SYNC_SEND, pos);

            /* == Insert receive on mapped cluster == */
            auto *recvTask = insertCommunicationTask(cluster, sendCluster, dataSize, sendTask, TaskType::SYNC_RECEIVE);

            /* == Re-route dependency of the original vertex to the recvTask == */
            task->setDependency(recvTask, static_cast<size_t>(pos));
            const auto currentStartTime = task->startTime();
            if (recvTask->endTime() > currentStartTime) {
                const auto offset = recvTask->endTime() - currentStartTime;
                task->setStartTime(currentStartTime + offset);
                task->setEndTime(task->endTime() + offset);
            }
        }
    }
}

spider::vector<spider::Scheduler::DataDependency>
spider::Scheduler::getDataDependencies(ScheduleTask *task) {
    const auto *vertex = task->vertex();
    const auto *platform = archi::platform();
    auto taskDependenciesIterator{ std::begin(task->dependencies()) };
    auto dataDependencies = factory::vector<DataDependency>(StackID::SCHEDULE);
    dataDependencies.reserve(vertex->inputEdgeCount());
    i32 pos = 0;
    for (auto &edge : vertex->inputEdgeVector()) {
        const auto rate = edge->sinkRateValue();
        if (rate) {
            auto *taskDep{ *taskDependenciesIterator };
            dataDependencies.push_back({ taskDep,
                                         platform->processingElement(taskDep->mappedPe()),
                                         static_cast<ufast64>(rate),
                                         pos });
        }
        taskDependenciesIterator++;
        pos++;
    }
    return dataDependencies;
}

void spider::Scheduler::mapTask(ScheduleTask *task) {
    const auto *vertex = task->vertex();
    if (!vertex) {
        throwSpiderException("can not schedule a task with no vertex.");
    }
    /* == Compute the minimum start time possible for vertex == */
    const auto minStartTime = Scheduler::computeMinStartTime(task);

    /* == Build the data dependency vector in order to compute receive cost == */
    const auto *platform = archi::platform();
    auto dataDependencies = getDataDependencies(task);

    /* == Search for a slave to map the task on */
    const auto *vertexRtConstraints{ vertex->runtimeInformation() };
    auto needToScheduleCom{ false };
    PE *mappingPe{ nullptr };
    auto mappingSt{ UINT_FAST64_MAX };
    auto mappingEt{ UINT_FAST64_MAX };
    auto bestScheduleCost{ UINT_FAST64_MAX };
    for (const auto &cluster : platform->clusters()) {
        /* == Fast check to discard entire cluster == */
        if (!vertexRtConstraints->isClusterMappable(cluster)) {
            continue;
        }
        /* == Find best fit PE for this cluster == */
        auto *foundPE = findBestPEFit(cluster, minStartTime,
                                      [&vertexRtConstraints](PE *pe) -> ufast64 {
                                          return !vertexRtConstraints->timingOnPE(pe);
                                      },
                                      [&vertexRtConstraints](PE *pe) -> bool {
                                          return !vertexRtConstraints->isPEMappable(pe);
                                      });
        if (foundPE) {
            /* == Compute communication cost == */
            ufast64 dataTransfertCost = 0;
            for (auto &dep : dataDependencies) {
                const auto peSrc{ dep.sender_ };
                const auto dataSize{ dep.size_ };
                dataTransfertCost += platform->dataCommunicationCostPEToPE(peSrc, foundPE, dataSize);
            }
            needToScheduleCom |= (dataTransfertCost != 0);
            /* == Check if it is better than previous cluster PE == */
            const auto startTime{ std::max(schedule_.endTime(foundPE->virtualIx()), minStartTime) };
            const auto endTime{ startTime + static_cast<ufast64>(vertexRtConstraints->timingOnPE(foundPE)) };
            const auto scheduleCost{ math::saturateAdd(endTime, dataTransfertCost) };
            if (scheduleCost < bestScheduleCost) {
                mappingPe = foundPE;
                mappingSt = startTime;
                mappingEt = endTime;
            }
        }
    }

    /* == Throw if no possible mapping was found == */
    if (!mappingPe) {
        throwSpiderException("Could not find suitable processing element for vertex: [%s]", vertex->name().c_str());
    }
    if (needToScheduleCom) {
        /* == Schedule communications == */
        task->setStartTime(mappingSt);
        task->setEndTime(mappingEt);
        scheduleCommunications(task, dataDependencies, mappingPe->cluster());
        mappingSt = task->startTime();
        mappingEt = task->endTime();
    }

    /* == Set job information and update schedule == */
    schedule_.updateTaskAndSetReady(static_cast<size_t>(task->ix()), mappingPe->virtualIx(), mappingSt, mappingEt);
}

void spider::Scheduler::allocateTaskMemory(spider::ScheduleTask *task) {
    if (task && allocator_) {
        allocator_->allocate(task);
    }
}

spider::unique_ptr<spider::Scheduler> spider::makeScheduler(SchedulingPolicy algorithm, pisdf::Graph *graph) {
    Scheduler *scheduler = nullptr;
    switch (algorithm) {
        case SchedulingPolicy::LIST_BEST_FIT:
            scheduler = make<BestFitScheduler, StackID::SCHEDULE>(graph);
            break;
        case SchedulingPolicy::LIST_ROUND_ROBIN:
            scheduler = make<RoundRobinScheduler, StackID::SCHEDULE>(graph);
            break;
        case SchedulingPolicy::GREEDY:
            scheduler = make<GreedyScheduler, StackID::SCHEDULE>(graph);
            break;
        default:
            break;
    }
    return make_unique<Scheduler>(scheduler);
}
