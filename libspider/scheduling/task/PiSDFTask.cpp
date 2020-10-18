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

#include <scheduling/task/PiSDFTask.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/memory/FifoAllocator.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs-tools/numerical/dependencies.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Graph.h>
#include <runtime/special-kernels/specialKernels.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::PiSDFTask::PiSDFTask(srless::GraphFiring *handler,
                                    const pisdf::Vertex *vertex,
                                    u32 firing,
                                    u32 depCount,
                                    u32 mergedFifoCount) : Task(),
                                                             handler_{ handler },
                                                             vertex_{ vertex },
                                                             firing_{ firing },
                                                             dependenciesCount_{ depCount } {
    const auto inputFifoCount = depCount + mergedFifoCount;
    fifos_ = spider::make_shared<JobFifos, StackID::SCHEDULE>(inputFifoCount, vertex->outputEdgeCount());
    dependencies_ = spider::make_unique(spider::allocate<Task *, StackID::SCHEDULE>(depCount));
    std::fill(dependencies_.get(), dependencies_.get() + depCount, nullptr);
}

void spider::sched::PiSDFTask::allocate(FifoAllocator *allocator) {
    allocator->allocate(this);
}

void spider::sched::PiSDFTask::updateTaskExecutionDependencies(const Schedule *schedule) {
    size_t i = 0u;
    for (const auto *edge : vertex_->inputEdges()) {
        const auto deps = spider::pisdf::computeExecDependency(vertex_, firing_, edge->sinkPortIx(), handler_);
        for (const auto &dep : deps) {
            if (dep.vertex_) {
                for (u32 k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                    const auto taskIx = dep.handler_->getTaskIx(dep.vertex_, k);
                    setExecutionDependency(i + k - dep.firingStart_, schedule->tasks()[taskIx].get());
                }
            }
            i += (dep.firingEnd_ - dep.firingStart_) + 1u;
        }
    }
}

spider::sched::AllocationRule spider::sched::PiSDFTask::allocationRuleForInputFifo(size_t edgeIx) const {
#ifndef NDEBUG
    if ((vertex_->subtype() != pisdf::VertexType::INPUT) && (edgeIx >= vertex_->inputEdgeCount())) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto *edge = vertex_->inputEdge(edgeIx);
    u32 count = 0;
    const auto deps = spider::pisdf::computeExecDependency(vertex_, firing_, edge->sinkPortIx(), handler_);
    for (const auto &dep : deps) {
        count += dep.firingEnd_ - dep.firingStart_ + 1u;
    }
    auto rule = AllocationRule{ };
    if (count > 1u) {
        rule.others_ = spider::allocate<AllocationRule, StackID::SCHEDULE>(count);
        rule.size_ = static_cast<u32>(handler_->getSinkRate(edge));
        rule.offset_ = count;
        rule.fifoIx_ = UINT32_MAX;
        rule.count_ = 1u;
        rule.type_ = AllocType::MERGE;
        rule.attribute_ = FifoAttribute::R_MERGE;
        size_t depOffset = 0u;
        for (const auto &dep : deps) {
            /* == first dependency == */
            auto rate = dep.firingStart_ == dep.firingEnd_ ? (dep.memoryEnd_ - dep.memoryStart_ + 1u) :
                        static_cast<u32>(dep.rate_) - dep.memoryStart_;
            rule.others_[depOffset] = AllocationRule(rate, dep.memoryStart_, dep.edgeIx_, 0u, AllocType::SAME_IN,
                                                     FifoAttribute::RW_OWN);
            /* == middle dependencies if > 2 == */
            for (auto k = dep.firingStart_ + 1; k < dep.firingEnd_; ++k) {
                const auto ix = k + depOffset - dep.firingStart_;
                rate = static_cast<u32>(dep.rate_);
                rule.others_[ix] = AllocationRule(rate, 0u, dep.edgeIx_, 0u, AllocType::SAME_IN, FifoAttribute::RW_OWN);
            }
            /* == last dependency == */
            const auto ix = dep.firingEnd_ - dep.firingStart_ + depOffset;
            if (ix > depOffset) {
                rate = dep.memoryEnd_ + 1u;
                rule.others_[ix] = AllocationRule(rate, 0u, dep.edgeIx_, 0u, AllocType::SAME_IN, FifoAttribute::RW_OWN);
            }
            depOffset += (dep.firingEnd_ - dep.firingStart_) + 1u;
        }
    } else if (deps.count()) {
        const auto &dep = *(deps.begin());
        const auto rate = (dep.rate_ > 0) * (dep.memoryEnd_ - dep.memoryStart_ + 1u);
        rule = AllocationRule(rate, dep.memoryStart_, dep.edgeIx_, 0u, AllocType::SAME_IN, FifoAttribute::RW_OWN);
        if (!rate) {
            rule.attribute_ = FifoAttribute::DUMMY;
        }
    } else {
        rule = AllocationRule(0, 0, 0, 0u, AllocType::SAME_IN, FifoAttribute::DUMMY);
    }
    return rule;
}

