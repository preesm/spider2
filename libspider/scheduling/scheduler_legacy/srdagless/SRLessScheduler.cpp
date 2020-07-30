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

#include <scheduling/scheduler_legacy/srdagless/SRLessScheduler.h>
#include <scheduling/scheduler_legacy/srdagless/SRLessBestFitScheduler.h>

/* === Function(s) definition === */


spider::unique_ptr<spider::SRLessScheduler>
spider::makeSRLessScheduler(pisdf::Graph *graph, SchedulingPolicy algorithm) {
    SRLessScheduler *scheduler = nullptr;
    switch (algorithm) {
        case SchedulingPolicy::LIST:
            scheduler = make<SRLessBestFitScheduler, StackID::SCHEDULE>(graph);
            break;
//        case SchedulingAlgorithm::GREEDY:
//            scheduler = make<GreedyScheduler, StackID::SCHEDULE>(graph);
//            break;
        default:
            break;
    }
    return spider::unique_ptr<spider::SRLessScheduler>(scheduler);
}

void spider::SRLessScheduler::mapTask(ScheduleTask *task) {
    const auto *vertex = task->vertex();
    if (!vertex) {
        throwSpiderException("can not schedule a task with no vertex.");
    }
    /* == Compute the minimum start time possible for vertex == */
    const auto minStartTime = SchedulerLegacy::computeMinStartTime(task);

    /* == Search for a slave to map the task on */
    const auto *platform = archi::platform();
    const auto *vertexRtConstraints{ vertex->runtimeInformation() };
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
        auto *foundPE = findBestPEFit(cluster, minStartTime, vertexRtConstraints,
                                      [](const PE *pe, const void *info) -> i64 {
                                          return reinterpret_cast<const RTInfo *>(info)->timingOnPE(pe);
                                      },
                                      [](const PE *pe, const void *info) -> bool {
                                          return !reinterpret_cast<const RTInfo *>(info)->isPEMappable(pe);
                                      });

        if (foundPE) {
            /* == Check if it is better than previous cluster PE == */
            const auto startTime{ std::max(schedule_.endTime(foundPE->virtualIx()), minStartTime) };
            const auto endTime{ startTime + static_cast<ufast64>(vertexRtConstraints->timingOnPE(foundPE)) };
            const auto scheduleCost{ endTime };
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
    /* == Set job information and update schedule == */
    schedule_.updateTaskAndSetReady(static_cast<size_t>(task->ix()), mappingPe->virtualIx(), mappingSt, mappingEt);
}
