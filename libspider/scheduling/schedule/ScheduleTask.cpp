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
}

spider::ScheduleTask::ScheduleTask(pisdf::Vertex *vertex) : ScheduleTask(TaskType::VERTEX) {
    if (!vertex) {
        throwSpiderException("creating TaskType::VERTEX task with nullptr vertex.");
    }
    vertex_ = vertex;
    setNumberOfDependencies(vertex->inputEdgeCount());
    outputFifos_.reserve(vertex->outputEdgeCount());
}

void spider::ScheduleTask::enableBroadcast() {
    const auto lrtCount = archi::platform()->LRTCount();
    std::fill(notificationFlags_.get(), notificationFlags_.get() + lrtCount, true);
}

std::string spider::ScheduleTask::name() const {
    if (type_ == TaskType::VERTEX) {
        return vertex_->name();
    } else if (type_ == TaskType::SYNC_SEND) {
        return "send-task";
    }
    return "receive-task";
}

u32 spider::ScheduleTask::color() const {
    if (type_ == TaskType::VERTEX) {
        const auto *reference = vertex_->reference();
        const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 3u) * 50 + 100);
        const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 2u) * 50 + 100);
        const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 4u) * 50 + 100);
        return 0u | (red << 16u) | (green << 8u) | (blue);
    }
    return static_cast<u32>(kernelIx_);
}

void spider::ScheduleTask::addOutputFifo(spider::RTFifo fifo) {
    outputFifos_.emplace_back(fifo);
}

spider::pisdf::Vertex *spider::ScheduleTask::vertex() const {
    return vertex_;
}

size_t spider::ScheduleTask::kernelIx() const {
    return type_ == TaskType::VERTEX ? vertex_->runtimeInformation()->kernelIx() : kernelIx_;
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

void spider::ScheduleTask::setVertex(pisdf::Vertex *vertex) {
    if (type_ == TaskType::VERTEX && vertex) {
        vertex_ = vertex;
    }
}

void spider::ScheduleTask::setKernelIx(size_t kernelIx) {
    if (type_ != TaskType::VERTEX) {
        kernelIx_ = kernelIx;
    }
}

spider::JobMessage spider::ScheduleTask::createJobMessage() const {
    JobMessage message{ };
    /* == Set core properties == */
    message.outputParamCount_ = vertex_ ? static_cast<i32>(vertex_->reference()->outputParamCount()) : 0;
    message.kernelIx_ = kernelIx();
    message.vertexIx_ = vertex_ ? vertex_->ix() : SIZE_MAX;
    message.ix_ = static_cast<size_t>(ix());

    /* == Set the notification flags == */
    const auto lrtCount{ archi::platform()->LRTCount() };
    message.notificationFlagsArray_ = make_unique<bool>(allocate<bool, StackID::SCHEDULE>(lrtCount));
    auto flags = notificationFlags_.get();
    for (auto &value : make_handle(message.notificationFlagsArray_.get(), lrtCount)) {
        value = (*(flags++));
    }

    /* == Set the execution task constraints == */

    return message;
}


