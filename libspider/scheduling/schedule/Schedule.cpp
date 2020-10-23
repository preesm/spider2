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

#include <scheduling/schedule/Schedule.h>
#include <scheduling/task/Task.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <archi/PE.h>
#include <api/runtime-api.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/communicator/RTCommunicator.h>
#include <graphs/sched/SchedVertex.h>

/* === Static function === */

/* === Method(s) implementation === */

void spider::sched::Schedule::clear() {
    tasks_.clear();
    readyTaskVector_.clear();
    stats_.reset();
    scheduleGraph_->clear();
}

void spider::sched::Schedule::reset() {
    for (const auto &task : tasks_) {
        task->setState(TaskState::READY);
        readyTaskVector_.emplace_back(task.get());
    }
}

void spider::sched::Schedule::addTask(spider::unique_ptr<Task> task) {
    if (!task || UINT32_MAX != task->ix()) {
        return;
    }
    task->setIx(static_cast<u32>(tasks_.size()));
    tasks_.emplace_back(std::move(task));
}

void spider::sched::Schedule::updateTaskAndSetReady(Task *task, const PE *slave, u64 startTime, u64 endTime) {
    if (task->state() == TaskState::READY) {
        return;
    }
    const auto peIx = slave->virtualIx();

    /* == Set job information == */
    task->setMappedPE(slave);
    task->setStartTime(startTime);
    task->setEndTime(endTime);
    task->setJobExecIx(static_cast<u32>(stats_.jobCount(peIx)));
    task->updateDependenciesNotificationFlag();

    /* == Update schedule statistics == */
    stats_.updateStartTime(peIx, startTime);
    stats_.updateIDLETime(peIx, startTime - stats_.endTime(peIx));
    stats_.updateEndTime(peIx, endTime);
    stats_.updateLoadTime(peIx, endTime - startTime);
    stats_.updateJobCount(peIx);

    /* == Update job state == */
    task->setState(TaskState::READY);
    readyTaskVector_.emplace_back(task);
}

void spider::sched::Schedule::updateTaskAndSetReady(sched::Vertex *vertex, const PE *slave, u64 startTime, u64 endTime) {
    if (vertex->state() == State::READY) {
        return;
    }
    const auto peIx = slave->virtualIx();
    /* == Set job information == */
    vertex->setMappedPE(slave);
    vertex->setStartTime(startTime);
    vertex->setEndTime(endTime);
    vertex->setJobExecIx(static_cast<u32>(stats_.jobCount(peIx)));
//    task->updateDependenciesNotificationFlag();
    /* == Update schedule statistics == */
    stats_.updateStartTime(peIx, startTime);
    stats_.updateIDLETime(peIx, startTime - stats_.endTime(peIx));
    stats_.updateEndTime(peIx, endTime);
    stats_.updateLoadTime(peIx, endTime - startTime);
    stats_.updateJobCount(peIx);
    /* == Update job state == */
    vertex->setState(State::READY);
}

void spider::sched::Schedule::sendReadyTasks() {
    if (log::enabled<log::SCHEDULE>()) {
//        print();
    }
    const auto grtIx = archi::platform()->getGRTIx();
    auto *communicator = rt::platform()->communicator();
    for (auto *task : readyTaskVector_) {
        if (task->state() == TaskState::READY) {
            /* == Create job message and send the notification == */
            const auto mappedLRTIx = task->mappedLRT()->virtualIx();
            const auto messageIx = communicator->push(task->createJobMessage(), mappedLRTIx);
            communicator->push(Notification{ NotificationType::JOB_ADD, grtIx, messageIx }, mappedLRTIx);
            /* == Set job in TaskState::RUNNING == */
            task->setState(TaskState::RUNNING);
        }
    }
    /* == Reset ready task vector == */
    readyTaskVector_.clear();
}
