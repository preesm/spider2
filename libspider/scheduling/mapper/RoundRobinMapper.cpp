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

#include <scheduling/mapper/RoundRobinMapper.h>
#include <scheduling/task/PiSDFTask.h>
#include <scheduling/schedule/Schedule.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs-tools/numerical/detail/dependenciesImpl.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::RoundRobinMapper::RoundRobinMapper() : Mapper() {
    currentPeIx_ = spider::make_unique(make_n<size_t, StackID::SCHEDULE>(archi::platform()->clusterCount(), 0));
}

void spider::sched::RoundRobinMapper::map(Task *task, Schedule *schedule) {
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

void spider::sched::RoundRobinMapper::map(PiSDFTask *task, Schedule *schedule) {
    if (!task) {
        throwSpiderException("can not map nullptr task.");
    }
    if (task->state() == TaskState::SKIPPED) {
        return;
    }
    task->setState(TaskState::PENDING);
    /* == Map pisdf task with dependencies == */
    mapImpl(task, schedule, task->computeExecDependencies());
}

/* === Private method(s) implementation === */

template<class... Args>
void spider::sched::RoundRobinMapper::mapImpl(Task *task, Schedule *schedule, Args &&... args) {
    /* == Compute the minimum start time possible for the task == */
    const auto minStartTime = Mapper::computeStartTime(task, schedule, std::forward<Args>(args)...);
    /* == Build the data dependency vector in order to compute receive cost == */
    const auto *platform = archi::platform();
    /* == Search for a slave to map the task on */
    const auto &scheduleStats = schedule->stats();
    MappingResult mappingResult{ };
    for (const auto *cluster : platform->clusters()) {
        /* == Find best fit PE for this cluster == */
        const auto *foundPE = findPE(cluster, scheduleStats, task, minStartTime);
        if (foundPE) {
            currentPeIx_[cluster->ix()] = (currentPeIx_[cluster->ix()] + 1u) % cluster->PECount();
            const auto result = Mapper::computeCommunicationCost(task, foundPE, schedule, std::forward<Args>(args)...);
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
            break;
        }
    }
    /* == Throw if no possible mapping was found == */
    if (!mappingResult.mappingPE) {
        throwSpiderException("Could not find suitable processing element for vertex: [%s]", task->name().c_str());
    }
    if (mappingResult.needToAddCommunication) {
        /* == Map communications == */
        Mapper::mapCommunications(mappingResult, task, schedule, std::forward<Args>(args)...);
    }
    schedule->updateTaskAndSetReady(task, mappingResult.mappingPE, mappingResult.startTime, mappingResult.endTime);
}

const spider::PE *spider::sched::RoundRobinMapper::findPE(const Cluster *cluster,
                                                          const Stats &,
                                                          const Task *task,
                                                          ufast64) const {
    const auto clusterIx = cluster->ix();
    const auto *pe = cluster->peArray()[currentPeIx_[clusterIx]];
    size_t count = 0;
    while ((!pe->enabled() || !task->isMappableOnPE(pe)) && count < cluster->PECount()) {
        currentPeIx_[clusterIx] = (currentPeIx_[clusterIx] + 1u) % cluster->PECount();
        pe = cluster->peArray()[currentPeIx_[clusterIx]];
        count++;
    }
    if (!pe->enabled() || !task->isMappableOnPE(pe)) {
        return nullptr;
    }
    return pe;
}
