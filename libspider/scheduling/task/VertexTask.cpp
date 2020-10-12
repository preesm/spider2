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
#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Edge.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <scheduling/task/VertexTask.h>
#include <scheduling/schedule/Schedule.h>
#include <api/runtime-api.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/common/Fifo.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::VertexTask::VertexTask(pisdf::Vertex *vertex) : Task(), vertex_{ vertex } {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    fifos_ = spider::make_shared<AllocatedFifos, StackID::SCHEDULE>(vertex->inputEdgeCount(),
                                                                    vertex->outputEdgeCount());
    dependencies_ = spider::make_unique(allocate<Task *, StackID::SCHEDULE>(vertex->inputEdgeCount()));
    std::fill(dependencies_.get(),
              std::next(dependencies_.get(), static_cast<long>(vertex->inputEdgeCount())), nullptr);
}

void spider::sched::VertexTask::updateTaskExecutionDependencies(const spider::sched::Schedule *schedule) {
    for (const auto *edge : vertex_->inputEdgeVector()) {
        const auto *source = edge->source();
        const auto rate = static_cast<u64>(edge->sinkRateValue());
        if (rate && source && source->executable()) {
            setExecutionDependency(edge->sinkPortIx(), schedule->tasks()[source->scheduleTaskIx()].get());
        }
    }
}

spider::sched::AllocationRule spider::sched::VertexTask::allocationRuleForInputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->inputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto *inputEdge = vertex_->inputEdge(ix);
    const auto rate = static_cast<u32>(inputEdge->sinkRateValue());
    const auto fifoIx = static_cast<u32>(inputEdge->sourcePortIx());
    return { rate, 0u, fifoIx, 0u, SAME_IN, FifoAttribute::RW_OWN };
}

spider::sched::AllocationRule spider::sched::VertexTask::allocationRuleForOutputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->outputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto *edge = vertex_->outputEdge(ix);
    const auto rate = static_cast<u32>(edge->sourceRateValue());
    const auto count = rate ? 1u : 0u;
    switch (vertex_->subtype()) {
        case pisdf::VertexType::FORK:
            if (ix == 0u) {
                return { rate, 0u, 0u, count, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
            } else {
                const auto prevIx = static_cast<u32>(ix - 1);
                const auto *previousEdge = vertex_->outputEdge(prevIx);
                const auto offset = static_cast<u32>(previousEdge->sourceRateValue());
                return { rate, offset, prevIx, count, AllocType::SAME_OUT, FifoAttribute::RW_ONLY };
            }
        case pisdf::VertexType::DUPLICATE:
            return { rate, 0u, 0u, count, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
        case pisdf::VertexType::EXTERN_IN: {
            const auto offset = vertex_->reference()->convertTo<pisdf::ExternInterface>()->bufferIndex();
            return { rate, static_cast<u32>(offset), 0u, count, AllocType::EXT, FifoAttribute::RW_EXT };
        }
        case pisdf::VertexType::REPEAT:
            if (rate == static_cast<size_t>(vertex_->inputEdge(0u)->sourceRateValue())) {
                return { rate, 0u, 0u, count, AllocType::SAME_IN, fifos_->inputFifo(0u).attribute_ };
            }
            break;
        default: {
            const auto *sink = edge->sink();
            if (sink && sink->subtype() == pisdf::VertexType::EXTERN_OUT) {
                const auto offset = sink->reference()->convertTo<pisdf::ExternInterface>()->bufferIndex();
                return { rate, static_cast<u32>(offset), 0u, count, AllocType::EXT, FifoAttribute::RW_EXT };
            }
            break;
        }
    }
    return { rate, 0u, 0u, count, AllocType::NEW, FifoAttribute::RW_OWN };
}

u32 spider::sched::VertexTask::color() const {
    const auto *reference = vertex_->reference();
    const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 3u) * 50 + 100);
    const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 2u) * 50 + 100);
    const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 4u) * 50 + 100);
    return 0u | (red << 16u) | (green << 8u) | (blue);
}

std::string spider::sched::VertexTask::name() const {
    return vertex_->name();
}

bool spider::sched::VertexTask::isSyncOptimizable() const noexcept {
    if (vertex_) {
        return (vertex_->subtype() == pisdf::VertexType::FORK) || (vertex_->subtype() == pisdf::VertexType::DUPLICATE);
    }
    return false;
}

spider::JobMessage spider::sched::VertexTask::createJobMessage() const {
    auto message = Task::createJobMessage();
    /* == Set core properties == */
    message.nParamsOut_ = static_cast<u32>(vertex_->reference()->outputParamCount());
    message.kernelIx_ = static_cast<u32>(vertex_->runtimeInformation()->kernelIx());
    /* == Set the input parameters (if any) == */
    message.inputParams_ = pisdf::buildVertexRuntimeInputParameters(vertex_);
    return message;
}

void spider::sched::VertexTask::setIx(u32 ix) noexcept {
    Task::setIx(ix);
    vertex_->setScheduleTaskIx(ix);
}

std::pair<ufast64, ufast64> spider::sched::VertexTask::computeCommunicationCost(const PE *mappedPE) const {
    const auto *platform = archi::platform();
    ufast64 externDataToReceive = 0u;
    /* == Compute communication cost == */
    ufast64 communicationCost = 0;
    for (const auto &edge : vertex_->inputEdgeVector()) {
        const auto rate = static_cast<u64>(edge->sourceRateValue());
        const auto source = edge->source();
        if (rate && source && source->executable()) {
            const auto *taskSource = previousTask(edge->sinkPortIx());
            const auto *mappedPESource = taskSource->mappedPe();
            communicationCost += platform->dataCommunicationCostPEToPE(mappedPESource, mappedPE, rate);
            if (mappedPE->cluster() != mappedPESource->cluster()) {
                externDataToReceive += rate;
            }
        }
    }
    return { communicationCost, externDataToReceive };
}

spider::sched::DependencyInfo spider::sched::VertexTask::getDependencyInfo(size_t ix) const {
    return { vertex_->inputEdge(ix)->sourcePortIx(),
             static_cast<size_t>(vertex_->inputEdge(ix)->sourceRateValue()) };
}

bool spider::sched::VertexTask::isMappableOnPE(const spider::PE *pe) const {
    return vertex_->runtimeInformation()->isPEMappable(pe);
}

u64 spider::sched::VertexTask::timingOnPE(const spider::PE *pe) const {
    return static_cast<u64>(vertex_->runtimeInformation()->timingOnPE(pe, vertex_->inputParamVector()));
}

size_t spider::sched::VertexTask::dependencyCount() const {
    return vertex_->inputEdgeCount();
}

#endif