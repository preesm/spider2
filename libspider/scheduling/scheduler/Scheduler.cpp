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
#include <scheduling/allocator/DefaultFifoAllocator.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <api/archi-api.h>
#include <archi/PE.h>
#include <runtime/interface/Message.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/runner/RTRunner.h>
#include <api/runtime-api.h>

/* === Function(s) definition === */

spider::Scheduler::Scheduler(spider::pisdf::Graph *graph,
                             const spider::vector<spider::pisdf::Param *> &,
                             FifoAllocatorType type) : graph_{ graph } {
    if (type == FifoAllocatorType::DEFAULT) {
        fifoAllocator_ = make<DefaultFifoAllocator, StackID::SCHEDULE>();
    } else {
        throwSpiderException("Unsupported type of FifoAllocator.");
    }
}

spider::Scheduler::~Scheduler() {
    destroy(fifoAllocator_);
}

void spider::Scheduler::addParameterVector(spider::vector<spider::pisdf::Param *> params) {
    parameterBankVector_.emplace_back(std::move(params));
}

/* === Protected method(s) === */

void
spider::Scheduler::setJobInformation(const pisdf::Vertex *vertex, size_t slave, uint64_t startTime, uint64_t endTime) {
    auto *platform = archi::platform();
    const auto &pe = platform->peFromVirtualIx(slave);
    auto &job = schedule_.job(vertex->scheduleJobIx());
    job.setMappingLRT(pe->attachedLRT()->virtualIx());
    job.setMappingPE(pe->virtualIx());
    job.setMappingStartTime(startTime);
    job.setMappingEndTime(endTime);
    schedule_.update(job);

    /* == Update the JobMessage == */
    auto &message = job.message();
    message.LRTs2Notify_ = spider::array<bool>(archi::platform()->LRTCount(), true);

    /* == Set the kernel ix == */
    message.kernelIx_ = vertex->runtimeInformation()->kernelIx();

    /* == Set the vertex ix == */
    if (vertex->subtype() == pisdf::VertexType::CONFIG) {
        size_t index = 0;
        for (auto &cfg : graph_->configVertices()) {
            if (vertex->ix() == cfg->ix()) {
                break;
            }
            index++;
        }
        message.vertexIx_ = index;
    } else {
        message.vertexIx_ = vertex->ix();
    }

    /* == Create input Fifos == */
    message.inputFifoArray_ = spider::array<RTFifo>(vertex->inputEdgeCount(), StackID::RUNTIME);
    size_t inputIx = 0;
    for (const auto &edge : vertex->inputEdgeVector()) {
        auto &fifo = message.inputFifoArray_[inputIx++];
        const auto &source = edge->source();
        auto &srcJob = schedule_.job(source->scheduleJobIx());
        fifo = srcJob.message().outputFifoArray_[edge->sourcePortIx()];
        // TODO: see for inter-cluster communication how to get proper interface
    }

    /* == Create output Fifos == */
    size_t outputIx = 0;
    message.outputFifoArray_ = spider::array<RTFifo>(vertex->outputEdgeCount(), StackID::RUNTIME);
    const auto &params = parameterBankVector_[vertex->transfoJobIx()];
    for (const auto &edge : vertex->outputEdgeVector()) {
        message.outputFifoArray_[outputIx++] = fifoAllocator_->allocate(
                static_cast<size_t>(edge->sourceRateExpression().evaluate(params)),
                archi::platform()->peFromVirtualIx(job.mappingInfo().PEIx)->cluster()->memoryInterface());
    }

    /* == Create the jobs 2 wait array == */
    const auto &numberOfConstraints = job.numberOfConstraints();
    message.jobs2Wait_ = spider::array<std::pair<size_t, size_t>>(numberOfConstraints, StackID::RUNTIME);
    auto jobIterator = message.jobs2Wait_.begin();
    for (auto &srcJob : job.jobConstraintVector()) {
        if (srcJob) {
            (*(jobIterator++)) = std::make_pair(srcJob->mappingInfo().LRTIx, srcJob->ix());
        }
    }

    /* == Set the number of output parameters to be set == */
    const auto &reference = vertex->reference();
    message.outputParamCount_ = reference->outputParamCount();

    /* == Set the input parameters == */
    message.inputParams_ = spider::array<int64_t>(reference->inputParamCount(), StackID::RUNTIME);
    auto paramIterator = message.inputParams_.begin();
    for (auto &index : reference->inputParamIxVector()) {
        (*(paramIterator++)) = params[index]->value(params);
    }
}

uint64_t spider::Scheduler::computeMinStartTime(const pisdf::Vertex *vertex) {
    uint64_t minimumStartTime = 0;
    auto &job = schedule_.job(vertex->scheduleJobIx());
    job.setState(sched::JobState::PENDING);
    job.setVertexIx(vertex->ix());
    const auto &params = parameterBankVector_[vertex->transfoJobIx()];
    for (const auto &edge : vertex->inputEdgeVector()) {
        const auto &rate = edge->sourceRateExpression().evaluate(params);
        if (rate) {
            const auto &src = edge->source();
            auto &srcJob = schedule_.job(src->scheduleJobIx());
            const auto &lrtIx = srcJob.mappingInfo().LRTIx;
            auto *currentConstraint = job.jobConstraint(lrtIx);
            if (!currentConstraint || (srcJob.ix() > currentConstraint->ix())) {
                job.setJobConstraint(&srcJob);
            }
            minimumStartTime = std::max(minimumStartTime, srcJob.mappingInfo().endTime);
        }
    }
    return minimumStartTime;
}

void spider::Scheduler::vertexMapper(const pisdf::Vertex *vertex) {
    /* == Compute the minimum start time possible for vertex == */
    uint64_t minStartTime = Scheduler::computeMinStartTime(vertex);

    /* == Build the data dependency vector in order to compute receive cost == */
    const auto *platform = archi::platform();
    const auto &params = parameterBankVector_[vertex->transfoJobIx()];
    auto dataDependencies = containers::vector<std::pair<PE *, uint64_t >>(StackID::SCHEDULE);
    dataDependencies.reserve(vertex->inputEdgeCount());
    for (auto &edge : vertex->inputEdgeVector()) {
        const auto &rate = edge->sinkRateExpression().evaluate(params);
        if (rate) {
            const auto &job = schedule_.job(edge->source()->scheduleJobIx());
            dataDependencies.emplace_back(platform->processingElement(job.mappingInfo().PEIx), rate);
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
                const auto &execTime = vertexRTConstraints->timingOnPE(pe, params);
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
    setJobInformation(vertex, bestSlave, bestStartTime, bestEndTime);
}

void spider::Scheduler::clearParameterBank() {
    for (auto &paramVector : parameterBankVector_) {
        for (auto &param : paramVector) {
            if (param && !param->graph()) {
                destroy(param);
            }
        }
        paramVector.clear();
    }
    parameterBankVector_.clear();
}
