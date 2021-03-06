/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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

#include <scheduling/mapper/Mapper.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/task/Task.h>
#include <scheduling/task/PiSDFTask.h>
#include <scheduling/task/SyncTask.h>
#include <archi/PE.h>
#include <api/archi-api.h>
#include <graphs-tools/numerical/detail/dependenciesImpl.h>

/* === Static variable(s) === */

/* === Method(s) implementation === */

void spider::sched::Mapper::map(Task *task, Schedule *schedule) {
    if (!task) {
        throwSpiderException("can not map nullptr task.");
    }
    if (task->state() == TaskState::SKIPPED) {
        return;
    }
    task->setState(TaskState::PENDING);
    /* == Map standard task == */
    mapImpl(task, schedule);
}

void spider::sched::Mapper::map(PiSDFTask *task, Schedule *schedule) {
    if (!task) {
        throwSpiderException("can not map nullptr task.");
    }
    if (task->state() == TaskState::SKIPPED) {
        return;
    }
    task->setState(TaskState::PENDING);
    /* == Map pisdf task with dependencies == */
    mapImpl(task, schedule);
}

/* === Private method(s) implementation === */

template<class T>
void spider::sched::Mapper::mapImpl(T *task, Schedule *schedule) {
    auto comRates = spider::make_unique(make_n<u32>(archi::platform()->LRTCount(), 0));
    if (!comRates) {
        throwNullptrException();
    }
    /* == Compute the minimum start time possible for the task == */
    const auto minStartTime = computeStartTime(task, schedule, comRates.get());
    /* == Build the data dependency vector in order to compute receive cost == */
    const auto *platform = archi::platform();
    /* == Search for a slave to map the task on */
    const auto &scheduleStats = schedule->stats();
    MappingResult mappingResult{ };
    for (const auto *cluster : platform->clusters()) {
        /* == Find best fit PE for this cluster == */
        const auto *foundPE = findPE(cluster, scheduleStats, task, minStartTime);
        if (foundPE) {
            const auto result = computeCommunicationCost(task, foundPE, schedule, comRates.get());
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
        mapCommunications(mappingResult, task, schedule);
    }
    schedule->updateTaskAndSetReady(task, mappingResult.mappingPE, mappingResult.startTime, mappingResult.endTime);
}

ufast64 spider::sched::Mapper::computeStartTime(Task *task, const Schedule *schedule, u32 *comRates) const {
    auto minTime = startTime_;
    if (!task) {
        return minTime;
    }
    for (size_t ix = 0; ix < task->dependencyCount(); ++ix) {
        const auto *srcTask = task->previousTask(ix, schedule);
        if (srcTask) {
            const auto srcLRTIx = srcTask->mappedLRT()->virtualIx();
            const auto currentJob = task->syncExecIxOnLRT(srcLRTIx);
            const auto srcJobIx = srcTask->ix();
            if (currentJob == UINT32_MAX || srcJobIx > currentJob) {
                task->setSyncExecIxOnLRT(srcLRTIx, srcJobIx);
            }
            /* == By summing up all the rates we are sure to compute com cost accurately == */
            comRates[srcLRTIx] += static_cast<u32>(task->inputRate(ix));
            minTime = std::max(minTime, srcTask->endTime());
        }
    }
    return minTime;
}

ufast64 spider::sched::Mapper::computeStartTime(PiSDFTask *task, const Schedule *schedule, u32 *comRates) const {
    auto minTime = startTime_;
    if (!task) {
        return minTime;
    }
    const auto *vertex = task->vertex();
    const auto *handler = task->handler();
    const auto lambda = [task, schedule, comRates, &minTime](const pisdf::DependencyInfo &dep) {
        if (!dep.vertex_ || !dep.handler_) {
            return;
        }
        const auto firing = task->firing();
        const auto *srcTaskIxArray = dep.handler_->getTaskIndexes(dep.vertex_);
        for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
            const auto srcTaskIx = srcTaskIxArray[k];
            const auto *srcTask = schedule->task(srcTaskIxArray[k]);
            if (srcTask) {
                const auto srcLRTIx = srcTask->mappedLRT()->virtualIx();
                const auto srcEndTime = srcTask->endTime();
                task->setOnFiring(firing);
                const auto currentJob = task->syncExecIxOnLRT(srcLRTIx);
                if (currentJob == UINT32_MAX || srcTaskIx > currentJob) {
                    task->setSyncExecIxOnLRT(srcLRTIx, srcTaskIx);
                }
                /* == By summing up all the rates we are sure to compute com cost accurately == */
                const auto memoryStart = (k == dep.firingStart_) * dep.memoryStart_;
                const auto memoryEnd = k == dep.firingEnd_ ? dep.memoryEnd_ : static_cast<u32>(dep.rate_) - 1;
                comRates[srcLRTIx] += (dep.rate_ > 0) * (memoryEnd - memoryStart + 1);
                minTime = std::max(minTime, srcEndTime);
            }
        }
    };
    const auto firing = task->firing();
    for (const auto *edge : vertex->inputEdges()) {
        pisdf::detail::computeExecDependency(handler, edge, firing, lambda);
    }
    return minTime;
}

