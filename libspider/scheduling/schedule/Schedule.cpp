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

#include <scheduling/schedule/Schedule.h>
#include <graphs/pisdf/Vertex.h>
#include <runtime/interface/RTCommunicator.h>
#include <runtime/platform/RTPlatform.h>
#include <api/archi-api.h>
#include <api/runtime-api.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

void spider::Schedule::clear() {
    taskVector_.clear();
    stats_.reset();
    lastRunTask_ = 0;
    readyJobCount_ = 0;
}

void spider::Schedule::reset() {
    for (auto &task : taskVector_) {
        task->setState(TaskState::READY);
    }
    lastRunTask_ = 0;
    readyJobCount_ = static_cast<long>(taskVector_.size());
}

void spider::Schedule::print() const {
    if (log::enabled<log::SCHEDULE>()) {
        const auto lrtCount = archi::platform()->LRTCount();
        for (const auto &task : taskVector_) {
            log::print<log::SCHEDULE>(log::magenta, "INFO: ", "Schedule: \n");
            log::print<log::SCHEDULE>(log::magenta, "INFO: ", "   >> task: %zu (runner: %zu)\n", task->ix(),
                                      task->mappedLrt(), task->name().c_str());
            for (size_t lrtIx = 0; lrtIx < lrtCount; ++lrtIx) {
                const auto taskIx = task->executionConstraint(lrtIx);
                if (taskIx >= 0) {
                    log::print<log::SCHEDULE>(log::magenta, "INFO: ",
                                              "           ----> task: %zu (runner: %zu) [%s]\n",
                                              taskVector_[static_cast<size_t>(taskIx)]->ix(), lrtIx,
                                              taskVector_[static_cast<size_t>(taskIx)]->name().c_str());
                }
            }
        }
    }
}

void spider::Schedule::addScheduleTask(ScheduleTask *task) {
    if (!task || (task->ix() >= 0)) {
        return;
    }
    task->setIx(static_cast<int32_t>(taskVector_.size()));
    taskVector_.emplace_back(task);
}

void spider::Schedule::updateTaskAndSetReady(size_t taskIx, size_t slave, uint64_t startTime, uint64_t endTime) {
    auto &task = taskVector_.at(taskIx);
    const auto &pe = archi::platform()->peFromVirtualIx(slave);
    const auto &peIx = pe->virtualIx();

    /* == Set job information == */
    task->setMappedLrt(pe->attachedLRT()->virtualIx());
    task->setMappedPE(peIx);
    task->setStartTime(startTime);
    task->setEndTime(endTime);

    /* == Set should notify value for previous jobs == */
    auto *platform = archi::platform();
    const auto lrtCount = platform->LRTCount();
    for (size_t i = 0; i < lrtCount; ++i) {
        const auto constraint = task->executionConstraint(i);
        if (constraint >= 0) {
            taskVector_[static_cast<size_t>(constraint)]->setNotificationFlag(task->mappedLrt(), true);
        }
    }

    /* == Update schedule statistics == */
    stats_.updateStartTime(peIx, startTime);
    stats_.updateIDLETime(peIx, startTime - stats_.endTime(peIx));
    stats_.updateEndTime(peIx, endTime);
    stats_.updateLoadTime(peIx, endTime - startTime);
    stats_.updateJobCount(peIx);

    /* == Update job state == */
    task->setState(TaskState::READY);
    readyJobCount_++;
}

void spider::Schedule::sendReadyTasks() {
    const auto grtIx = archi::platform()->spiderGRTPE()->virtualIx();
    auto *communicator = rt::platform()->communicator();
    /* == Compute the iterator on ready only jobs == */
    auto startIterator = std::begin(taskVector_) + lastRunTask_;
    auto endIterator = startIterator + readyJobCount_;
    for (auto it = startIterator; it < endIterator; ++it) {
        auto *task = it->get();
        /* == Create job message and send the notification == */
        const auto messageIx = communicator->push(task->createJobMessage(), task->mappedLrt());
        communicator->push(Notification(NotificationType::JOB_ADD, grtIx, messageIx), task->mappedLrt());
        /* == Set job in TaskState::RUNNING == */
        task->setState(TaskState::RUNNING);
    }
    /* == Reset last job and ready count == */
    lastRunTask_ += readyJobCount_;
    readyJobCount_ = 0;
}
