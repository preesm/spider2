/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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
#include <scheduling/allocator/TaskMemory.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <archi/Platform.h>
#include <api/archi-api.h>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::ScheduleTask::ScheduleTask(TaskType type) : type_{ type } {
    const auto lrtCount{ archi::platform()->LRTCount() };
    executionConstraints_ = make_unique<int32_t>(allocate<int32_t, StackID::SCHEDULE>(lrtCount));
    notificationFlags_ = make_unique<bool>(allocate<bool, StackID::SCHEDULE>(lrtCount));

    std::fill(executionConstraints_.get(), executionConstraints_.get() + lrtCount, -1);
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

spider::pisdf::Vertex *spider::ScheduleTask::vertex() const {
    return type_ == TaskType::VERTEX ? reinterpret_cast<pisdf::Vertex *>(internal_) : nullptr;
}

spider::ComTaskInformation *spider::ScheduleTask::comTaskInfo() const {
    return type_ != TaskType::VERTEX ? reinterpret_cast<ComTaskInformation *>(internal_) : nullptr;
}

size_t spider::ScheduleTask::kernelIx() const {
    return type_ == TaskType::VERTEX ? vertex()->runtimeInformation()->kernelIx() :
           reinterpret_cast<ComTaskInformation *>(internal_)->kernelIx_;
}

spider::RTFifo spider::ScheduleTask::getInputFifo(size_t ix) const {
    if (taskMemory_) {
        return taskMemory_->inputFifo(ix);
    }
    return spider::RTFifo{ };
}

spider::RTFifo spider::ScheduleTask::getOutputFifo(size_t ix) const {
    if (taskMemory_) {
        return taskMemory_->outputFifo(ix);
    }
    return spider::RTFifo{ };
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
    if (information && !internal_) {
        internal_ = information;
    }
}

spider::JobMessage spider::ScheduleTask::createJobMessage() const {
    JobMessage message{ };
    /* == Set core properties == */
    const auto *vertex = this->vertex();
    message.outputParamCount_ = vertex ? static_cast<i32>(vertex->reference()->outputParamCount()) : 0;
    message.kernelIx_ = kernelIx();
    message.vertexIx_ = vertex ? vertex->ix() : SIZE_MAX;
    message.ix_ = static_cast<size_t>(execIx());

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
    message.execConstraints_ = array<JobConstraint>(numberOfConstraints, StackID::RUNTIME);
    size_t lrtIt = 0;
    auto jobIterator = message.execConstraints_.begin();
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

    /* == Copy input Fifos == */
    const auto inputFifos = taskMemory_->inputFifos();
    message.inputFifoArray_ = array<RTFifo>(taskMemory_->inputFifoCount(), StackID::RUNTIME);
    std::copy(std::begin(inputFifos), std::end(inputFifos), std::begin(message.inputFifoArray_));

    /* == Copy output Fifos == */
    const auto outputFifos = taskMemory_->outputFifos();
    message.outputFifoArray_ = array<RTFifo>(taskMemory_->outputFifoCount(), StackID::RUNTIME);
    std::copy(std::begin(outputFifos), std::end(outputFifos), std::begin(message.outputFifoArray_));
    return message;
}

void spider::ScheduleTask::setTaskMemory(spider::unique_ptr<TaskMemory> taskMemory) {
    taskMemory_ = std::move(taskMemory);
}

/* === Private method(s) === */

void spider::ScheduleTask::setJobMessageInputParameters(JobMessage &message) const {
    switch (type_) {
        case TaskType::VERTEX:
            message.inputParams_ = pisdf::buildVertexRuntimeInputParameters(vertex());
            break;
        case TaskType::SYNC_SEND:
        case TaskType::SYNC_RECEIVE: {
            const auto fstPeIx = type_ == TaskType::SYNC_SEND ? mappedLrt() : dependenciesArray_[0]->mappedLrt();
            const auto sndPeIx = type_ == TaskType::SYNC_SEND ? dependenciesArray_[0]->mappedLrt() : mappedLrt();
            const auto *fstPe = archi::platform()->processingElement(fstPeIx);
            const auto *sndPe = archi::platform()->processingElement(sndPeIx);
            message.inputParams_ = array<i64>(4, StackID::RUNTIME);
            message.inputParams_[0] = static_cast<i64>(fstPe->cluster()->ix());
            message.inputParams_[1] = static_cast<i64>(sndPe->cluster()->ix());
            message.inputParams_[2] = static_cast<i64>(comTaskInfo()->size_);
            message.inputParams_[3] = static_cast<i64>(comTaskInfo()->packetIx_);
        }
            break;
    }
}
