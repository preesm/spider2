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

#include <scheduling/schedule/ScheduleTask.h>
#include <graphs/pisdf/Graph.h>
#include <archi/Platform.h>
#include <api/archi-api.h>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::ScheduleTask::ScheduleTask(TaskType type) : outputFifos_{ sbc::vector < RTFifo, StackID::SCHEDULE > { }},
                                                    type_{ type } {
    const auto lrtCount{ archi::platform()->LRTCount() };
    executionConstraints_ = make_unique<int32_t>(allocate<int32_t, StackID::SCHEDULE>(lrtCount));
    std::fill(executionConstraints_.get(), executionConstraints_.get() + lrtCount, -1);
    notificationFlags_ = make_unique<bool>(allocate<bool, StackID::SCHEDULE>(lrtCount));
    std::fill(notificationFlags_.get(), notificationFlags_.get() + lrtCount, false);
    if ((type == TaskType::SYNC_RECEIVE) || (type == TaskType::SYNC_SEND)) {
        dependenciesArray_ = array<ScheduleTask *>(1, nullptr, StackID::SCHEDULE);
    }
}

spider::ScheduleTask::ScheduleTask(pisdf::Vertex *vertex) : ScheduleTask(TaskType::VERTEX) {
    if (!vertex) {
        throwSpiderException("creating TaskType::VERTEX task with nullptr vertex.");
    }
    internal_ = vertex;
    setNumberOfDependencies(vertex->inputEdgeCount());
    outputFifos_.reserve(vertex->outputEdgeCount());
}

spider::ScheduleTask::~ScheduleTask() {
    if (type_ != TaskType::VERTEX) {
        auto *info = reinterpret_cast<ComTaskInformation *>(internal_);
        destroy(info);
    }
}

void spider::ScheduleTask::enableBroadcast() {
    const auto lrtCount = archi::platform()->LRTCount();
    std::fill(notificationFlags_.get(), notificationFlags_.get() + lrtCount, true);
}

std::string spider::ScheduleTask::name() const {
    if (type_ == TaskType::VERTEX) {
        return vertex()->name();
    }
    return (type_ == TaskType::SYNC_SEND) ? "send-task" : "receive-task";
}

u32 spider::ScheduleTask::color() const {
    if (type_ == TaskType::VERTEX) {
        const auto *reference = vertex()->reference();
        const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 3u) * 50 + 100);
        const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 2u) * 50 + 100);
        const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 4u) * 50 + 100);
        return 0u | (red << 16u) | (green << 8u) | (blue);
    }
    return static_cast<u32>(kernelIx());
}

void spider::ScheduleTask::addOutputFifo(RTFifo fifo) {
    outputFifos_.emplace_back(fifo);
}

spider::pisdf::Vertex *spider::ScheduleTask::vertex() const {
    return type_ == TaskType::VERTEX ? reinterpret_cast<pisdf::Vertex *>(internal_) : nullptr;
}

size_t spider::ScheduleTask::kernelIx() const {
    return type_ == TaskType::VERTEX ? vertex()->runtimeInformation()->kernelIx() :
           reinterpret_cast<ComTaskInformation *>(internal_)->kernelIx_;
}

void spider::ScheduleTask::setNumberOfDependencies(size_t count) {
    if (dependenciesArray_.empty()) {
        dependenciesArray_ = array<ScheduleTask *>(count, nullptr, StackID::SCHEDULE);
    }
}

void spider::ScheduleTask::setDependency(ScheduleTask *task, size_t pos) {
    if (!task || dependenciesArray_.empty()) {
        return;
    }
    dependenciesArray_.at(pos) = task;
}

void spider::ScheduleTask::setInternal(void *information) {
    if (information) {
        internal_ = information;
    }
}

spider::JobMessage spider::ScheduleTask::createJobMessage() const {
    JobMessage message{ };
    /* == Set core properties == */
    message.outputParamCount_ =
            type_ == TaskType::VERTEX ? static_cast<i32>(vertex()->reference()->outputParamCount()) : 0;
    message.kernelIx_ = kernelIx();
    message.vertexIx_ = type_ == TaskType::VERTEX ? vertex()->ix() : SIZE_MAX;
    message.ix_ = static_cast<size_t>(ix());

    /* == Set the notification flags == */
    const auto lrtCount{ archi::platform()->LRTCount() };
    message.notificationFlagsArray_ = make_unique<bool>(allocate<bool, StackID::SCHEDULE>(lrtCount));
    auto flags = notificationFlags_.get();
    for (auto &value : make_handle(message.notificationFlagsArray_.get(), lrtCount)) {
        value = (*(flags++));
    }

    /* == Set the execution task constraints == */
    const auto numberOfConstraints{ lrtCount - static_cast<size_t>(std::count(executionConstraints_.get(),
                                                                              executionConstraints_.get() + lrtCount,
                                                                              -1)) };
    message.jobs2Wait_ = array<JobConstraint>(numberOfConstraints, StackID::RUNTIME);
    auto jobIterator = message.jobs2Wait_.begin();
    size_t lrtIt = 0;
    for (auto value : make_handle(executionConstraints_.get(), lrtCount)) {
        if (value >= 0) {
            jobIterator->lrtToWait_ = lrtIt;
            jobIterator->jobToWait_ = static_cast<size_t>(value);
            jobIterator++;
        }
        lrtIt++;
    }

    /* == Set the input parameters (if any) == */
    setJobMessageInputParameters(message);

    /* == Creates the fifos == */
    setJobMessageInputFifos(message);
    setJobMessageOutputFifos(message);

    return message;
}

