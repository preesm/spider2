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
#include <api/archi-api.h>
#include <archi/PE.h>
#include <runtime/interface/Message.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/runner/RTRunner.h>
#include <api/runtime-api.h>

/* === Function(s) definition === */

spider::Scheduler::Scheduler(spider::pisdf::Graph *graph, FifoAllocatorType type) : graph_{ graph } {
    if (type == FifoAllocatorType::DEFAULT) {
        fifoAllocator_ = make<DefaultFifoAllocator, StackID::SCHEDULE>();
    } else {
        throwSpiderException("Unsupported type of FifoAllocator.");
    }
}

spider::Scheduler::~Scheduler() {
    destroy(fifoAllocator_);
}

void spider::Scheduler::clear() {
    schedule_.clear();
    fifoAllocator_->clear();
}

/* === Protected method(s) === */

uint64_t spider::Scheduler::computeMinStartTime(const pisdf::Vertex *vertex) {
    uint64_t minimumStartTime = 0;
    auto &job = schedule_.job(vertex->scheduleJobIx());
    job.setState(sched::JobState::PENDING);
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
    return spider::make_unique<spider::Scheduler>(scheduler);
}
