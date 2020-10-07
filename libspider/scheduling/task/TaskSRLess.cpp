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

#include <scheduling/task/TaskSRLess.h>
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

spider::sched::TaskSRLess::TaskSRLess(srless::GraphFiring *handler,
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

spider::sched::Task *spider::sched::TaskSRLess::previousTask(size_t ix) const {
#ifndef NDEBUG
    if (ix >= dependenciesCount_) {
        throwSpiderException("index out of bound.");
    }
#endif
    return execInfo_.dependencies_.get()[ix];
}

void spider::sched::TaskSRLess::updateTaskExecutionDependencies(const Schedule *schedule) {
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

void spider::sched::TaskSRLess::updateExecutionConstraints() {
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

void spider::sched::TaskSRLess::setExecutionDependency(size_t ix, spider::sched::Task *task) {
#ifndef NDEBUG
    if (ix >= dependenciesCount_) {
        throwSpiderException("index out of bound.");
    }
#endif
    if (task) {
        execInfo_.dependencies_.get()[ix] = task;
    }
}

spider::sched::AllocationRule spider::sched::TaskSRLess::allocationRuleForInputFifo(size_t ix) const {
#ifndef NDEBUG
    if ((vertex_->subtype() != pisdf::VertexType::INPUT) && (ix >= vertex_->inputEdgeCount())) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto *edge = vertex_->inputEdge(ix);
    return allocateInputFifo(edge);
}

spider::sched::AllocationRule spider::sched::TaskSRLess::allocationRuleForOutputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->outputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto *edge = vertex_->outputEdge(ix);
    auto rule = AllocationRule{ };
    rule.size_ = static_cast<size_t>(edge->sourceRateExpression().evaluate(handler_->getParams()));
    rule.offset_ = 0u;
    rule.fifoIx_ = 0u;
    rule.count_ = 0u;
    if (rule.size_) {
        const auto edgeIx = static_cast<u32>(edge->sourcePortIx());
        const auto dependencies = pisdf::computeConsDependency(vertex_, firing_, edgeIx, handler_);
        if (!dependencies.count()) {
            rule.attribute_ = FifoAttribute::W_SINK;
        } else {
            for (const auto &dep : dependencies) {
                rule.count_ += (dep.rate_ > 0) * (dep.firingEnd_ - dep.firingStart_ + 1u);
            }
        }
    }
    switch (vertex_->subtype()) {
//        case pisdf::VertexType::FORK:
//            if (ix == 0u) {
//                rule.type_ = AllocType::SAME_IN;
//            } else {
//                rule.offset_ = static_cast<size_t>(vertex_->outputEdge(ix - 1)->sourceRateExpression().evaluate(
//                        handler_->getParams()));
//                rule.fifoIx_ = static_cast<u32>(ix - 1);
//                rule.type_ = AllocType::SAME_OUT;
//            }
//            rule.attribute_ = FifoAttribute::RW_ONLY;
//            break;
        case pisdf::VertexType::DUPLICATE:
            rule.type_ = AllocType::SAME_IN;
            rule.attribute_ = FifoAttribute::RW_ONLY;
            break;
        case pisdf::VertexType::EXTERN_IN:
            rule.offset_ = vertex_->reference()->convertTo<pisdf::ExternInterface>()->bufferIndex();
            rule.type_ = AllocType::EXT;
            rule.attribute_ = FifoAttribute::RW_EXT;
            break;
        case pisdf::VertexType::REPEAT:
            if (rule.size_ ==
                static_cast<size_t>(vertex_->inputEdge(0u)->sourceRateExpression().evaluate(handler_->getParams()))) {
                auto inputFifo = fifos_->inputFifo(0u);
                rule.type_ = AllocType::SAME_IN;
                rule.attribute_ = inputFifo.attribute_;
            }
            break;
        default: {
            const auto *sink = edge->sink();
            if (sink && sink->subtype() == pisdf::VertexType::EXTERN_OUT) {
                const auto *extInterface = sink->reference()->convertTo<pisdf::ExternInterface>();
                rule.offset_ = extInterface->bufferIndex();
                rule.type_ = AllocType::EXT;
                rule.attribute_ = FifoAttribute::RW_EXT;
            }
            break;
        }
    }
    return rule;
}

spider::JobMessage spider::sched::TaskSRLess::createJobMessage() const {
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

u32 spider::sched::TaskSRLess::color() const {
    const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 3u) * 50 + 100);
    const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 2u) * 50 + 100);
    const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 4u) * 50 + 100);
    return 0u | (red << 16u) | (green << 8u) | (blue);
}

