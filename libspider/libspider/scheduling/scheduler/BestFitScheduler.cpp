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

#include <scheduling/scheduler/BestFitScheduler.h>
#include <spider-api/archi.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/ProcessingElement.h>
#include <spider-api/scenario.h>
#include <graphs/pisdf/ExecVertex.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

Spider::Schedule &Spider::BestFitScheduler::mappingScheduling() {
    schedule_.setJobCount(sortedVertexVector_.size());
    for (auto &listVertex : sortedVertexVector_) {
        vertexMapper(listVertex.vertex);
    }
    return schedule_;
}

void Spider::BestFitScheduler::vertexMapper(const PiSDFAbstractVertex *vertex) {
    /* == Compute the minimum start time possible for vertex == */
    std::uint64_t minStartTime = 0;

    /* == Search for the best slave possible == */
    const auto *platform = Spider::platform();
    const auto &scenario = Spider::scenario();
    const auto *reference = dynamic_cast<const PiSDFVertex *>(vertex->reference());
    const auto &platformStats = schedule_.stats();

    std::pair<std::uint32_t, std::uint32_t> bestSlave{ UINT32_MAX, UINT32_MAX };
    std::uint64_t bestStartTime = 0;
    std::uint64_t bestEndTime = UINT64_MAX;
    std::uint64_t bestScheduleCost = UINT64_MAX;
    for (const auto &cluster : platform->clusters()) {
        for (const auto &PE : cluster->processingElements()) {
            /* == Check that PE is enabled and vertex is mappable on it == */
            if (PE->enabled() && scenario.isMappable(reference, PE)) {
                /* == Retrieving information needed for scheduling cost == */
                const auto &readyTime = platformStats.startTime(PE->spiderPEIx());
                const auto &startTime = std::max(readyTime, minStartTime);
                const auto &waitTime = startTime - readyTime;
                const auto &execTime = scenario.executionTiming(reference, PE);
                const auto &endTime = startTime + execTime;

                /* == Compute communication cost == */
                std::uint64_t receiveCost = 0;

                /* == Compute total schedule cost == */
                const auto &scheduleCost = Spider::Math::saturateAdd(Spider::Math::saturateAdd(endTime, waitTime),
                                                                     receiveCost);
                if (scheduleCost < bestScheduleCost) {
                    bestScheduleCost = scheduleCost;
                    bestStartTime = startTime;
                    bestEndTime = endTime;
                    bestSlave.first = cluster->ix();
                    bestSlave.second = PE->clusterPEIx();
                }
            }
        }
    }

    if (bestSlave.first == UINT32_MAX) {
        throwSpiderException("Could not find suitable processing element for vertex: [%s]", vertex->name());
    }
    const auto &PE = platform->findPE(bestSlave.first, bestSlave.second);
    auto job = ScheduleJob(schedule_.jobCount(), /* == Job ix == */
                           vertex->ix(),         /* == Vertex ix == */
                           PE.clusterPEIx(),     /* == PE ix inside the cluster == */
                           PE.cluster()->ix(),   /* == Cluster ix of the PE == */
                           0);                   /* == LRT ix to which the PE is linked == */
    job.setMappingStartTime(bestStartTime);
    job.setMappingEndTime(bestEndTime);
    job.setMappingLRT(PE.managingLRTIx());
    job.setMappingPE(PE.clusterPEIx(), PE.cluster()->ix());
    schedule_.add(std::move(job));
}
