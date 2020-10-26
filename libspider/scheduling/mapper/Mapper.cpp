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

#include <scheduling/mapper/Mapper.h>
#include <scheduling/task/Task.h>
#include <scheduling/schedule/Schedule.h>
#include <graphs-tools/numerical/dependencies.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <archi/Platform.h>
#include <api/archi-api.h>

/* === Static function === */

/* === Method(s) implementation === */

ufast64 spider::sched::Mapper::computeStartTime(const Task *task, const Schedule *schedule) const {
    auto minTime = startTime_;
    if (task) {
        for (size_t ix = 0; ix < task->dependencyCount(); ++ix) {
            const auto *source = task->previousTask(ix, schedule);
            minTime = std::max(minTime, source ? source->endTime() : ufast64{ 0 });
        }
    }
    return minTime;
}

ufast64 spider::sched::Mapper::computeStartTime(const Task *task,
                                                const Schedule *schedule,
                                                const spider::vector<pisdf::DependencyIterator> &dependencies) const {
    auto minTime = startTime_;
    if (!task) {
        return minTime;
    }
    for (const auto &depIt : dependencies) {
        for (const auto &dep : depIt) {
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto *srcTask = schedule->task(dep.handler_->getTaskIx(dep.vertex_, k));
                minTime = std::max(minTime, srcTask ? srcTask->endTime() : ufast64{ 0 });
            }
        }
    }
    return minTime;
}

std::pair<ufast64, ufast64> spider::sched::Mapper::computeCommunicationCost(const Task *task,
                                                                            const PE *mappedPE,
                                                                            const Schedule *schedule) {
    /* == Compute communication cost == */
    ufast64 externDataToReceive = 0u;
    ufast64 communicationCost = 0;
    for (size_t ix = 0; ix < task->dependencyCount(); ++ix) {
        const auto *srcTask = task->previousTask(ix, schedule);
        const auto rate = static_cast<ufast64>(task->inputRate(ix));
        updateCommunicationCost(mappedPE, srcTask, rate, communicationCost, externDataToReceive);
    }
    return { communicationCost, externDataToReceive };
}

std::pair<ufast64, ufast64>
spider::sched::Mapper::computeCommunicationCost(const Task *,
                                                const spider::PE *mappedPE,
                                                const Schedule *schedule,
                                                const spider::vector<pisdf::DependencyIterator> &dependencies) {
    /* == Compute communication cost == */
    ufast64 externDataToReceive = 0u;
    ufast64 communicationCost = 0;
    for (const auto &depIt : dependencies) {
        for (const auto &dep : depIt) {
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto memoryStart = (k == dep.firingStart_) * dep.memoryStart_;
                const auto memoryEnd = k == dep.firingEnd_ ? dep.memoryEnd_ : static_cast<u32>(dep.rate_) - 1;
                const auto *srcTask = schedule->task(dep.handler_->getTaskIx(dep.vertex_, k));
                const auto rate = (dep.rate_ > 0) * (memoryEnd - memoryStart + 1);
                updateCommunicationCost(mappedPE, srcTask, rate, communicationCost, externDataToReceive);
            }
        }
    }
    return { communicationCost, externDataToReceive };
}

void spider::sched::Mapper::updateCommunicationCost(const spider::PE *mappedPE,
                                                    const Task *srcTask,
                                                    ufast64 rate,
                                                    ufast64 &communicationCost,
                                                    ufast64 &externDataToReceive) {
    const auto *platform = archi::platform();
    if (rate && srcTask && srcTask->state() != TaskState::NOT_RUNNABLE) {
        const auto *mappedPESource = srcTask->mappedPe();
        communicationCost += platform->dataCommunicationCostPEToPE(mappedPESource, mappedPE, rate);
        if (mappedPE->cluster() != mappedPESource->cluster()) {
            externDataToReceive += rate;
        }
    }
}
