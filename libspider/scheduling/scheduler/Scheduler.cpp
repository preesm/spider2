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

spider::Scheduler::Scheduler(spider::pisdf::Graph *graph) : graph_{ graph } {
}

void spider::Scheduler::clear() {
    schedule_.clear();
}

/* === Protected method(s) === */

uint64_t spider::Scheduler::computeMinStartTime(const pisdf::Vertex *vertex) {
    uint64_t minimumStartTime = 0;
    auto &job = schedule_.job(vertex->scheduleJobIx());
    job.setState(JobState::PENDING);
    job.setVertex(vertex);
    for (const auto &edge : vertex->inputEdgeVector()) {
        const auto &rate = edge->sourceRateValue();
        if (rate) {
            const auto &src = edge->source();
            auto &srcJob = schedule_.job(src->scheduleJobIx());
            const auto &srcJobLRTIx = srcJob.LRTIx();
            const auto &currentConstraintIx = job.scheduleJobConstraintOnLRT(srcJobLRTIx);
            if (currentConstraintIx == SIZE_MAX || (srcJob.ix() > currentConstraintIx)) {
                job.setScheduleConstraint(srcJob.ix(), srcJobLRTIx);
            }
            minimumStartTime = std::max(minimumStartTime, srcJob.endTime());
        }
    }
    return minimumStartTime;
}

void spider::Scheduler::vertexMapper(const pisdf::Vertex *vertex) {
    /* == Compute the minimum start time possible for vertex == */
    uint64_t minStartTime = Scheduler::computeMinStartTime(vertex);

    /* == Build the data dependency vector in order to compute receive cost == */
    const auto *platform = archi::platform();
    auto dataDependencies = factory::vector<std::pair<PE *, uint64_t >>(StackID::SCHEDULE);
    dataDependencies.reserve(vertex->inputEdgeCount());
    for (auto &edge : vertex->inputEdgeVector()) {
        const auto &rate = edge->sinkRateValue();
        if (rate) {
            const auto &job = schedule_.job(edge->source()->scheduleJobIx());
            dataDependencies.emplace_back(platform->processingElement(job.PEIx()), rate);
        }
    }

    /* == Search for the best slave possible == */
    const auto *vertexRTConstraints = vertex->runtimeInformation();
    const auto &platformStats = schedule_.stats();
    size_t bestSlave = SIZE_MAX;
    uint64_t bestStartTime = 0;
    uint64_t bestEndTime = UINT64_MAX;
    uint64_t bestWaitTime = UINT64_MAX;
    uint64_t bestScheduleCost = UINT64_MAX;
    for (const auto &cluster : platform->clusters()) {
        /* == Fast check to discard entire cluster == */
        if (!vertexRTConstraints->isClusterMappable(cluster)) {
            continue;
        }
        for (const auto &pe : cluster->array()) {
            /* == Check that PE is enabled and vertex is mappable on it == */
            if (pe->enabled() && vertexRTConstraints->isPEMappable(pe)) {
                /* == Retrieving information needed for scheduling cost == */
                const auto &PEReadyTime = platformStats.endTime(pe->virtualIx());
                const auto &JobStartTime = std::max(PEReadyTime, minStartTime);
                const auto &waitTime = JobStartTime - PEReadyTime;
                const auto &execTime = vertexRTConstraints->timingOnPE(pe, vertex->inputParamVector());
                const auto &endTime = static_cast<uint64_t>(execTime) + JobStartTime;

                /* == Compute communication cost == */
                uint64_t dataTransfertCost = 0;
                for (auto &dep : dataDependencies) {
                    const auto &src = dep.first;
                    const auto &dataExchanged = dep.second;
                    dataTransfertCost += platform->dataCommunicationCostPEToPE(src, pe, dataExchanged);
                }

                /* == Compute total schedule cost == */
                const auto &scheduleCost = math::saturateAdd(endTime, dataTransfertCost);
                if (scheduleCost < bestScheduleCost || (scheduleCost == bestScheduleCost && waitTime < bestWaitTime)) {
                    bestScheduleCost = scheduleCost;
                    bestStartTime = JobStartTime;
                    bestEndTime = endTime;
                    bestWaitTime = waitTime;
                    bestSlave = pe->virtualIx();
                }
            }
        }
    }

    if (bestSlave == SIZE_MAX) {
        throwSpiderException("Could not find suitable processing element for vertex: [%s]", vertex->name().c_str());
    }
    /* == Set job information and update schedule == */
    schedule_.updateJobAndSetReady(vertex->scheduleJobIx(), bestSlave, bestStartTime, bestEndTime);
}

uint64_t spider::Scheduler::computeMinStartTime(ScheduleTask *task) {
    uint64_t minimumStartTime = 0;
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
                                             uint_fast64_t minStartTime,
                                             uint_fast64_t execTime,
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
        } else if ((endTime == bestFitEndTime) && (idleTime < bestFitIdleTime)) {
            foundPE = pe;
            bestFitEndTime = endTime;
            bestFitIdleTime = idleTime;
        }
    }
    return foundPE;
}