std::pair<ufast64, ufast64> spider::sched::Mapper::computeCommunicationCost(Task *task,
                                                                            const PE *mappedPE,
                                                                            const Schedule *schedule,
                                                                            const u32 *comRates) {
    /* == Compute communication cost == */
    ufast64 externDataToReceive = 0u;
    ufast64 communicationCost = 0;
    const auto *platform = archi::platform();
    const auto lrtCount = platform->LRTCount();
    const auto firing = task->firing();
    for (size_t i = 0; i < lrtCount; ++i) {
        const auto srcTaskIx = task->syncExecIxOnLRT(i);
        const auto rate = comRates[i];
        if (srcTaskIx != UINT32_MAX) {
            const auto *srcTask = schedule->task(srcTaskIx);
            if (srcTask && srcTask->state() != TaskState::NOT_RUNNABLE) {
                const auto *mappedPESource = srcTask->mappedPe();
                communicationCost += platform->dataCommunicationCostPEToPE(mappedPESource, mappedPE, rate);
                if (mappedPE->cluster() != mappedPESource->cluster()) {
                    externDataToReceive += rate;
                }
            }
            task->setOnFiring(firing);
        }
    }
    return { communicationCost, externDataToReceive };
}

void spider::sched::Mapper::mapCommunications(MappingResult &mappingInfo, Task *task, Schedule *schedule) const {
    for (size_t ix = 0; ix < task->dependencyCount(); ++ix) {
        auto *srcTask = task->previousTask(ix, schedule);
        mapCommunications(mappingInfo, task, srcTask, ix, schedule);
    }
}

void spider::sched::Mapper::mapCommunications(MappingResult &mappingInfo, PiSDFTask *task, Schedule *schedule) const {
    size_t depIx = 0;
    const auto lambda = [&depIx, &mappingInfo, schedule, task, this](const pisdf::DependencyInfo &dep) {
        if (!dep.handler_ || !dep.vertex_) {
            return;
        }
        for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
            auto *srcTask = schedule->task(dep.handler_->getTaskIx(dep.vertex_, k));
            mapCommunications(mappingInfo, task, srcTask, depIx, schedule);
            depIx++;
        }
    };
    const auto *vertex = task->vertex();
    const auto firing = task->firing();
    const auto *handler = task->handler();
    for (const auto *edge : vertex->inputEdges()) {
        pisdf::detail::computeExecDependency(handler, edge, firing, lambda);
    }

}

void spider::sched::Mapper::mapCommunications(MappingResult &mappingInfo,
                                              Task *task,
                                              Task *srcTask,
                                              size_t depIx,
                                              Schedule *schedule) const {
    if (!srcTask) {
        return;
    }
    const auto *mappedCluster = mappingInfo.mappingPE->cluster();
    const auto *prevCluster = srcTask->mappedPe()->cluster();
    if (prevCluster != mappedCluster) {
        /* == Insert send on source cluster == */
        const auto *sndBus = archi::platform()->getClusterToClusterMemoryBus(prevCluster, mappedCluster);
        /* == Create the com task == */
        auto *sndTask = spider::make<SyncTask, StackID::SCHEDULE>(SyncType::SEND, sndBus);
        /* == Search for the first PE able to run the send task == */
        auto minStartTime = srcTask->endTime();
        auto *mappedPe = this->findPE(prevCluster, schedule->stats(), sndTask, minStartTime);
        if (!mappedPe) {
            throwSpiderException("could not find any processing element to map communication vertexTask.");
        }
        /* == Set job information and update schedule == */
        auto mappedPeIx{ mappedPe->virtualIx() };
        auto mappingSt{ std::max(schedule->stats().endTime(mappedPeIx), minStartTime) };
        auto mappingEt{ mappingSt + sndTask->timingOnPE(nullptr) };
        schedule->updateTaskAndSetReady(sndTask, mappedPe, mappingSt, mappingEt);
        /* == Insert receive on mapped cluster == */
        const auto *rcvBus = archi::platform()->getClusterToClusterMemoryBus(mappedCluster, prevCluster);
        auto *rcvTask = spider::make<SyncTask, StackID::SCHEDULE>(SyncType::RECEIVE, rcvBus);
        /* == Search for the first PE able to run the send task == */
        minStartTime = sndTask->endTime();
        mappedPe = this->findPE(mappedCluster, schedule->stats(), rcvTask, minStartTime);
        if (!mappedPe) {
            throwSpiderException("could not find any processing element to map communication vertexTask.");
        }
        /* == Set job information and update schedule == */
        mappedPeIx = mappedPe->virtualIx();
        mappingSt = std::max(schedule->stats().endTime(mappedPeIx), minStartTime);
        mappingEt = mappingSt + rcvTask->timingOnPE(nullptr);
        schedule->updateTaskAndSetReady(rcvTask, mappedPe, mappingSt, mappingEt);
        schedule->insertTasks(task->ix(), { ComposedTask{ sndTask, 0 }, ComposedTask{ rcvTask, 0 }});
        /* == Re-route dependency of the original vertex to the recvTask == */
        if (rcvTask->endTime() > mappingInfo.startTime) {
            const auto offset = rcvTask->endTime() - mappingInfo.startTime;
            mappingInfo.startTime += offset;
            mappingInfo.endTime += +offset;
        }
        /* == Set dependencies == */
        sndTask->setPredecessor(srcTask);
        sndTask->setSuccessor(rcvTask);
        sndTask->setDepIx(static_cast<u32>(depIx));
        rcvTask->setPredecessor(sndTask);
        rcvTask->setSuccessor(task);
        rcvTask->setDepIx(static_cast<u32>(depIx));
    }
}
