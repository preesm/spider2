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

#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Edge.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <scheduling/task/TaskVertex.h>
#include <scheduling/schedule/Schedule.h>
#include <api/runtime-api.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/common/Fifo.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::TaskVertex::TaskVertex(pisdf::Vertex *vertex) : Task(), vertex_{ vertex } {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    fifos_ = spider::make_shared<AllocatedFifos, StackID::SCHEDULE>(vertex->inputEdgeCount(),
                                                                    vertex->outputEdgeCount());
    execInfo_.dependencies_ = spider::make_unique(allocate<Task *, StackID::SCHEDULE>(vertex->inputEdgeCount()));
    std::fill(execInfo_.dependencies_.get(),
              std::next(execInfo_.dependencies_.get(), static_cast<long>(vertex->inputEdgeCount())), nullptr);
}

void spider::sched::TaskVertex::updateTaskExecutionDependencies(const spider::sched::Schedule *schedule) {
    for (const auto *edge : vertex_->inputEdgeVector()) {
        const auto *source = edge->source();
        const auto rate = static_cast<u64>(edge->sinkRateValue());
        if (rate && source && source->executable()) {
            const auto &sourceTask = schedule->tasks()[source->scheduleTaskIx()];
            execInfo_.dependencies_.get()[edge->sinkPortIx()] = sourceTask.get();
        }
    }
}

void spider::sched::TaskVertex::updateExecutionConstraints() {
    auto *execDependencies = execInfo_.dependencies_.get();
    auto *execConstraints = execInfo_.constraints_.get();
    const auto lrtCount = archi::platform()->LRTCount();
    std::fill(execConstraints, execConstraints + lrtCount, SIZE_MAX);
    auto shouldNotifyArray = array<size_t>(lrtCount, SIZE_MAX, StackID::SCHEDULE);
    // TODO: see how these two loops could be merged into one
    for (size_t i = 0; i < vertex_->inputEdgeCount(); ++i) {
        auto *dependency = execDependencies[i];
        if (dependency) {
            const auto *depLRT = dependency->mappedLRT();
            const auto currentJobConstraint = execConstraints[depLRT->virtualIx()];
            if ((currentJobConstraint == SIZE_MAX) || (dependency->jobExecIx() > currentJobConstraint)) {
                execConstraints[depLRT->virtualIx()] = dependency->jobExecIx();
                shouldNotifyArray[depLRT->virtualIx()] = i;
            }
        }
    }
    for (const auto &value : shouldNotifyArray) {
        if (value != SIZE_MAX) {
            execDependencies[value]->setNotificationFlag(mappedLRT()->virtualIx(), true);
        }
    }
}

spider::sched::AllocationRule spider::sched::TaskVertex::allocationRuleForInputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->inputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto *inputEdge = vertex_->inputEdge(ix);
    const auto size = static_cast<size_t>(inputEdge->sinkRateValue());
    const auto index = static_cast<u32>(inputEdge->sourcePortIx());
    switch (vertex_->subtype()) {
        case pisdf::VertexType::FORK:
            return { nullptr, size, 0u, index, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
        case pisdf::VertexType::DUPLICATE:
            return { nullptr, size, 0u, index, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
        case pisdf::VertexType::REPEAT:
            if (size == static_cast<size_t>(vertex_->outputEdge(0u)->sourceRateValue())) {
                return { nullptr, size, 0u, index, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
            }
            return { nullptr, size, 0u, index, spider::sched::AllocType::SAME_IN, spider::FifoAttribute::RW_OWN };
        default:
            return { nullptr, size, 0u, index, spider::sched::AllocType::SAME_IN, spider::FifoAttribute::RW_OWN };
    }
}

spider::sched::AllocationRule spider::sched::TaskVertex::allocationRuleForOutputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->outputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto *edge = vertex_->outputEdge(ix);
    const auto size = static_cast<size_t>(edge->sourceRateValue());
    switch (vertex_->subtype()) {
        case pisdf::VertexType::FORK:
            if (ix == 0u) {
                return { nullptr, size, 0u, 0u, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
            } else {
                const auto offset = static_cast<size_t>(vertex_->outputEdge(ix - 1)->sourceRateValue());
                return { nullptr, size, offset, static_cast<u32>(ix - 1), AllocType::SAME_OUT, FifoAttribute::RW_ONLY };
            }
        case pisdf::VertexType::DUPLICATE:
            return { nullptr, size, 0u, 0u, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
        case pisdf::VertexType::EXTERN_IN: {
            const auto ref = vertex_->reference()->convertTo<pisdf::ExternInterface>();
            return { nullptr, size, ref->bufferIndex(), 0u, AllocType::EXT, FifoAttribute::RW_EXT };
        }
        case pisdf::VertexType::REPEAT:
            if (size == static_cast<size_t>(vertex_->inputEdge(0u)->sourceRateValue())) {
                auto inputFifo = fifos_->inputFifo(0u);
                return { nullptr, size, 0u, 0u, AllocType::SAME_IN, inputFifo.attribute_ };
            }
            return { nullptr, size, 0u, UINT32_MAX, AllocType::NEW, FifoAttribute::RW_OWN };
        default: {
            const auto *sink = edge->sink();
            if (sink && sink->subtype() == pisdf::VertexType::EXTERN_OUT) {
                const auto *extInterface = sink->reference()->convertTo<pisdf::ExternInterface>();
                return { nullptr, size, extInterface->bufferIndex(), 0u, AllocType::EXT, FifoAttribute::RW_EXT };
            }
            return { nullptr, size, 0u, UINT32_MAX, AllocType::NEW, FifoAttribute::RW_OWN };
        }
    }
}

spider::sched::Task *spider::sched::TaskVertex::previousTask(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->inputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    return execInfo_.dependencies_.get()[ix];
}

u32 spider::sched::TaskVertex::color() const {
    const auto *reference = vertex_->reference();
    const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 3u) * 50 + 100);
    const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 2u) * 50 + 100);
    const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 4u) * 50 + 100);
    return 0u | (red << 16u) | (green << 8u) | (blue);
}

