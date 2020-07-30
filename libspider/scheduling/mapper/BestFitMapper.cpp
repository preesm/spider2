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
    vertex->setScheduleTaskIx(task->ix());

    /* == Compute the minimum start time possible for the task == */
    const auto minStartTime = computeStartTime(vertex, schedule);

    /* == Build the data dependency vector in order to compute receive cost == */
    const auto *platform = archi::platform();

    /* == Search for a slave to map the task on */
    const auto *constraints{ vertex->runtimeInformation() };
    const PE *bestFitPE{ nullptr };
    auto needToScheduleCom{ false };
    auto bestStartTime{ UINT_FAST64_MAX };
    auto bestEndTime{ UINT_FAST64_MAX };
    auto bestScheduleCost{ UINT_FAST64_MAX };
    for (const auto &cluster : platform->clusters()) {
        /* == Fast check to discard entire cluster == */
        if (!constraints->isClusterMappable(cluster)) {
            continue;
        }
        /* == Find best fit PE for this cluster == */
        PE *foundPE = nullptr;
        auto bestFitIdleTime = UINT_FAST64_MAX;
        auto bestFitEndTime = UINT_FAST64_MAX;
        for (const auto &pe : cluster->peArray()) {
            if (!pe->enabled() || !constraints->isPEMappable(pe)) {
                continue;
            }
            const auto readyTime = schedule->stats().endTime(pe->virtualIx());
            const auto startTime = std::max(readyTime, minStartTime);
            const auto idleTime = startTime - readyTime;
            const auto endTime = startTime + static_cast<u64>(constraints->timingOnPE(pe));
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

        if (foundPE) {
            /* == Data to allocate == */
            ufast64 dataToSend = 0U;
            /* == Compute communication cost == */
            ufast64 communicationCost = 0;
            for (const auto &edge : vertex->inputEdgeVector()) {
                const auto rate = static_cast<u64>(edge->sourceRateValue());
                const auto source = edge->source();
                if (rate && source && source->executable()) {
                    const auto taskSource = schedule->tasks()[source->scheduleTaskIx()].get();
                    const auto mappedPESource = taskSource->mappedPe();
                    communicationCost += platform->dataCommunicationCostPEToPE(mappedPESource, foundPE, rate);
                    if (foundPE->cluster() != mappedPESource->cluster()) {
                        dataToSend += rate;
                    }
                }
            }
            needToScheduleCom |= (dataToSend != 0);
            /* == Check if it is better than previous cluster PE == */
            const auto startTime{ std::max(schedule->stats().endTime(foundPE->virtualIx()), minStartTime) };
            const auto endTime{ startTime + static_cast<ufast64>(constraints->timingOnPE(foundPE)) };
            const auto scheduleCost{ math::saturateAdd(endTime, communicationCost) };
            if (scheduleCost < bestScheduleCost) {
                bestFitPE = foundPE;
                bestStartTime = startTime;
                bestEndTime = endTime;
            }
        }
    }

    /* == Throw if no possible mapping was found == */
    if (!bestFitPE) {
        throwSpiderException("Could not find suitable processing element for vertex: [%s]", vertex->name().c_str());
    }

//    if (needToScheduleCom) {
//        /* == Schedule communications == */
//        task->setStartTime(mappingSt);
//        task->setEndTime(mappingEt);
//        scheduleCommunications(task, dataDependencies, mappingPe->cluster());
//        mappingSt = task->startTime();
//        mappingEt = task->endTime();
//    }
    schedule->updateTaskAndSetReady(task, bestFitPE->virtualIx(), bestStartTime, bestEndTime);
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
