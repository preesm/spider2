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

/* === Static function === */

/* === Method(s) implementation === */

void spider::sched::Schedule::addTask(spider::unique_ptr<Task> task) {
    if (!task || UINT32_MAX == task->ix()) {
        return;
    }
    task->setIx(static_cast<u32>(tasks_.size()));
    tasks_.emplace_back(std::move(task));
}

/* === Private method(s) implementation === */

void spider::sched::Schedule::updateTaskAndSetReady(Task *task, size_t slave, u64 startTime, u64 endTime) {
    if (task->state() == TaskState::READY) {
        return;
    }
    const auto &pe = archi::platform()->peFromVirtualIx(slave);
    const auto &peIx = pe->virtualIx();

    /* == Set job information == */
    task->setMappedPE(pe);
    task->setStartTime(startTime);
    task->setEndTime(endTime);
//    task->setExecIx(static_cast<i32>(stats_.jobCount(peIx)));

    /* == Find minimal dependencies == */
//    task->updateExecutionConstraints();

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