std::string spider::sched::TaskVertex::name() const {
    return vertex_->name();
}

bool spider::sched::TaskVertex::isSyncOptimizable() const noexcept {
    if (vertex_) {
        return (vertex_->subtype() == pisdf::VertexType::FORK) || (vertex_->subtype() == pisdf::VertexType::DUPLICATE);
    }
    return false;
}

spider::array_handle<spider::sched::Task *> spider::sched::TaskVertex::getDependencies() const {
    return { execInfo_.dependencies_.get(), vertex_->inputEdgeCount() };
}

void spider::sched::TaskVertex::setExecutionDependency(size_t ix, Task *task) {
#ifndef NDEBUG
    if (ix >= vertex_->inputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    if (task) {
        execInfo_.dependencies_.get()[ix] = task;
    }
}

spider::JobMessage spider::sched::TaskVertex::createJobMessage() const {
    JobMessage message{ };
    /* == Set core properties == */
    message.nParamsOut_ = static_cast<u32>(vertex_->reference()->outputParamCount());
    message.kernelIx_ = static_cast<u32>(vertex_->runtimeInformation()->kernelIx());
    message.taskIx_ = static_cast<u32>(vertex_->ix());
    message.ix_ = jobExecIx_;

    /* == Set the synchronization flags == */
    const auto lrtCount{ archi::platform()->LRTCount() };
    const auto *flags = execInfo_.notifications_.get();
    message.synchronizationFlags_ = make_unique<bool>(allocate<bool, StackID::RUNTIME>(lrtCount));
    std::copy(flags, std::next(flags, static_cast<long long>(lrtCount)), message.synchronizationFlags_.get());

    /* == Set the execution task constraints == */
    auto *execConstraints = execInfo_.constraints_.get();
    const auto numberOfConstraints{
            lrtCount - static_cast<size_t>(std::count(execConstraints, execConstraints + lrtCount, SIZE_MAX)) };
    message.execConstraints_ = array<SyncInfo>(numberOfConstraints, StackID::RUNTIME);
    auto jobIterator = std::begin(message.execConstraints_);
    for (size_t i = 0; i < lrtCount; ++i) {
        const auto value = execConstraints[i];
        if (value != SIZE_MAX) {
            jobIterator->lrtToWait_ = i;
            jobIterator->jobToWait_ = static_cast<size_t>(value);
            jobIterator++;
        }
    }

    /* == Set the input parameters (if any) == */
    message.inputParams_ = pisdf::buildVertexRuntimeInputParameters(vertex_);

    /* == Set Fifos == */
    message.fifos_ = fifos_;
    return message;
}

void spider::sched::TaskVertex::setIx(u32 ix) noexcept {
    Task::setIx(ix);
    vertex_->setScheduleTaskIx(ix);
}

std::pair<ufast64, ufast64> spider::sched::TaskVertex::computeCommunicationCost(const PE *mappedPE) const {
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

spider::sched::DependencyInfo spider::sched::TaskVertex::getDependencyInfo(size_t ix) const {
    return { vertex_->inputEdge(ix)->sourcePortIx(),
             static_cast<size_t>(vertex_->inputEdge(ix)->sourceRateValue()) };
}

bool spider::sched::TaskVertex::isMappableOnPE(const spider::PE *pe) const {
    return vertex_->runtimeInformation()->isPEMappable(pe);
}

u64 spider::sched::TaskVertex::timingOnPE(const spider::PE *pe) const {
    return static_cast<u64>(vertex_->runtimeInformation()->timingOnPE(pe, vertex_->inputParamVector()));
}
