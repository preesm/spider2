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
    auto iterator = sortedTaskVector_.begin() + static_cast<long>(lastScheduledTask_);
    auto endIterator = sortedTaskVector_.begin() + static_cast<long>(lastSchedulableTask_);
    if (mode_ == JIT_SEND) {
        /* == Send LRT_START_ITERATION notification == */
        rt::platform()->sendStartIteration();
        while (iterator != endIterator) {
            auto &listVertex = (*(iterator++));
            Scheduler::taskMapper(listVertex.task_);
            /* == We are in JIT mode, we need to broadcast the job stamp == */
            listVertex.task_->enableBroadcast();
            /* == If we got an allocator let use it == */
            if (allocator_) {
                auto *vertex = listVertex.task_->vertex();
                for (auto &edge : vertex->outputEdgeVector()) {
                    listVertex.task_->addOutputFifo(allocator_->allocate(edge->sourceRateValue()));
                }
            }
            /* == Create job message and send it == */
            schedule_.sendReadyTasks();
        }
        /* == Send LRT_END_ITERATION notification == */
        rt::platform()->sendEndIteration();
    } else {
        while (iterator != endIterator) {
            auto &listVertex = (*(iterator++));
            Scheduler::taskMapper(listVertex.task_);
        }
        /* == Send LRT_START_ITERATION notification == */
        rt::platform()->sendStartIteration();
        /* == If we got an allocator let use it == */
        if (allocator_) {
            iterator = sortedTaskVector_.begin() + static_cast<long>(lastScheduledTask_);
            while (iterator != endIterator) {
                auto &listVertex = (*(iterator++));
                auto *vertex = listVertex.task_->vertex();
                for (auto &edge : vertex->outputEdgeVector()) {
                    listVertex.task_->addOutputFifo(allocator_->allocate(edge->sourceRateValue()));
                }
            }
        }
        /* == Creates all job messages and send them == */
        schedule_.sendReadyTasks();
        /* == Send LRT_END_ITERATION notification == */
        rt::platform()->sendEndIteration();
    }
    lastScheduledTask_ = lastSchedulableTask_;
    return schedule_;
}