std::string spider::sched::TaskSRLess::name() const {
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

void spider::sched::TaskSRLess::setIx(u32 ix) noexcept {
    Task::setIx(ix);
    handler_->registerTaskIx(vertex_, firing_, ix);
}

spider::array_handle<spider::sched::Task *> spider::sched::TaskSRLess::getDependencies() const {
    return { execInfo_.dependencies_.get(), dependenciesCount_ };
}

std::pair<ufast64, ufast64> spider::sched::TaskSRLess::computeCommunicationCost(const spider::PE */*mappedPE*/) const {
    return { };
}

bool spider::sched::TaskSRLess::isMappableOnPE(const spider::PE *pe) const {
    return vertex_->runtimeInformation()->isPEMappable(pe);
}

u64 spider::sched::TaskSRLess::timingOnPE(const spider::PE *pe) const {
    return static_cast<u64>(vertex_->runtimeInformation()->timingOnPE(pe, handler_->getParams()));
}

spider::sched::DependencyInfo spider::sched::TaskSRLess::getDependencyInfo(size_t /*size*/) const {
    return { };
}

/* === Private method(s) implementation === */

size_t spider::sched::TaskSRLess::updateTaskExecutionDependency(const Schedule *schedule,
                                                                const pisdf::DependencyInfo &dependency,
                                                                size_t index) {
    if (!dependency.vertex_) {
        return index;
    }
    for (u32 k = dependency.firingStart_; k <= dependency.firingEnd_; ++k) {
        const auto taskIx = dependency.handler_->getTaskIx(dependency.vertex_, k);
        const auto &sourceTask = schedule->tasks()[taskIx];
        execInfo_.dependencies_.get()[index + k - dependency.firingStart_] = sourceTask.get();
    }
    return index + (dependency.firingEnd_ - dependency.firingStart_) + 1u;
}

spider::sched::AllocationRule
spider::sched::TaskSRLess::allocateInputFifo(const pisdf::Edge *edge) const {
    u32 count = 0;
    const auto deps = spider::pisdf::computeExecDependency(vertex_, firing_, edge->sinkPortIx(), handler_);
    for (const auto &dep : deps) {
        count += dep.firingEnd_ - dep.firingStart_ + 1u;
    }
    auto rule = AllocationRule{ };
    if (count > 1u) {
        rule.others_ = spider::allocate<AllocationRule, StackID::SCHEDULE>(count);
        rule.size_ = static_cast<size_t>(edge->sinkRateExpression().evaluate(handler_->getParams()));
        rule.offset_ = 0u;
        rule.fifoIx_ = UINT32_MAX;
        rule.count_ = count;
        rule.type_ = AllocType::MERGE;
        rule.attribute_ = FifoAttribute::R_MERGE;
        size_t depOffset = 0u;
        for (const auto &dep : deps) {
            /* == first dependency == */
            auto rate = dep.firingStart_ == dep.firingEnd_ ? (dep.memoryEnd_ - dep.memoryStart_ + 1u) :
                        static_cast<size_t>(dep.rate_ - dep.memoryStart_);
            rule.others_[depOffset] = AllocationRule(rate, dep.memoryStart_, dep.edgeIx_, 0u, AllocType::SAME_IN,
                                                     FifoAttribute::RW_OWN);
            /* == middle dependencies if > 2 == */
            for (auto k = dep.firingStart_ + 1; k < dep.firingEnd_; ++k) {
                const auto ix = k + depOffset - dep.firingStart_;
                rate = static_cast<size_t>(dep.rate_);
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