/* === Private method(s) === */

void spider::ScheduleTask::setJobMessageInputParameters(JobMessage &message) const {
    if (type_ != TaskType::VERTEX) {
        // TODO: handle SEND RECEIVE
        // TODO: need to know both clusters and size...
        auto *information = reinterpret_cast<ComTaskInformation *>(internal_);
        message.inputParams_ = array<i64>(3, StackID::RUNTIME);
        auto *otherPe = archi::platform()->processingElement(dependenciesArray_[0]->mappedLrt());
        auto *thisPe = archi::platform()->processingElement(mappedLrt());
        message.inputParams_[0] = static_cast<i64>(type_ == TaskType::SYNC_SEND ? thisPe->cluster()->ix()
                                                                                : otherPe->cluster()->ix());
        message.inputParams_[1] = static_cast<i64>(type_ == TaskType::SYNC_SEND ? otherPe->cluster()->ix()
                                                                                : thisPe->cluster()->ix());
        message.inputParams_[2] = static_cast<i64>(information->size_);
        message.inputParams_[3] = static_cast<i64>(information->packetIx_);
        return;
    }
    auto *vertex = reinterpret_cast<pisdf::Vertex *>(internal_);
    auto inputEdgeCount{ vertex->inputEdgeCount() };
    auto outputEdgeCount{ vertex->outputEdgeCount() };
    switch (vertex->subtype()) {
        case pisdf::VertexType::CONFIG:
        case pisdf::VertexType::NORMAL: {
            message.inputParams_ = array<i64>(vertex->refinementParamVector().size(),
                                              StackID::RUNTIME);
            auto paramIterator = message.inputParams_.begin();
            for (auto &param : vertex->refinementParamVector()) {
                (*(paramIterator++)) = param->value();
            }
        }
            break;
        case pisdf::VertexType::FORK:
            message.inputParams_ = array<i64>(outputEdgeCount + 2, StackID::RUNTIME);
            message.inputParams_[0] = vertex->inputEdge(0)->sinkRateValue();
            message.inputParams_[1] = static_cast<i64>(outputEdgeCount);
            for (size_t i = 0; i < outputEdgeCount; ++i) {
                message.inputParams_[i + 2] = vertex->outputEdge(i)->sourceRateValue();
            }
            break;
        case pisdf::VertexType::JOIN:
            message.inputParams_ = array<i64>(inputEdgeCount + 2, StackID::RUNTIME);
            message.inputParams_[0] = vertex->outputEdge(0)->sourceRateValue();
            message.inputParams_[1] = static_cast<i64>(inputEdgeCount);
            for (size_t i = 0; i < inputEdgeCount; ++i) {
                message.inputParams_[i + 2] = vertex->inputEdge(i)->sinkRateValue();
            }
            break;
        case pisdf::VertexType::REPEAT:
            message.inputParams_ = array<i64>(2, StackID::RUNTIME);
            message.inputParams_[0] = vertex->inputEdge(0)->sinkRateValue();
            message.inputParams_[1] = vertex->outputEdge(0)->sourceRateValue();
            break;
        case pisdf::VertexType::DUPLICATE:
            message.inputParams_ = array<i64>(2, StackID::RUNTIME);
            message.inputParams_[0] = static_cast<i64>(outputEdgeCount);
            message.inputParams_[1] = vertex->inputEdge(0)->sinkRateValue();
            break;
        case pisdf::VertexType::TAIL: {
            size_t inputCount = 1;
            auto rate = vertex->outputEdge(0)->sourceRateValue();
            for (auto it = vertex->inputEdgeVector().rbegin(); it != vertex->inputEdgeVector().rend(); ++it) {
                const auto &inRate = (*it)->sinkRateValue();
                if (inRate >= rate) {
                    break;
                }
                rate -= inRate;
                inputCount++;
            }
            message.inputParams_ = array<i64>(4 + inputCount, StackID::RUNTIME);
            /* = Number of input = */
            message.inputParams_[0] = static_cast<i64>(inputEdgeCount);
            /* = First input to be considered = */
            message.inputParams_[1] = static_cast<i64>(inputEdgeCount - inputCount);
            /* = Offset in the first buffer if any = */
            message.inputParams_[2] = vertex->inputEdge(inputEdgeCount - inputCount)->sinkRateValue() - rate;
            /* = Effective size to copy of the first input = */
            message.inputParams_[3] = rate;
            size_t i = 4;
            for (auto it = vertex->inputEdgeVector().rbegin();
                 it != vertex->inputEdgeVector().rbegin() + static_cast<long>(inputCount) - 1; ++it) {
                message.inputParams_[i++] = (*it)->sinkRateValue();
            }
        }
            break;
        case pisdf::VertexType::HEAD: {
            size_t inputCount = 1;
            auto rate = vertex->outputEdge(0)->sourceRateValue();
            for (auto &edge : vertex->inputEdgeVector()) {
                const auto &inRate = edge->sinkRateValue();
                if (inRate >= rate) {
                    break;
                }
                rate -= inRate;
                inputCount++;
            }
            message.inputParams_ = array<i64>(1 + inputCount, StackID::RUNTIME);
            message.inputParams_[0] = static_cast<i64>(inputCount);
            rate = vertex->outputEdge(0)->sourceRateValue();
            for (size_t i = 0; i < inputCount; ++i) {
                const auto &inRate = vertex->inputEdge(i)->sinkRateValue();
                message.inputParams_[i + 1] = std::min(inRate, rate);
                rate -= inRate;
            }
        }
            break;
        case pisdf::VertexType::INIT:
            message.inputParams_ = array<i64>(2, StackID::RUNTIME);
            message.inputParams_[0] = static_cast<i64>(-1);
            if (!vertex->refinementParamVector().empty()) {
                message.inputParams_[0] = vertex->refinementParamVector()[0]->value();
            }
            message.inputParams_[1] = vertex->outputEdge(0)->sourceRateValue();
            break;
        case pisdf::VertexType::END:
            message.inputParams_ = array<i64>(2, StackID::RUNTIME);
            message.inputParams_[0] = static_cast<i64>(-1);
            if (!vertex->refinementParamVector().empty()) {
                message.inputParams_[0] = vertex->refinementParamVector()[0]->value();
            }
            message.inputParams_[1] = vertex->inputEdge(0)->sinkRateValue();
            break;
        default:
            break;
    }
}

