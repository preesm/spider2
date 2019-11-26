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
#include <archi/PE.h>
#include <thread/Thread.h>

/* === Static function === */

/* === Method(s) implementation === */

void spider::JITMSRTRunner::run(bool infiniteLoop) {
    bool run = true;
    bool canRun = true;
    while (run && !stop_) {
        /* == Check for notifications == */
        bool blockingPop = (infiniteLoop && (jobQueueCurrentPos_ < jobQueue_.size())) || !canRun;
        while (readNotification(blockingPop)) {
            blockingPop = false;
        }
        if (stop_) {
            if (log_enabled<LOG_LRT>()) {
                spider::log::info("Runner #%zu -> exiting from notification.\n", ix());
            }
            break;
        }
        /* == Sanity check of the job queue == */
        if (lastJobIx_ != UINT32_MAX && jobQueue_.size() >= lastJobIx_) {
            throwSpiderException("Runner #%zu -> job queue size larger than expected.", ix());
        }

        /* == If there is a job available, do it == */
        if (jobQueueCurrentPos_ < jobQueue_.size()) {
            if (log_enabled<LOG_LRT>()) {
                spider::log::info("Runner #%zu -> finished jobs.\n", ix());
            }
            auto &job = jobQueue_[jobQueueCurrentPos_];
            canRun = isJobRunnable(job);
            if (canRun) {
                if (log_enabled<LOG_LRT>()) {
                    spider::log::info("Runner #%zu -> starting job %" PRId32"\n ", ix(), jobQueueCurrentPos_);
                }
                /* == Run the job == */
                runJob(job);

                /* == Update current position in job queue == */
                if (log_enabled<LOG_LRT>()) {
                    spider::log::info("Runner #%zu -> finished job %" PRId32"\n ", ix(), jobQueueCurrentPos_);
                }
                jobQueueCurrentPos_++;
            }
        }

        /* == Exit condition based on infinite loop flag == */
        bool finishedIteration = (lastJobIx_ != UINT32_MAX) && (jobQueueCurrentPos_ == lastJobIx_);
        if (finishedIteration) {
            if (!infiniteLoop) {
                if (log_enabled<LOG_LRT>()) {
                    spider::log::info("Runner #%zu -> finished jobs.\n", ix());
                }
                run = false;
            } else {
                clearLocalJobStamps();
                localJobStampsArray_.set(UINT32_MAX);

                /* == Send END_ITERATION notification to GRT == */
                Notification notification{ NotificationType::LRT,
                                           LRTNotifification::FINISHED_ITERATION,
                                           static_cast<int32_t>(ix()) };
                const auto &target = spider::platform()->spiderGRTPE()->managingLRTIx();
                spider::rtPlatform()->communicator()->push(notification, static_cast<size_t>(target));
            }
            if (shouldBroadcast_) {
                shouldBroadcast_ = false;
                broadcastJobStamps();
            }
        }
    }
}

/* === Private method(s) implementation === */

void spider::JITMSRTRunner::runJob(const spider::JobMessage &job) {
    /* == Fetch input memory == */

    /* == Allocate output memory == */

    /* == Run the job == */

    /* == Send output data == */

    /* == Send output parameters == */

    /* == Notify other runtimes that need to know == */
    for (const auto &shouldNotify : job.LRTs2Notify_) {
        if (shouldNotify) {

        }
    }

    /* == Send traces == */
}

bool spider::JITMSRTRunner::isJobRunnable(const JobMessage &job) const {
    for (const auto &constraint : job.jobs2Wait_) {
        const auto &runnerIx = constraint.first;
        const auto &job2Wait = constraint.second;
        if (localJobStampsArray_[runnerIx] < static_cast<uint32_t>(job2Wait)) {
            if (runnerIx == ix()) {
                if (log_enabled<LOG_LRT>()) {
                    spider::log::error("Runner #%zu -> waiting for future self job..\n", ix());
                    spider::log::error(
                            "Runner #%zu -> current job stamp: %" PRId32" -- waited job stamp: %" PRId32"\n",
                            ix(), localJobStampsArray_[runnerIx], job2Wait);
                }
                throwSpiderException("Runner #%zu -> bad job ix.", ix());
            }
            if (log_enabled<LOG_LRT>()) {
                spider::log::info("Runner #%zu -> current job stamp %" PRId32"\n "
                                  "Runner #%zu -> waiting runner #%" PRIu32" -- job stamp %" PRId32"\n",
                                  ix(), jobQueueCurrentPos_, runnerIx, job2Wait);
            }
            return false;
        }
    }
    return true;
}

bool spider::JITMSRTRunner::readNotification(bool blocking) {
    Notification notification;
    if (blocking) {
        if (log_enabled<LOG_LRT>()) {
            spider::log::info("Runner #%zu -> waiting for notification...\n", ix());
        }
        spider::rtPlatform()->communicator()->pop(notification, ix());
    } else if (!spider::rtPlatform()->communicator()->try_pop(notification, ix())) {
        return false;
    }
    switch (notification.type_) {
        case NotificationType::LRT:
            readRuntimeNotification(notification);
            break;
        case NotificationType::TRACE:
            readTraceNotification(notification);
            break;
        case NotificationType::JOB:
            readJobNotification(notification);
            break;
        default:
            throwSpiderException("unhandled type of notification.");
    }
    return true;
}

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
            if (log_enabled<LOG_LRT>()) {
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
    switch (notification.subtype_) {
        case LRTNotifification::END_ITERATION:
            lastJobIx_ = static_cast<uint32_t >(notification.notificationIx_);
            if (log_enabled<LOG_LRT>()) {
                spider::log::info("Runner #%zu -> last job stamp: %" PRIu32"\n", ix(), lastJobIx_);
            }
            break;
        case LRTNotifification::RST_ITERATION:
            jobQueueCurrentPos_ = 0;
            break;
        case LRTNotifification::REPEAT_ITERATION_EN:
            break;
        case LRTNotifification::REPEAT_ITERATION_DIS:
            break;
        case LRTNotifification::PAUSE:
            // TODO: implement this
            break;
        case LRTNotifification::RESUME:
            // TODO: implement this
            break;
        case LRTNotifification::STOP:
            stop_ = true;
            break;
        default:
            throwSpiderException("unhandled type of LRTNotification.");
    }
}

void spider::JITMSRTRunner::readTraceNotification(spider::Notification &notification) {
    switch (notification.subtype_) {
        case TraceNotification::ENABLE:
            break;
        case TraceNotification::DISABLE:
            break;
        case TraceNotification::RST:
            break;
        default:
            throwSpiderException("unhandled type of TraceNotification.");
    }
}

