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

#include <scheduling/mapper/BestFitMapper.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/task/TaskVertex.h>
#include <scheduling/task/TaskSync.h>
#include <archi/Platform.h>
#include <archi/MemoryBus.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Edge.h>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

void spider::sched::BestFitMapper::map(TaskVertex *task, Schedule *schedule) {
    auto *vertex = task->vertex();
    if (!vertex) {
        throwSpiderException("can not schedule a task with no vertex.");
    }
    task->setState(sched::TaskState::PENDING);
    task->updateTaskExecutionDependencies(schedule);

    /* == Compute the minimum start time possible for the task == */
    const auto minStartTime = computeStartTime(vertex, schedule);

    /* == Build the data dependency vector in order to compute receive cost == */
    const auto *platform = archi::platform();

    /* == Search for a slave to map the task on */
    const auto *constraints{ vertex->runtimeInformation() };
    const auto scheduleStats = schedule->stats();
    MappingResult mappingResult{ };
    for (const auto *cluster : platform->clusters()) {
        /* == Fast check to discard entire cluster == */
        if (!constraints->isClusterMappable(cluster)) {
            continue;
        }
        /* == Find best fit PE for this cluster == */
        const auto *foundPE = findBestFitPE(cluster, scheduleStats, minStartTime,
                                            [constraints](const PE *pe) { return constraints->isPEMappable(pe); },
                                            [constraints](const PE *pe) {
                                                return static_cast<u64>(constraints->timingOnPE(pe));
                                            });

        if (foundPE) {
            const auto result = computeComputationCost(vertex, foundPE, schedule);
            const auto communicationCost = result.first;
            const auto dataToSend = result.second;
            mappingResult.needToAddCommunication |= (dataToSend != 0);
            /* == Check if it is better than previous cluster PE == */
            const auto startTime{ std::max(scheduleStats.endTime(foundPE->virtualIx()), minStartTime) };
            const auto endTime{ startTime + static_cast<ufast64>(constraints->timingOnPE(foundPE)) };
            const auto scheduleCost{ math::saturateAdd(endTime, communicationCost) };
            if (scheduleCost < mappingResult.scheduleCost) {
                mappingResult.mappingPE = foundPE;
                mappingResult.startTime = startTime;
                mappingResult.endTime = endTime;
                mappingResult.scheduleCost = scheduleCost;
            }
        }
    }

    /* == Throw if no possible mapping was found == */
    if (!mappingResult.mappingPE) {
        throwSpiderException("Could not find suitable processing element for vertex: [%s]", vertex->name().c_str());
    }

    if (mappingResult.needToAddCommunication) {
        /* == Map communications == */
        task->setStartTime(mappingResult.startTime);
        task->setEndTime(mappingResult.endTime);
        mapCommunications(task, mappingResult.mappingPE->cluster(), schedule);
        mappingResult.startTime = task->startTime();
        mappingResult.endTime = task->endTime();
    }
    schedule->updateTaskAndSetReady(task, mappingResult.mappingPE->virtualIx(),
                                    mappingResult.startTime, mappingResult.endTime);
}

ufast64 spider::sched::BestFitMapper::computeStartTime(const pisdf::Vertex *vertex, Schedule *schedule) const {
    auto minTime = startTime_;
    if (vertex) {
        for (const auto *edge : vertex->inputEdgeVector()) {
            const auto source = edge->source();
            if (source && source->executable()) {
                const auto &task = schedule->tasks()[source->scheduleTaskIx()];
                minTime = std::max(minTime, task->endTime());
            }
        }
    }
    return minTime;
}

