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
#include <scenario/Scenario.h>
#include <spider-api/archi.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/ProcessingElement.h>

/* === Function(s) definition === */


void spider::Scheduler::setJobInformation(sched::Job *job,
                                          std::pair<std::uint32_t, std::uint32_t> slave,
                                          std::uint64_t startTime,
                                          std::uint64_t endTime) {
    auto *platform = spider::platform();
    const auto &PE = platform->findPE(slave.first, slave.second);
    job->setMappingLRT(PE.managingLRTIx());
    job->setMappingPE(PE.clusterPEIx(), PE.cluster()->ix());
    job->setMappingStartTime(startTime);
    job->setMappingEndTime(endTime);
    job->setMappingLRT(PE.managingLRTIx());
    job->setMappingPE(PE.clusterPEIx(), PE.cluster()->ix());
    schedule_.update(*job);
}

std::uint64_t spider::Scheduler::computeMinStartTime(const PiSDFAbstractVertex *vertex) {
    std::uint64_t minimumStartTime = 0;
    auto &job = schedule_.job(vertex->ix());
    job.setVertexIx(vertex->ix());
    for (const auto &edge : vertex->inputEdgeArray()) {
        const auto &rate = edge->sinkRateExpression().evaluate(params_);
        if (rate) {
            const auto &src = edge->source();
            auto &srcJob = schedule_.job(src->ix());
            const auto &lrtIx = srcJob.mappingInfo().LRTIx;
            auto *currentConstraint = job.constraint(lrtIx);
            if (!currentConstraint || (srcJob.ix() > currentConstraint->ix())) {
                job.setConstraint(&srcJob);
            }
            minimumStartTime = std::max(minimumStartTime, srcJob.mappingInfo().endTime);
        }
    }
    return minimumStartTime;
}

void spider::Scheduler::vertexMapper(const PiSDFAbstractVertex *vertex) {
    /* == Compute the minimum start time possible for vertex == */
    std::uint64_t minStartTime = Scheduler::computeMinStartTime(vertex);

    /* == Search for the best slave possible == */
    const auto *platform = spider::platform();
    const auto *scenario = vertex->containingGraph()->scenario();
    const auto &platformStats = schedule_.stats();

    std::pair<std::uint32_t, std::uint32_t> bestSlave{ UINT32_MAX, UINT32_MAX };
    std::uint64_t bestStartTime = 0;
    std::uint64_t bestEndTime = UINT64_MAX;
    std::uint64_t bestWaitTime = UINT64_MAX;
    std::uint64_t bestScheduleCost = UINT64_MAX;
    for (const auto &cluster : platform->clusters()) {
        for (const auto &PE : cluster->processingElements()) {
            /* == Check that PE is enabled and vertex is mappable on it == */
            if (PE->enabled() && scenario->isMappable(vertex, PE)) {
                /* == Retrieving information needed for scheduling cost == */
                const auto &PEReadyTime = platformStats.endTime(PE->spiderPEIx());
                const auto &JobStartTime = std::max(PEReadyTime, minStartTime);
                const auto &waitTime = JobStartTime - PEReadyTime;
                const auto &execTime = scenario->executionTiming(vertex, PE);
                const auto &endTime = execTime + JobStartTime;

                /* == Compute communication cost == */
                std::uint64_t receiveCost = 0;

                /* == Compute total schedule cost == */
                const auto &scheduleCost = spider::math::saturateAdd(endTime, receiveCost);
                if (scheduleCost < bestScheduleCost || (scheduleCost == bestScheduleCost && waitTime < bestWaitTime)) {
                    bestScheduleCost = scheduleCost;
                    bestStartTime = JobStartTime;
                    bestEndTime = endTime;
                    bestWaitTime = waitTime;
                    bestSlave.first = cluster->ix();
                    bestSlave.second = PE->clusterPEIx();
                }
            }
        }
    }

    if (bestSlave.first == UINT32_MAX) {
        throwSpiderException("Could not find suitable processing element for vertex: [%s]", vertex->name().c_str());
    }
    /* == Set job information and update schedule == */
    Scheduler::setJobInformation(&schedule_.job(vertex->ix()), std::move(bestSlave), bestStartTime, bestEndTime);
}
