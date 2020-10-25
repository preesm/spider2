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
#include <scheduling/task/SyncTask.h>
#include <archi/Platform.h>
#include <archi/MemoryBus.h>

/* === Static function === */

/* === Method(s) implementation === */

void spider::sched::BestFitMapper::map(Task *task, Schedule *schedule) {
    if (!task) {
        throwSpiderException("can not map nullptr task.");
    }
    if (task->state() == TaskState::SKIPPED) {
        return;
    }
    task->setState(TaskState::PENDING);
    /* == Compute the minimum start time possible for the task == */
    const auto minStartTime = Mapper::computeStartTime(task, schedule);
    /* == Build the data dependency vector in order to compute receive cost == */
    const auto *platform = archi::platform();
    /* == Search for a slave to map the task on */
    const auto &scheduleStats = schedule->stats();
    MappingResult mappingResult{ };
    for (const auto *cluster : platform->clusters()) {
        /* == Find best fit PE for this cluster == */
        const auto *foundPE = findBestFitPE(cluster, scheduleStats, task, minStartTime);
        if (foundPE) {
            const auto result = task->computeCommunicationCost(foundPE, schedule);
            const auto communicationCost = result.first;
            const auto externDataToReceive = result.second;
            mappingResult.needToAddCommunication |= (externDataToReceive != 0);
            /* == Check if it is better than previous cluster PE == */
            const auto startTime{ std::max(scheduleStats.endTime(foundPE->virtualIx()), minStartTime) };
            const auto endTime{ startTime + task->timingOnPE(foundPE) };
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
        throwSpiderException("Could not find suitable processing element for vertex: [%s]", task->name().c_str());
    }
    if (mappingResult.needToAddCommunication) {
        /* == Map communications == */
//        vertex->setStartTime(mappingResult.startTime);
//        vertex->setEndTime(mappingResult.endTime);
//        mapCommunications(graph, vertex, mappingResult.mappingPE->cluster(), schedule);
//        mappingResult.startTime = vertex->startTime();
//        mappingResult.endTime = vertex->endTime();
    }
    schedule->updateTaskAndSetReady(task, mappingResult.mappingPE, mappingResult.startTime, mappingResult.endTime);
}

/* === Private method(s) implementation === */

const spider::PE *spider::sched::BestFitMapper::findBestFitPE(const Cluster *cluster,
                                                              const Stats &stats,
                                                              const Task *task,
                                                              ufast64 minStartTime) {
    const PE *foundPE = nullptr;
    auto bestFitIdleTime = UINT_FAST64_MAX;
    auto bestFitEndTime = UINT_FAST64_MAX;
    for (const auto *pe : cluster->peArray()) {
        if (!pe->enabled() || !task->isMappableOnPE(pe)) {
            continue;
        }
        const auto readyTime = stats.endTime(pe->virtualIx());
        const auto startTime = std::max(readyTime, minStartTime);
        const auto idleTime = startTime - readyTime;
        const auto endTime = startTime + task->timingOnPE(pe);
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

void spider::sched::BestFitMapper::mapCommunications(Task *task, const Cluster *cluster, Schedule *schedule) {
    for (size_t ix = 0; ix < task->dependencyCount(); ++ix) {
        auto *srcTask = task->previousTask(ix, schedule);
        if (!srcTask) {
            continue;
        }
        const auto *prevCluster = srcTask->mappedPe()->cluster();
        if (prevCluster != cluster) {
            /* == Insert send on source cluster == */
            const auto *sndBus = archi::platform()->getClusterToClusterMemoryBus(prevCluster, cluster);
            /* == Create the com task == */
            auto *sndTask = spider::make<SyncTask, StackID::SCHEDULE>(SyncType::SEND, sndBus);
            /* == Search for the first PE able to run the send task == */
            auto minStartTime = srcTask->endTime();
            auto *mappedPe = findBestFitPE(prevCluster, schedule->stats(), sndTask, minStartTime);
            if (!mappedPe) {
                throwSpiderException("could not find any processing element to map communication vertexTask.");
            }
            /* == Set job information and update schedule == */
            auto mappedPeIx{ mappedPe->virtualIx() };
            auto mappingSt{ std::max(schedule->stats().endTime(mappedPeIx), minStartTime) };
            auto mappingEt{ mappingSt + sndTask->timingOnPE(nullptr) };
            schedule->addTask(sndTask);
            schedule->updateTaskAndSetReady(sndTask, mappedPe, mappingSt, mappingEt);
            /* == Insert receive on mapped cluster == */
            const auto *rcvBus = archi::platform()->getClusterToClusterMemoryBus(cluster, prevCluster);
            auto *rcvTask = spider::make<SyncTask, StackID::SCHEDULE>(SyncType::RECEIVE, rcvBus);
            /* == Search for the first PE able to run the send task == */
            minStartTime = sndTask->endTime();
            mappedPe = findBestFitPE(cluster, schedule->stats(), rcvTask, minStartTime);
            if (!mappedPe) {
                throwSpiderException("could not find any processing element to map communication vertexTask.");
            }
            /* == Set job information and update schedule == */
            mappedPeIx = mappedPe->virtualIx();
            mappingSt = std::max(schedule->stats().endTime(mappedPeIx), minStartTime);
            mappingEt = mappingSt + rcvTask->timingOnPE(nullptr);
            schedule->addTask(rcvTask);
            schedule->updateTaskAndSetReady(rcvTask, mappedPe, mappingSt, mappingEt);
            /* == Re-route dependency of the original vertex to the recvTask == */
            const auto currentStartTime = task->startTime();
            if (rcvTask->endTime() > currentStartTime) {
                const auto offset = rcvTask->endTime() - currentStartTime;
                task->setStartTime(currentStartTime + offset);
                task->setEndTime(task->endTime() + offset);
            }
            /* == Set dependencies == */
            sndTask->setPredecessor(srcTask);
            sndTask->setSuccessor(rcvTask);
            rcvTask->setPredecessor(sndTask);
            rcvTask->setSuccessor(task);
            task->insertSyncTasks(sndTask, rcvTask, ix, schedule);
        }
    }
}
