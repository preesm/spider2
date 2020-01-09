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

#include <thread/Thread.h>
#include <runtime/runner/JITMSRTRunner.h>
#include <runtime/interface/RTCommunicator.h>
#include <archi/MemoryInterface.h>
#include <archi/PE.h>

/* === Static function === */

/* === Method(s) implementation === */

void spider::JITMSRTRunner::run(bool infiniteLoop) {
    bool run = true;
    bool canRun = true;
    if (infiniteLoop) {
        log::info("Runner #%zu -> hello from thread %" PRId32"\n", ix(), this_thread::get_affinity());
    }
    while (run && !stop_) {
        /* == Check for notifications == */
        bool blockingPop = (infiniteLoop && (jobQueueCurrentPos_ <= jobQueue_.size())) || !canRun;
        const auto &currentNumberOfJob = jobQueue_.size();
        while (readNotification(blockingPop)) {
            blockingPop = false;
        }
        /* == Reorder the job received order to respect send / execute order == */
        if (jobQueue_.size() - currentNumberOfJob) {
            std::reverse(jobQueue_.rbegin(), jobQueue_.rbegin() + static_cast<long>(jobQueue_.size() -
                                                                                    currentNumberOfJob));
        }

        if (stop_) {
            if (log::enabled<log::Type::LRT>()) {
                log::info<log::Type::LRT>("Runner #%zu -> received STOP notification.\n", ix());
            }
            break;
        }
        /* == Sanity check of the job queue == */
        if (jobCount_ != SIZE_MAX && jobQueue_.size() > jobCount_) {
            throwSpiderException("Runner #%zu -> job queue size larger than expected.", ix());
        }

        /* == If there is a job available, do it == */
        if (jobQueueCurrentPos_ < jobQueue_.size()) {
            if (log::enabled<log::Type::LRT>()) {
                log::info<log::Type::LRT>("Runner #%zu -> %zu / %zu jobs done.\n", ix(), jobQueueCurrentPos_,
                                          jobCount_);
            }
            auto &job = jobQueue_[jobQueueCurrentPos_];
            canRun = isJobRunnable(job);
            if (canRun) {
                if (log::enabled<log::Type::LRT>()) {
                    log::info<log::Type::LRT>("Runner #%zu -> starting job %zu.\n", ix(), jobQueueCurrentPos_);
                }

                /* == Run the job == */
                runJob(job);

                /* == Update current position in job queue == */
                jobQueueCurrentPos_++;
                if (log::enabled<log::Type::LRT>()) {
                    log::info<log::Type::LRT>("Runner #%zu -> %zu / %zu jobs done.\n", ix(), jobQueueCurrentPos_,
                                              jobCount_);
                }
            }
        }

        /* == Exit condition based on infinite loop flag == */
        bool finishedIteration = (jobCount_ != SIZE_MAX) && (jobQueueCurrentPos_ == jobCount_);
        if (finishedIteration) {
            if (!infiniteLoop) {
                if (log::enabled<log::Type::LRT>()) {
                    log::info<log::Type::LRT>("Runner #%zu -> finished all jobs.\n", ix());
                }
                run = false;
            } else {
                clearLocalJobStamps();
                localJobStampsArray_.assign(SIZE_MAX);

                /* == Send END_ITERATION notification to GRT == */
                Notification notification{ NotificationType::LRT,
                                           LRTNotifification::FINISHED_ITERATION,
                                           ix() };
                const auto *target = archi::platform()->spiderGRTPE()->attachedLRT();
                rt::platform()->communicator()->push(notification, target->virtualIx());
            }
            if (shouldBroadcast_) {
                shouldBroadcast_ = false;
                broadcastJobStamps();
            }
        }
    }
}

void spider::JITMSRTRunner::begin() {
    if (affinity_ >= 0) {
        this_thread::set_affinity(affinity_);
    }
    auto *platform = archi::platform();
    if (attachedPE_ != platform->spiderGRTPE()) {
        run(true);
    }
}

/* === Private method(s) implementation === */

void spider::JITMSRTRunner::runJob(const JobMessage &job) {
    /* == Fetch input memory == */
    // TODO: add time monitoring
    spider::array<void *> inputBuffersArray{ job.inputFifoArray_.size(), nullptr, StackID::RUNTIME };
    auto inputBufferIterator = inputBuffersArray.begin();
    for (auto &inputFIFO : job.inputFifoArray_) {
        *(inputBufferIterator++) = inputFIFO.memoryInterface_->read(inputFIFO.virtualAddress_);
    }

    /* == Allocate output memory == */
    // TODO: add time monitoring
    spider::array<void *> outputBuffersArray{ job.outputFifoArray_.size(), nullptr, StackID::RUNTIME };
    auto outputBufferIterator = outputBuffersArray.begin();
    for (auto &outputFIFO : job.outputFifoArray_) {
        *(outputBufferIterator++) = outputFIFO.memoryInterface_->allocate(outputFIFO.virtualAddress_, outputFIFO.size_);
    }

    /* == Allocate output parameter memory == */
    // TODO: add time monitoring
    spider::array<int64_t> outputParams{ job.outputParamCount_, 0, StackID::RUNTIME };

    /* == Run the job == */
    // TODO: add time monitoring
    const auto &kernel = (rt::platform()->getKernel(job.kernelIx_));
    if (kernel) {
        (*kernel)(job.inputParams_.data(), outputParams.data(), inputBuffersArray.data(), outputBuffersArray.data());
    }

    /* == Deallocate input buffers == */
    // TODO: add time monitoring
    for (auto &inputFIFO : job.inputFifoArray_) {
        inputFIFO.memoryInterface_->deallocate(inputFIFO.virtualAddress_, inputFIFO.size_);
    }

    /* == Send output data == */
    // TODO: add time monitoring
    for (auto &outputFIFO : job.outputFifoArray_) {
        outputFIFO.memoryInterface_->write(outputFIFO.virtualAddress_);
    }

    /* == Notify other runtimes that need to know == */
    localJobStampsArray_[ix()] = job.ix_;
    size_t lrtIx = 0;
    for (const auto &shouldNotify : job.LRTs2Notify_) {
        if (shouldNotify) {
            Notification updateJobStampNotification{ NotificationType::JOB,
                                                     JobNotification::UPDATE_JOBSTAMP,
                                                     ix(),
                                                     job.ix_ };
            rt::platform()->communicator()->push(updateJobStampNotification, lrtIx);
            log::info<log::Type::LRT>("Runner #%zu -> notifying runner #%zu\n"
                                      "Runner #%zu -> sent job stamp: %zu\n",
                                      ix(), lrtIx,
                                      ix(), job.ix_);
        }
        lrtIx++;
    }

    /* == Send output parameters == */
    if (job.outputParamCount_) {
        const auto *spiderGRT = archi::platform()->spiderGRTPE()->attachedLRT();
        auto paramMessage = ParameterMessage(job.vertexIx_, std::move(outputParams));
        auto index = rt::platform()->communicator()->push(std::move(paramMessage), spiderGRT->virtualIx());
        rt::platform()->communicator()->pushParamNotification(attachedPE_->virtualIx(), index);
    }

    /* == Send traces == */
}