spider::ScheduleTask *spider::Scheduler::insertSendTask(Cluster *cluster,
                                                        RTKernel *kernel,
                                                        uint64_t comTime,
                                                        ScheduleTask *taskToUpdate) {
    const auto minStartTime = taskToUpdate->endTime();
    /* == Search for the first PE able to run the send task == */
    auto *mappedPE = findBestPEFit(cluster, minStartTime, comTime, [](PE *) -> bool { return false; });
    if (!mappedPE) {
        throwSpiderException("could not find any processing element to map task.");
    }
    const auto mappedPEIx = mappedPE->virtualIx();
    const auto mappingST = std::max(schedule_.endTime(mappedPEIx), minStartTime);
    const auto mappingET = mappingST + comTime;
    /* == Create the send task == */
    auto *newTask = make<ScheduleKernelTask, StackID::SCHEDULE>(kernel, TaskType::SYNC_SEND);
    newTask->setNumberOfDependencies(1);
    newTask->setDependency(taskToUpdate, 0);
    schedule_.addJobToSchedule(nullptr);
    /* == Set job information and update schedule == */
    schedule_.updateJobAndSetReady(static_cast<size_t>(newTask->ix()), mappedPEIx, mappingST, mappingET);
    return newTask;
}


uint64_t spider::Scheduler::insertRecvTask(Cluster *cluster,
                                           RTKernel *kernel,
                                           uint64_t comTime,
                                           size_t pos,
                                           ScheduleTask *taskToUpdate) {
    auto *oldTask = taskToUpdate->dependencies()[pos];
    const auto minStartTime = oldTask->startTime();
    /* == Search for the first PE able to run the send task == */
    auto *mappedPE = findBestPEFit(cluster, minStartTime, comTime, [](PE *) -> bool { return false; });
    if (!mappedPE) {
        throwSpiderException("could not find any processing element to map task.");
    }
    const auto mappedPEIx = mappedPE->virtualIx();
    const auto mappingST = std::max(schedule_.endTime(mappedPEIx), minStartTime);
    const auto mappingET = mappingST + comTime;
    /* == Create the receive task == */
    auto *recvTask = make<ScheduleKernelTask, StackID::SCHEDULE>(kernel, TaskType::SYNC_RECEIVE);
    /* == Set new task as the dependency of the original vertex == */
    recvTask->setNumberOfDependencies(1);
    recvTask->setDependency(oldTask, 0);
    /* == Set dependency of the original vertex as dependency of recvTask == */
    taskToUpdate->setDependency(recvTask, pos);
    const auto currentStartTime = taskToUpdate->startTime();
    if (mappingET > currentStartTime) {
        const auto offset = mappingET - currentStartTime;
        taskToUpdate->setStartTime(currentStartTime + offset);
        taskToUpdate->setEndTime(taskToUpdate->endTime() + offset);
    }
    schedule_.addJobToSchedule(nullptr);
    /* == Set job information and update schedule == */
    schedule_.updateJobAndSetReady(static_cast<size_t>(recvTask->ix()), mappedPEIx, mappingST, mappingET);
    return recvTask->endTime();
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
            auto *sendTask = insertSendTask(srcCluster, sendKernel, sendTime, dep.task_);
            /* == Insert receive on mapped cluster == */
            auto *recvKernel = recvBus->receiveKernel();
            const auto recvTime = recvBus->readSpeed() / dataSize;
            const auto pos = std::distance(begin, std::find(begin, end, dep.task_));
            task->setDependency(sendTask, static_cast<size_t>(pos));
            insertRecvTask(mappedCluster, recvKernel, recvTime, static_cast<size_t>(pos), task);
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
        auto *foundPE = findBestPEFit(cluster, minStartTime, static_cast<uint_fast64_t>(execTime),
                                      [&vertexRTConstraints](PE *pe) -> bool {
                                          return !vertexRTConstraints->isPEMappable(pe);
                                      });
        if (foundPE) {
            /* == Compute communication cost == */
            uint64_t dataTransfertCost = 0;
            for (auto &dep : dataDependencies) {
                const auto peSrc = dep.sender_;
                const auto dataSize = dep.size_;
                dataTransfertCost += platform->dataCommunicationCostPEToPE(peSrc, foundPE, dataSize);
            }
            if (dataTransfertCost) {
                needToScheduleCom = true;
            }
            /* == Check if it is better than previous cluster PE == */
            const auto startTime = std::max(schedule_.endTime(foundPE->virtualIx()), minStartTime);
            const auto endTime = startTime + static_cast<uint_fast64_t>(execTime);
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
    schedule_.updateJobAndSetReady(static_cast<size_t>(task->ix()), mappingPE->virtualIx(), mappingST, mappingET);
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