spider::sched::AllocationRule spider::sched::PiSDFTask::allocationRuleForOutputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->outputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto *edge = vertex_->outputEdge(ix);
    const auto rate = static_cast<u32>(handler_->getSourceRate(edge));
    auto count = spider::pisdf::computeConsDependencyCount(vertex_, firing_, edge->sourcePortIx(), handler_);
    if (rate && count == UINT32_MAX) {
        return { rate, 0u, 0u, 1u, AllocType::NEW, FifoAttribute::W_SINK };
    } else if (!count) {
        count = rate > 0;
    }
    switch (vertex_->subtype()) {
        case pisdf::VertexType::FORK:
            if (ix == 0u) {
                return { rate, 0u, 0u, count, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
            } else {
                const auto prevIx = static_cast<u32>(ix - 1);
                const auto *previousEdge = vertex_->outputEdge(prevIx);
                const auto offset = static_cast<u32>(handler_->getSourceRate(previousEdge));
                return { rate, offset, prevIx, count, AllocType::SAME_OUT, FifoAttribute::RW_ONLY };
            }
        case pisdf::VertexType::DUPLICATE:
            return { rate, 0u, 0u, count, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
        case pisdf::VertexType::EXTERN_IN: {
            const auto offset = vertex_->convertTo<pisdf::ExternInterface>()->bufferIndex();
            return { rate, static_cast<u32>(offset), 0u, count, AllocType::EXT, FifoAttribute::RW_EXT };
        }
        case pisdf::VertexType::REPEAT:
            if (rate == static_cast<size_t>(handler_->getSourceRate(vertex_->inputEdge(0u)))) {
                return { rate, 0u, 0u, count, AllocType::SAME_IN, fifos_->inputFifo(0u).attribute_ };
            }
            break;
        default:
            if (edge->sink() && edge->sink()->subtype() == pisdf::VertexType::EXTERN_OUT) {
                const auto offset = edge->sink()->convertTo<pisdf::ExternInterface>()->bufferIndex();
                return { rate, static_cast<u32>(offset), 0u, count, AllocType::EXT, FifoAttribute::RW_EXT };
            }
            break;
    }
    return { rate, 0u, 0u, count, AllocType::NEW, FifoAttribute::RW_OWN };
}

spider::JobMessage spider::sched::PiSDFTask::createJobMessage() const {
    auto message = Task::createJobMessage();
    message.nParamsOut_ = static_cast<u32>(vertex_->outputParamCount());
    message.kernelIx_ = static_cast<u32>(vertex_->runtimeInformation()->kernelIx());
    /* == Set the input parameters (if any) == */
    message.inputParams_ = pisdf::buildVertexRuntimeInputParameters(vertex_, handler_->getParams());
    return message;
}

u32 spider::sched::PiSDFTask::color() const {
    const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 3u) * 50 + 100);
    const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 2u) * 50 + 100);
    const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 4u) * 50 + 100);
    return 0u | (red << 16u) | (green << 8u) | (blue);
}

std::string spider::sched::PiSDFTask::name() const {
    std::string name{ };
    const auto *vertex = vertex_;
    const auto *handler = handler_;
    while (handler) {
        const auto *graph = vertex->graph();
        const auto firing = handler->firingValue();
        name = graph->name() + std::string(":").append(std::to_string(firing)).append(":").append(name);
        handler = handler->getParent()->handler();
        vertex = graph;
    }
    return name.append(vertex_->name()).append(":").append(std::to_string(firing_));
}

void spider::sched::PiSDFTask::setIx(u32 ix) noexcept {
    Task::setIx(ix);
    handler_->registerTaskIx(vertex_, firing_, ix);
}

std::pair<ufast64, ufast64> spider::sched::PiSDFTask::computeCommunicationCost(const spider::PE */*mappedPE*/) const {
    return { };
}

bool spider::sched::PiSDFTask::isMappableOnPE(const spider::PE *pe) const {
    return vertex_->runtimeInformation()->isPEMappable(pe);
}

u64 spider::sched::PiSDFTask::timingOnPE(const spider::PE *pe) const {
    return static_cast<u64>(vertex_->runtimeInformation()->timingOnPE(pe, handler_->getParams()));
}

spider::sched::DependencyInfo spider::sched::PiSDFTask::getDependencyInfo(size_t /*size*/) const {
    return { };
}

size_t spider::sched::PiSDFTask::dependencyCount() const {
    return dependenciesCount_;
}