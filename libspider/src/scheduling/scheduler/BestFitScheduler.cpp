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

#include <scheduling/scheduler/BestFitScheduler.h>
#include <scheduling/allocator/FifoAllocator.h>
#include <runtime/interface/Message.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/runner/RTRunner.h>
#include <api/runtime-api.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::Schedule &spider::BestFitScheduler::execute() {
    /* == Schedule and map the vertex onto available resource == */
    auto startIterator = sortedTaskVector_.begin() + static_cast<long>(lastScheduledTask_);
    auto endIterator = sortedTaskVector_.begin() + static_cast<long>(lastSchedulableTask_);
    if (mode_ == JIT_SEND) {
        std::for_each(startIterator, endIterator, [this](ListScheduler::ListTask &listTask) {
            /* == Do the mapping scheduling for the task == */
            Scheduler::mapTask(listTask.task_);
            /* == We are in JIT mode, we need to broadcast the job stamp == */
            listTask.task_->enableBroadcast();
            /* == Allocate output fifos for the task == */
            Scheduler::allocateTaskMemory(listTask.task_);
            /* == Create job message and send it == */
            schedule_.sendReadyTasks();
        });
    } else {
        /* == Do the mapping scheduling for all the tasks == */
        std::for_each(startIterator, endIterator, [this](ListScheduler::ListTask &listTask) {
            Scheduler::mapTask(listTask.task_);
        });
        /* == Allocate output fifos for all the tasks == */
        std::for_each(startIterator, endIterator, [this](ListScheduler::ListTask &listTask) {
            Scheduler::allocateTaskMemory(listTask.task_);
        });
        /* == Creates all job messages and send them == */
        schedule_.sendReadyTasks();
    }
    lastScheduledTask_ = lastSchedulableTask_;
    return schedule_;
}
