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
#include <scheduling/schedule/ScheduleVertexTask.h>
#include <runtime/interface/Message.h>
#include <runtime/platform/RTPlatform.h>
#include <api/archi-api.h>
#include <archi/PE.h>
#include <archi/Platform.h>
#include <archi/MemoryBus.h>
#include <scheduling/schedule/ScheduleKernelTask.h>

/* === Function(s) definition === */

spider::Scheduler::Scheduler(spider::pisdf::Graph *graph, ScheduleMode mode) : graph_{ graph }, mode_{ mode } {
}

void spider::Scheduler::clear() {
    schedule_.clear();
}

/* === Protected method(s) === */

ufast64 spider::Scheduler::computeMinStartTime(ScheduleTask *task) {
    ufast64 minimumStartTime = 0;
    task->setState(TaskState::PENDING);
    for (const auto *dependency : task->dependencies()) {
        const auto mappedLRT = dependency->mappedLrt();
        const auto currentJobConstraint = task->executionConstraint(mappedLRT);
        if ((currentJobConstraint < 0) || (dependency->ix() > currentJobConstraint)) {
            task->setExecutionConstraint(mappedLRT, dependency->ix());
        }
        minimumStartTime = std::max(minimumStartTime, dependency->endTime());
    }
    return minimumStartTime;
}