const spider::PE *spider::sched::BestFitMapper::findBestFitPE(const Cluster *cluster,
                                                              const Stats &stats,
                                                              const ufast64 minStartTime,
                                                              const std::function<bool(const PE *)> &isPEMappable,
                                                              const std::function<u64(const PE *)> &timingOnPE) const {
    const PE *foundPE = nullptr;
    auto bestFitIdleTime = UINT_FAST64_MAX;
    auto bestFitEndTime = UINT_FAST64_MAX;
    for (const auto *pe : cluster->peArray()) {
        if (!pe->enabled() || !isPEMappable(pe)) {
            continue;
        }
        const auto readyTime = stats.endTime(pe->virtualIx());
        const auto startTime = std::max(readyTime, minStartTime);
        const auto idleTime = startTime - readyTime;
        const auto endTime = startTime + timingOnPE(pe);
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

std::pair<ufast64, ufast64>
spider::sched::BestFitMapper::computeComputationCost(const pisdf::Vertex *vertex,
                                                     const PE *mappedPE,
                                                     const Schedule *schedule) {
    const auto *platform = archi::platform();
    ufast64 dataToSend = 0U;
    ufast64 communicationCost = 0;
    /* == Compute communication cost == */
    for (const auto &edge : vertex->inputEdgeVector()) {
        const auto rate = static_cast<u64>(edge->sourceRateValue());
        const auto source = edge->source();
        if (rate && source && source->executable()) {
            const auto &taskSource = schedule->tasks()[source->scheduleTaskIx()];
            const auto mappedPESource = taskSource->mappedPe();
            communicationCost += platform->dataCommunicationCostPEToPE(mappedPESource, mappedPE, rate);
            if (mappedPE->cluster() != mappedPESource->cluster()) {
                dataToSend += rate;
            }
        }
    }
    return { communicationCost, dataToSend };
}

void spider::sched::BestFitMapper::mapCommunications(TaskVertex *task,
                                                     const Cluster *cluster,
                                                     Schedule *schedule) {
    const auto *vertex = task->vertex();
    for (const auto *edge : vertex->inputEdgeVector()) {
        const auto *source = edge->source();
        const auto rate = static_cast<u64>(edge->sinkRateValue());
        if (rate && source && source->executable()) {
            const auto &sourceTask = schedule->tasks()[source->scheduleTaskIx()];
            const auto *srcCluster = sourceTask->mappedPe()->cluster();
            if (cluster != srcCluster) {
                /* == Insert send on source cluster == */
                auto *sendTask = insertCommunicationTask(srcCluster, cluster, rate, task, SyncType::SEND, schedule);
                sendTask->setInputPortIx(static_cast<u32>(edge->sourcePortIx()));
                /* == Insert receive on mapped cluster == */
                auto *recvTask = insertCommunicationTask(cluster, srcCluster, rate, sendTask, SyncType::RECEIVE,
                                                         schedule);
                sendTask->setSuccessor(recvTask);
                /* == Re-route dependency of the original vertex to the recvTask == */
                task->setExecutionDependency(edge->sinkPortIx(), recvTask);
                const auto currentStartTime = task->startTime();
                if (recvTask->endTime() > currentStartTime) {
                    const auto offset = recvTask->endTime() - currentStartTime;
                    task->setStartTime(currentStartTime + offset);
                    task->setEndTime(task->endTime() + offset);
                }
                schedule->addTask(spider::make_unique(sendTask));
                schedule->addTask(spider::make_unique(recvTask));
            }
        }
    }
}

spider::sched::TaskSync *spider::sched::BestFitMapper::insertCommunicationTask(const Cluster *cluster,
                                                                               const Cluster *distCluster,
                                                                               ufast64 dataSize,
                                                                               Task *previousTask,
                                                                               SyncType type,
                                                                               Schedule *schedule) {
    const auto *bus = archi::platform()->getClusterToClusterMemoryBus(cluster, distCluster);
    const auto busSpeed = type == SyncType::SEND ? bus->writeSpeed() : bus->readSpeed();
    const auto comTime = static_cast<i64>(busSpeed / dataSize);

    /* == Search for the first PE able to run the send task == */
    const auto minStartTime = previousTask->endTime();
    const auto *mappedPe = findBestFitPE(cluster,
                                         schedule->stats(),
                                         minStartTime,
                                         [](const PE *) { return true; },
                                         [comTime](const PE *) { return static_cast<u64>(comTime); });
    if (!mappedPe) {
        throwSpiderException("could not find any processing element to map communication task.");
    }

    /* == Create the com task == */
    auto *comTask = make<TaskSync, StackID::SCHEDULE>(type);
    comTask->setExecutionDependency(0, previousTask);

    /* == Creates the com task information == */
    comTask->setSize(dataSize);
    comTask->setMemoryBus(bus);

    /* == Set job information and update schedule == */
    const auto mappedPeIx{ mappedPe->virtualIx() };
    const auto mappingSt{ std::max(schedule->stats().endTime(mappedPeIx), minStartTime) };
    const auto mappingEt{ mappingSt + static_cast<u64>(comTime) };
    schedule->updateTaskAndSetReady(comTask, mappedPeIx, mappingSt, mappingEt);
    return comTask;
}
