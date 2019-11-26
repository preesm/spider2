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

#include <runtime/runner/JITMSRTRunner.h>
#include <runtime/platform/ThreadRTPlatform.h>
#include <runtime/interface/RTCommunicator.h>
#include <thread/Thread.h>

/* === Static function === */

/* === Method(s) implementation === */

void spider::JITMSRTRunner::run(bool infiniteLoop) {
    bool run = true;
    while (run) {
        /* == Check for notifications == */

        /* == If there is a job available, do it == */
        if (jobQueueCurrentPos_ < jobQueue_.size()) {
            if (spider::api::verbose() && log_enabled<LOG_LRT>()) {
                spider::log::info("Runner #%zu -> finished jobs.\n", ix());
            }
            auto &job = jobQueue_[jobQueueCurrentPos_];
            bool canRun = true;
            for (const auto &constraint : job.jobs2Wait_) {
                const auto &runnerIx = constraint.first;
                const auto &job2Wait = constraint.second;
                if (localJobStampsArray_[runnerIx] < static_cast<uint32_t>(job2Wait)) {
                    if (runnerIx == ix()) {
                        if (spider::api::verbose() && log_enabled<LOG_LRT>()) {
                            spider::log::error("Runner #%zu -> waiting for future self job..\n", ix());
                            spider::log::error(
                                    "Runner #%zu -> current job stamp: %" PRId32" -- waited job stamp: %" PRId32"\n",
                                    ix(), localJobStampsArray_[runnerIx], job2Wait);
                        }
                        throwSpiderException("Runner #%zu -> bad job ix.", ix());
                    }
                    if (spider::api::verbose() && log_enabled<LOG_LRT>()) {
                        spider::log::info("Runner #%zu -> current job stamp %" PRId32"\n "
                                          "Runner #%zu -> waiting runner #%" PRIu32" -- job stamp %" PRId32"\n",
                                          ix(), jobQueueCurrentPos_, runnerIx, job2Wait);
                    }
                    canRun = false;
                    break;
                }
            }
            if (canRun) {
                if (spider::api::verbose() && log_enabled<LOG_LRT>()) {
                    spider::log::info("Runner #%zu -> starting job %" PRId32"\n ", ix(), jobQueueCurrentPos_);
                }
                /* == Run the job == */

                /* == Update current position in job queue == */
                if (spider::api::verbose() && log_enabled<LOG_LRT>()) {
                    spider::log::info("Runner #%zu -> finished job %" PRId32"\n ", ix(), jobQueueCurrentPos_);
                }
                jobQueueCurrentPos_++;
            }
        }

        /* == Exit condition based on infinite loop flag == */
        bool finishedIteration = (lastJobIx_ != UINT32_MAX) && (jobQueueCurrentPos_ == lastJobIx_);
        if (finishedIteration) {
            if (!infiniteLoop) {
                if (spider::api::verbose() && log_enabled<LOG_LRT>()) {
                    spider::log::info("Runner #%zu -> finished jobs.\n", ix());
                }
                run = false;
            } else {
                clearLocalJobStamps();
                localJobStampsArray_.set(UINT32_MAX);
                /* == Send END_ITERATION notification to GRT == */
            }
            if (shouldBroadcast_) {
                shouldBroadcast_ = false;
                broadcastJobStamps();
            }
        }
    }
}

/* === Private method(s) implementation === */

void spider::JITMSRTRunner::readJobNotification(spider::Notification &notification) {
    switch (notification.subtype_) {
        case JobNotification::ADD: {
            JobMessage message;
            spider::rtPlatform()->communicator()->pop(message, ix(), notification.notificationIx_);
            jobQueue_.emplace_back(std::move(message));
        }
            break;
        case JobNotification::CLEAR_QUEUE:
            lastJobIx_ = UINT32_MAX;
            clearLocalJobStamps();
            break;
        case JobNotification::LAST_ID:
            lastJobIx_ = static_cast<uint32_t>(notification.notificationIx_);
            break;
        case JobNotification::UPDATE_JOBSTAMP:
            if (notification.senderIx_ < 0) {
                throwSpiderException("Runner #%zu -> received notification from bad ix: %"
                                             PRId32
                                             "\n", ix(), notification.senderIx_);
            }
            localJobStampsArray_[static_cast<size_t>(notification.senderIx_)] = static_cast<uint32_t>(notification.notificationIx_);
            if (spider::api::verbose() && log_enabled<LOG_LRT>()) {
                spider::log::info(
                        "Runner #%zu -> updating local job stamp of runner #%" PRId32" -- value: %" PRId32"\n",
                        ix(), notification.senderIx_, notification.notificationIx_);
            }
            break;
        case JobNotification::DELAY_BROADCAST_JOBSTAMP:
            shouldBroadcast_ = true;
            break;
        case JobNotification::BROADCAST_JOBSTAMP:
            broadcastJobStamps();
            break;
        default:
            throwSpiderException("unhandled type of JobNotification.");
    }
}

void spider::JITMSRTRunner::readRuntimeNotification(spider::Notification &notification) {

}

void spider::JITMSRTRunner::readTraceNotification(spider::Notification &notification) {

}
