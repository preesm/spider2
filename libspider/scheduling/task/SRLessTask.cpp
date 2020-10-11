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

#include <scheduling/task/SRLessTask.h>
#include <scheduling/schedule/Schedule.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs-tools/transformation/srless/GraphHandler.h>
#include <graphs-tools/transformation/srless/GraphFiring.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Graph.h>
#include <runtime/special-kernels/specialKernels.h>
#include <graphs-tools/numerical/dependencies.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::SRLessTask::SRLessTask(srless::GraphFiring *handler,
                                      const pisdf::Vertex *vertex,
                                      u32 firing,
                                      u32 depCount,
                                      u32 mergedFifoCount) : Task(),
                                                             handler_{ handler },
                                                             vertex_{ vertex },
                                                             firing_{ firing },
                                                             dependenciesCount_{ depCount } {
    const auto inputFifoCount = depCount + mergedFifoCount;
    fifos_ = spider::make_shared<AllocatedFifos, StackID::SCHEDULE>(inputFifoCount, vertex->outputEdgeCount());
    execInfo_.dependencies_ = spider::make_unique(allocate<Task *, StackID::SCHEDULE>(depCount));
    auto *beginIt = execInfo_.dependencies_.get();
    std::fill(beginIt, std::next(beginIt, static_cast<long>(depCount)), nullptr);
}

spider::sched::Task *spider::sched::SRLessTask::previousTask(size_t ix) const {
#ifndef NDEBUG
    if (ix >= dependenciesCount_) {
        throwSpiderException("index out of bound.");
    }
#endif
    return execInfo_.dependencies_.get()[ix];
}

void spider::sched::SRLessTask::updateTaskExecutionDependencies(const Schedule *schedule) {
    size_t i = 0u;
    for (const auto *edge : vertex_->inputEdgeVector()) {
        const auto deps = spider::pisdf::computeExecDependency(vertex_, firing_, edge->sinkPortIx(), handler_);
        for (const auto &dep : deps) {
            if (dep.vertex_) {
                for (u32 k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                    const auto taskIx = dep.handler_->getTaskIx(dep.vertex_, k);
                    const auto &sourceTask = schedule->tasks()[taskIx];
                    execInfo_.dependencies_.get()[i + k - dep.firingStart_] = sourceTask.get();
                }
            }
            i += (dep.firingEnd_ - dep.firingStart_) + 1u;
        }
    }
}

void spider::sched::SRLessTask::updateExecutionConstraints() {
    auto *execDependencies = execInfo_.dependencies_.get();
    auto *execConstraints = execInfo_.constraints_.get();
    const auto lrtCount = archi::platform()->LRTCount();
    std::fill(execConstraints, execConstraints + lrtCount, SIZE_MAX);
    auto shouldNotifyArray = array<size_t>(lrtCount, SIZE_MAX, StackID::SCHEDULE);
    for (u32 i = 0; i < dependenciesCount_; ++i) {
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

void spider::sched::SRLessTask::setExecutionDependency(size_t ix, spider::sched::Task *task) {
#ifndef NDEBUG
    if (ix >= dependenciesCount_) {
        throwSpiderException("index out of bound.");
    }
#endif
    if (task) {
        execInfo_.dependencies_.get()[ix] = task;
    }
}

spider::sched::AllocationRule spider::sched::SRLessTask::allocationRuleForInputFifo(size_t edgeIx) const {
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

spider::sched::AllocationRule spider::sched::SRLessTask::allocationRuleForOutputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->outputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto *edge = vertex_->outputEdge(ix);
    const auto rate = static_cast<u32>(handler_->getSourceRate(edge));
    const auto count = countConsummerCount(edge);
    if (rate && !count) {
        return { rate, 0u, 0u, 1u, AllocType::NEW, FifoAttribute::W_SINK };
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

spider::JobMessage spider::sched::SRLessTask::createJobMessage() const {
    JobMessage message{ };
    /* == Set core properties == */
    message.nParamsOut_ = static_cast<u32>(vertex_->reference()->outputParamCount());
    message.kernelIx_ = static_cast<u32>(vertex_->runtimeInformation()->kernelIx());
    message.taskIx_ = ix_;
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
    message.inputParams_ = pisdf::buildVertexRuntimeInputParameters(vertex_, handler_->getParams());

    /* == Set Fifos == */
    message.fifos_ = fifos_;
    return message;
}

u32 spider::sched::SRLessTask::color() const {
    const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 3u) * 50 + 100);
    const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 2u) * 50 + 100);
    const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 4u) * 50 + 100);
    return 0u | (red << 16u) | (green << 8u) | (blue);
}

std::string spider::sched::SRLessTask::name() const {
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

void spider::sched::SRLessTask::setIx(u32 ix) noexcept {
    Task::setIx(ix);
    handler_->registerTaskIx(vertex_, firing_, ix);
}

std::pair<ufast64, ufast64> spider::sched::SRLessTask::computeCommunicationCost(const spider::PE */*mappedPE*/) const {
    return { };
}

bool spider::sched::SRLessTask::isMappableOnPE(const spider::PE *pe) const {
    return vertex_->runtimeInformation()->isPEMappable(pe);
}

u64 spider::sched::SRLessTask::timingOnPE(const spider::PE *pe) const {
    return static_cast<u64>(vertex_->runtimeInformation()->timingOnPE(pe, handler_->getParams()));
}

spider::sched::DependencyInfo spider::sched::SRLessTask::getDependencyInfo(size_t /*size*/) const {
    return { };
}

size_t spider::sched::SRLessTask::dependencyCount() const {
    return dependenciesCount_;
}

/* === Private method(s) implementation === */

u32 spider::sched::SRLessTask::countConsummerCount(const spider::pisdf::Edge *edge) const {
    const auto dependencies = pisdf::computeConsDependency(vertex_, firing_, edge->sourcePortIx(), handler_);
    if (dependencies.count()) {
        u32 count = 0;
        for (const auto &dep : dependencies) {
            count += (dep.rate_ > 0) * (dep.firingEnd_ - dep.firingStart_ + 1u);
        }
        return count ? count : 1;
    }
    return 0;
}