template<class SkipPredicate>
spider::PE *spider::Scheduler::findBestPEFit(Cluster *cluster,
                                             ufast64 minStartTime,
                                             ufast64 execTime,
                                             SkipPredicate skipPredicate) {
    auto bestFitIdleTime = UINT_FAST64_MAX;
    auto bestFitEndTime = UINT_FAST64_MAX;
    PE *foundPE = nullptr;
    for (const auto &pe : cluster->array()) {
        if (!pe->enabled() || skipPredicate(pe)) {
            continue;
        }
        const auto readyTime = schedule_.endTime(pe->virtualIx());
        const auto startTime = std::max(readyTime, minStartTime);
        const auto idleTime = startTime - readyTime;
        const auto endTime = startTime + execTime;
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
                                                                 RTKernel *kernel,
                                                                 ufast64 comTime,
                                                                 ScheduleTask *previousTask,
                                                                 TaskType type) {
    const auto minStartTime = previousTask->endTime();
    /* == Search for the first PE able to run the send task == */
    auto *mappedPE = findBestPEFit(cluster, minStartTime, comTime, [](PE *) -> bool { return false; });
    if (!mappedPE) {
        throwSpiderException("could not find any processing element to map task.");
    }
    const auto mappedPEIx = mappedPE->virtualIx();
    const auto mappingST = std::max(schedule_.endTime(mappedPEIx), minStartTime);
    const auto mappingET = mappingST + comTime;
    /* == Create the send task == */
    auto *comTask = make<ScheduleKernelTask, StackID::SCHEDULE>(kernel, type);
    comTask->setNumberOfDependencies(1);
    comTask->setDependency(previousTask, 0);
    schedule_.addScheduleTask(comTask);
    /* == Set job information and update schedule == */
    schedule_.updateJobAndSetReady(static_cast<size_t>(comTask->ix()), mappedPEIx, mappingST, mappingET);
    return comTask;
}

void spider::Scheduler::scheduleCommunications(ScheduleVertexTask *task,
                                               vector<DataDependency> &dependencies,
                                               Cluster *mappedCluster) {
    auto begin = task->dependencies().begin();
    auto end = task->dependencies().end();
    for (auto &dep : dependencies) {
        const auto peSrc = dep.sender_;
        const auto dataSize = dep.size_;
        auto *srcCluster = peSrc->cluster();
        if (srcCluster != mappedCluster) {
            /* == Add send / receive task and update time if needed == */
            const auto *sendBus = archi::platform()->getClusterToClusterMemoryBus(srcCluster, mappedCluster);
            const auto *recvBus = archi::platform()->getClusterToClusterMemoryBus(mappedCluster, srcCluster);
            /* == Insert send on source cluster == */
            auto *sendKernel = sendBus->sendKernel();
            const auto sendTime = sendBus->writeSpeed() / dataSize;
            auto *sendTask = insertCommunicationTask(srcCluster, sendKernel, sendTime, dep.task_, TaskType::SYNC_SEND);
            /* == Insert receive on mapped cluster == */
            auto *recvKernel = recvBus->receiveKernel();
            const auto recvTime = recvBus->readSpeed() / dataSize;
            auto *recvTask = insertCommunicationTask(mappedCluster, recvKernel, recvTime, sendTask,
                                                     TaskType::SYNC_RECEIVE);
            /* == Set dependency of the original vertex as dependency of recvTask == */
            const auto pos = std::distance(begin, std::find(begin, end, dep.task_));
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
spider::Scheduler::getDataDependencies(ScheduleVertexTask *task) {
    const auto *vertex = task->vertex();
    const auto *platform = archi::platform();
    auto taskDependenciesIterator = task->dependencies().begin();
    auto dataDependencies = factory::vector<DataDependency>(StackID::SCHEDULE);
    dataDependencies.reserve(vertex->inputEdgeCount());
    for (auto &edge : vertex->inputEdgeVector()) {
        const auto rate = edge->sinkRateValue();
        if (rate) {
            auto *taskDep = (*taskDependenciesIterator);
            dataDependencies.emplace_back(taskDep, platform->processingElement(taskDep->mappedPE()), rate);
        }
        taskDependenciesIterator++;
    }
    return dataDependencies;
}

void spider::Scheduler::taskMapper(ScheduleVertexTask *task) {
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
    const auto *vertexRTConstraints = vertex->runtimeInformation();
    bool needToScheduleCom = false;
    PE *mappingPE = nullptr;
    auto mappingST = UINT_FAST64_MAX;
    auto mappingET = UINT_FAST64_MAX;
    auto bestScheduleCost = UINT_FAST64_MAX;
    for (const auto &cluster : platform->clusters()) {
        /* == Fast check to discard entire cluster == */
        if (!vertexRTConstraints->isClusterMappable(cluster)) {
            continue;
        }
        /* == Find best fit PE for this cluster == */
        const auto execTime = vertexRTConstraints->timingOnCluster(cluster, vertex->inputParamVector());
        auto *foundPE = findBestPEFit(cluster, minStartTime, static_cast<ufast64>(execTime),
                                      [&vertexRTConstraints](PE *pe) -> bool {
                                          return !vertexRTConstraints->isPEMappable(pe);
                                      });
        if (foundPE) {
            /* == Compute communication cost == */
            ufast64 dataTransfertCost = 0;
            for (auto &dep : dataDependencies) {
                const auto peSrc = dep.sender_;
                const auto dataSize = dep.size_;
                dataTransfertCost += platform->dataCommunicationCostPEToPE(peSrc, foundPE, dataSize);
            }
            needToScheduleCom |= (dataTransfertCost != 0);
            /* == Check if it is better than previous cluster PE == */
            const auto startTime = std::max(schedule_.endTime(foundPE->virtualIx()), minStartTime);
            const auto endTime = startTime + static_cast<ufast64>(execTime);
            const auto scheduleCost = math::saturateAdd(endTime, dataTransfertCost);
            if (scheduleCost < bestScheduleCost) {
                mappingPE = foundPE;
                mappingST = startTime;
                mappingET = endTime;
            }
        }
    }

    /* == Throw if no possible mapping was found == */
    if (!mappingPE) {
        throwSpiderException("Could not find suitable processing element for vertex: [%s]", vertex->name().c_str());
    }
    if (needToScheduleCom) {
        /* == Schedule communications == */
        task->setStartTime(mappingST);
        task->setEndTime(mappingET);
        scheduleCommunications(task, dataDependencies, mappingPE->cluster());
        mappingST = task->startTime();
        mappingET = task->endTime();
    }

    /* == Set job information and update schedule == */
    schedule_.updateTaskAndSetReady(static_cast<size_t>(task->ix()), mappingPE->virtualIx(), mappingST, mappingET);
}

spider::unique_ptr<spider::Scheduler> spider::makeScheduler(SchedulingAlgorithm algorithm, pisdf::Graph *graph) {
    Scheduler *scheduler = nullptr;
    switch (algorithm) {
        case SchedulingAlgorithm::LIST_BEST_FIT:
            scheduler = make<BestFitScheduler, StackID::SCHEDULE>(graph);
            break;
        case SchedulingAlgorithm::LIST_ROUND_ROBIN:
            scheduler = make<RoundRobinScheduler, StackID::SCHEDULE>(graph);
            break;
        case SchedulingAlgorithm::GREEDY:
            scheduler = make<GreedyScheduler, StackID::SCHEDULE>(graph);
            break;
    }
    return make_unique<Scheduler>(scheduler);
}
