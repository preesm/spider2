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
#include <scheduling/task/TaskVertex.h>
#include <scheduling/schedule/Schedule.h>
#include <api/runtime-api.h>
#include <runtime/platform/RTPlatform.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::TaskVertex::TaskVertex(pisdf::Vertex *vertex) : Task(), vertex_{ vertex } {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    fifos_ = spider::make_shared<TaskFifos, StackID::SCHEDULE>(vertex->inputEdgeCount(), vertex->outputEdgeCount());
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
    const auto lrtCount = archi::platform()->LRTCount();
    auto oldDependencies = array<size_t>(lrtCount, SIZE_MAX, StackID::SCHEDULE);
    auto *execDependencies = execInfo_.dependencies_.get();
    auto *execConstraints = execInfo_.constraints_.get();
    std::fill(execConstraints, execConstraints + lrtCount, SIZE_MAX);
    for (size_t i = 0; i < vertex_->inputEdgeCount(); ++i) {
        auto *dependency = execInfo_.dependencies_.get()[i];
        if (dependency) {
            const auto *depLRT = dependency->mappedLRT();
            const auto currentJobConstraint = execConstraints[depLRT->virtualIx()];
            if (currentJobConstraint == SIZE_MAX) {
                oldDependencies[depLRT->virtualIx()] = i;
                execConstraints[depLRT->virtualIx()] = dependency->jobExecIx();
                dependency->setNotificationFlag(mappedLRT()->virtualIx(), true);
            } else if (dependency->jobExecIx() > currentJobConstraint) {
                auto *oldDep = execDependencies[oldDependencies[depLRT->virtualIx()]];
                oldDep->setNotificationFlag(mappedLRT()->virtualIx(), false);
                dependency->setNotificationFlag(mappedLRT()->virtualIx(), true);
                oldDependencies[depLRT->virtualIx()] = i;
                execConstraints[depLRT->virtualIx()] = dependency->jobExecIx();
            }
        }
    }
}

spider::sched::AllocationRule spider::sched::TaskVertex::allocationRuleForInputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->inputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto size = static_cast<size_t>(vertex_->inputEdge(ix)->sinkRateValue());
    switch (vertex_->subtype()) {
        case pisdf::VertexType::FORK:
            return { 0U, size, AllocType::SAME, FifoAttribute::RW_ONLY };
        case pisdf::VertexType::DUPLICATE:
            return { 0U, size, AllocType::SAME, FifoAttribute::RW_ONLY };
        case pisdf::VertexType::EXTERN_OUT: {
            const auto ref = vertex_->reference()->convertTo<pisdf::ExternInterface>();
            return { ref->bufferIndex(), size, AllocType::NEW, FifoAttribute::RW_EXT };
        }
        case pisdf::VertexType::REPEAT:
            if (size == static_cast<size_t>(vertex_->outputEdge(ix)->sourceRateValue())) {
                return { 0U, size, AllocType::SAME, FifoAttribute::RW_ONLY };
            }
            return { 0U, size, AllocType::SAME, FifoAttribute::RW_OWN };
        default:
            return { 0U, size, AllocType::SAME, FifoAttribute::RW_OWN };
    }
}

spider::sched::AllocationRule spider::sched::TaskVertex::allocationRuleForOutputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->outputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto size = static_cast<size_t>(vertex_->outputEdge(ix)->sourceRateValue());
    switch (vertex_->subtype()) {
        case pisdf::VertexType::FORK: {
            size_t offset{ 0U };
            for (size_t i = 0U; i < ix; ++i) {
                offset += static_cast<size_t>(vertex_->outputEdge(i)->sourceRateValue());
            }
            return { offset, size, AllocType::SAME, FifoAttribute::RW_ONLY };
        }
        case pisdf::VertexType::DUPLICATE:
            return { 0U, size, AllocType::SAME, FifoAttribute::RW_ONLY };
        case pisdf::VertexType::EXTERN_IN: {
            const auto ref = vertex_->reference()->convertTo<pisdf::ExternInterface>();
            return { ref->bufferIndex(), size, AllocType::NEW, FifoAttribute::RW_EXT };
        }
        case pisdf::VertexType::REPEAT:
            if (size == static_cast<size_t>(vertex_->inputEdge(ix)->sinkRateValue())) {
                return { 0U, size, AllocType::SAME, FifoAttribute::RW_ONLY };
            }
            return { 0U, size, AllocType::NEW, FifoAttribute::RW_OWN };
        default:
            return { 0U, size, AllocType::NEW, FifoAttribute::RW_OWN };
    }
}

spider::sched::Task *spider::sched::TaskVertex::previousTask(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->inputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    return nullptr;
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
    message.kernel_ = rt::platform()->getKernel(vertex_->runtimeInformation()->kernelIx());
    message.taskIx_ = static_cast<u32>(vertex_->ix());
    message.ix_ = jobExecIx_;

    /* == Set the notification flags == */
    const auto lrtCount{ archi::platform()->LRTCount() };
//    message.notificationFlagsArray_ = make_unique<bool>(allocate<bool, StackID::SCHEDULE>(lrtCount));
//    auto flags = notificationFlags_.get();
//    for (auto &value : make_handle(message.notificationFlagsArray_.get(), lrtCount)) {
//        value = (*(flags++));
//    }

    /* == Set the execution task constraints == */
    auto *execConstraints = execInfo_.constraints_.get();
    const auto numberOfConstraints{
            lrtCount - static_cast<size_t>(std::count(execConstraints, execConstraints + lrtCount, SIZE_MAX)) };
    message.execConstraints_ = array<JobConstraint>(numberOfConstraints, StackID::RUNTIME);
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
//    setJobMessageInputParameters(message);

    /* == Set Fifos == */
    message.fifos_ = fifos_;
    return message;
}

/* === Private method(s) implementation === */