bool spider::JITMSRTRunner::isJobRunnable(const JobMessage &job) const {
    for (const auto &constraint : job.jobs2Wait_) {
        const auto &runner2WaitIx = constraint.first;
        const auto &job2Wait = constraint.second;
        if (localJobStampsArray_[runner2WaitIx] == SIZE_MAX ||
            localJobStampsArray_[runner2WaitIx] < job2Wait) {
            if (runner2WaitIx == ix()) {
                if (log::enabled<log::Type::LRT>()) {
                    log::error<log::Type::LRT>("Runner #%zu -> waiting for future self job..\n"
                                               "Runner #%zu -> current job stamp: %zu\n"
                                               "Runner #%zu -> waiting job stamp: %zu\n", ix(),
                                               ix(), jobQueueCurrentPos_,
                                               ix(), job2Wait);
                }
                throwSpiderException("Runner #%zu -> bad job ix.", ix());
            }
            if (log::enabled<log::Type::LRT>()) {
                log::info<log::Type::LRT>("Runner #%zu -> current job stamp %zu\n"
                                          "Runner #%zu -> waiting runner #%zu -- job stamp %zu\n",
                                          ix(), jobQueueCurrentPos_,
                                          ix(), runner2WaitIx, job2Wait);
            }
            return false;
        }
    }
    return true;
}

bool spider::JITMSRTRunner::readNotification(bool blocking) {
    Notification notification;
    if (blocking) {
        if (log::enabled<log::Type::LRT>()) {
            log::info<log::Type::LRT>("Runner #%zu -> waiting for notification...\n", ix());
        }
        rt::platform()->communicator()->pop(notification, ix());
    } else if (!rt::platform()->communicator()->try_pop(notification, ix())) {
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

void spider::JITMSRTRunner::readJobNotification(Notification &notification) {
    switch (notification.subtype_) {
        case JobNotification::ADD: {
            JobMessage message;
            rt::platform()->communicator()->pop(message, ix(), notification.notificationIx_);
            jobQueue_.emplace_back(std::move(message));
        }
            break;
        case JobNotification::CLEAR_QUEUE:
            jobCount_ = SIZE_MAX;
            clearLocalJobStamps();
            break;
        case JobNotification::JOB_COUNT:
            jobCount_ = notification.notificationIx_;
            break;
        case JobNotification::UPDATE_JOBSTAMP:
            if (notification.senderIx_ == SIZE_MAX) {
                throwSpiderException("Runner #%zu -> received notification from bad ix: %zu\n",
                                     ix(), notification.senderIx_);
            }
            localJobStampsArray_[notification.senderIx_] = notification.notificationIx_;
            if (log::enabled<log::Type::LRT>()) {
                log::info<log::Type::LRT>("Runner #%zu -> updating local job stamp of runner #%zu\n "
                                          "Runner #%zu -> received value: %zu\n",
                                          ix(), notification.senderIx_,
                                          ix(), notification.notificationIx_);
            }
            break;
        case JobNotification::DELAY_BROADCAST_JOBSTAMP:
            shouldBroadcast_ = true;
            break;
        case JobNotification::BROADCAST_JOBSTAMP:
            broadcastJobStamps();
            break;
        default:
            throwSpiderException("unhandled type of Job Notification.");
    }
}

void spider::JITMSRTRunner::readRuntimeNotification(Notification &notification) {
    switch (notification.subtype_) {
        case LRTNotifification::END_ITERATION:
            jobCount_ = notification.notificationIx_;
            if (log::enabled<log::Type::LRT>()) {
                log::info<log::Type::LRT>("Runner #%zu -> received number of jobs to do: %zu\n", ix(), jobCount_);
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
            throwSpiderException("unhandled type of Runtime Notification.");
    }
}

void spider::JITMSRTRunner::readTraceNotification(Notification &notification) {
    switch (notification.subtype_) {
        case TraceNotification::ENABLE:
            break;
        case TraceNotification::DISABLE:
            break;
        case TraceNotification::RST:
            break;
        default:
            throwSpiderException("unhandled type of Trace Notification.");
    }
}