void spider::ScheduleTask::setJobMessageInputFifos(JobMessage &message) const {
    if (type_ != TaskType::VERTEX) {
        auto *information = reinterpret_cast<ComTaskInformation *>(internal_);
        message.inputFifoArray_ = array<RTFifo>(1, StackID::RUNTIME);
        auto &task = dependenciesArray_[0];
        if (type_ == TaskType::SYNC_RECEIVE) {
            message.inputFifoArray_[0] = task->outputFifos_[0];
        } else {
            message.inputFifoArray_[0] = task->outputFifos_[static_cast<size_t>(information->inputPortIx_)];
        }
        return;
    }
    auto *vertex = reinterpret_cast<pisdf::Vertex *>(internal_);
    const auto inputEdgeCount{ vertex->inputEdgeCount() };
    message.inputFifoArray_ = array<RTFifo>(inputEdgeCount, StackID::RUNTIME);
    for (size_t inputIx = 0; inputIx < inputEdgeCount; ++inputIx) {
        auto &task = dependenciesArray_[inputIx];
        if (task->type() == TaskType::VERTEX) {
            const auto edge = vertex->inputEdge(inputIx);
            message.inputFifoArray_[inputIx] = task->outputFifos_[edge->sourcePortIx()];
        } else if (task->type() == TaskType::SYNC_RECEIVE) {
            message.inputFifoArray_[inputIx] = task->outputFifos_[0];
        }
    }
}

void spider::ScheduleTask::setJobMessageOutputFifos(JobMessage &message) const {
    if (type_ != TaskType::VERTEX) {
        if (type_ == TaskType::SYNC_RECEIVE) {
            message.outputFifoArray_ = array<RTFifo>(1, StackID::RUNTIME);
            message.outputFifoArray_[0] = outputFifos_[0];
        }
        return;
    }
    auto *vertex = reinterpret_cast<pisdf::Vertex *>(internal_);
    const auto outputEdgeCount{ vertex->outputEdgeCount() };
    message.outputFifoArray_ = array<RTFifo>(outputEdgeCount, StackID::RUNTIME);
    for (size_t outputIx = 0; outputIx < outputEdgeCount; ++outputIx) {
        message.outputFifoArray_[outputIx] = outputFifos_[outputIx];
    }
}


