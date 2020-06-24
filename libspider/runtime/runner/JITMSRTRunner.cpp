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

#include <thread/Thread.h>
#include <api/runtime-api.h>
#include <runtime/runner/JITMSRTRunner.h>
#include <runtime/interface/RTCommunicator.h>
#include <runtime/platform/RTPlatform.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/MemoryInterface.h>
#include <api/config-api.h>

/* === Define(s) === */

#define LOG_WAIT() \
    if (log::enabled<log::LRT>()) {\
        log::info<log::LRT>("Runner #%zu -> waiting for notification...\n", ix());\
    }

#define LOG_STOP() \
    if (log::enabled<log::LRT>()) {\
        log::info<log::LRT>("Runner #%zu -> received STOP notification.\n", ix());\
    }

#define LOG_JOB_PUSH() \
    if (log::enabled<log::LRT>()) {\
        log::print<log::LRT>(log::blue, "INFO", "Runner #%zu -> received %zu new jobs.\n", ix(),\
                jobQueue_.size() - currentNumberOfJob);\
        log::print<log::LRT>(log::blue, "INFO", "Runner #%zu -> received jobs:\n", ix());\
        for (size_t i = 0; i < (jobQueue_.size() - currentNumberOfJob); ++i) {\
            log::print<log::LRT>(log::blue, "INFO", "Runner #%zu ->          >> %zu\n", ix(),\
                    jobQueue_[currentNumberOfJob + i].ix_);\
        }\
    }

#define LOG_STATUS_ERROR() \
    if (log::enabled<log::LRT>()) {\
        log::error<log::LRT>("Runner #%zu -> waiting for future self job..\n"\
                             "Runner #%zu -> current job stamp: %zu\n"\
                             "Runner #%zu -> waiting job stamp: %zu\n", ix(),\
                             ix(), jobQueueCurrentPos_,\
                             ix(), job2Wait);\
    }

#define LOG_STATUS() \
    if (log::enabled<log::LRT>()) {\
        log::info<log::LRT>("Runner #%zu -> current job stamp %zu\n"\
                            "Runner #%zu -> waiting runner #%zu -- job stamp %zu\n",\
                            ix(), localJobStamp,\
                            ix(), runner2WaitIx, job2Wait);\
    }

#define LOG_JOB_START() \
    if (log::enabled<log::LRT>()) {\
        log::info<log::LRT>("Runner #%zu -> starting job %zu.\n", ix(), job.ix_);\
    }

#define LOG_JOB_END() \
    if (log::enabled<log::LRT>()) { \
        if (jobCount_ != 0) {\
            log::info<log::LRT>("Runner #%zu -> %zu / %zu jobs done.\n", ix(), jobQueueCurrentPos_, jobCount_); \
        } else { \
            log::info<log::LRT>("Runner #%zu -> %zu / ? jobs done.\n", ix(), jobQueueCurrentPos_);\
        }\
    }

#define LOG_END_ITER() \
    if (log::enabled<log::LRT>()) {\
        log::info<log::LRT>("Runner #%zu -> finished all jobs.\n", ix());\
    }

#define LOG_UPDATE() \
     if (log::enabled<log::LRT>()) {\
        log::info<log::LRT>("Runner #%zu -> updating local job stamp of runner #%zu\n"\
                            "Runner #%zu -> received value: %zu\n",\
                            ix(), lrtIx,\
                            ix(), jobStampValue);\
    }

#define LOG_UNHANDLED() \
     if (log::enabled<log::LRT>()) {\
        log::info<log::LRT>("Runner #%zu -> received unhandled type of notification: #%s\n",\
                            ix(), notificationToString(notification.type_));\
    }

#define LOG_NOTIFICATION() \
     if (log::enabled<log::LRT>()) {\
        log::info<log::LRT>("Runner #%zu -> received notification: #%s\n",\
                            ix(), notificationToString(notification.type_));\
    }

#define LOG_JOB() \
    if (log::enabled<log::LRT>() && spider::api::verboseEnabled()) {\
        log::print<log::LRT>(log::blue, "INFO", "Runner #%zu -> Task: %zu\n", ix(), job.ix_);\
        log::print<log::LRT>(log::blue, "INFO", "Runner #%zu -> Constraints:\n", ix());\
        for (const auto &constraint : job.execConstraints_) {\
            log::print<log::LRT>(log::blue, "INFO", "Runner #%zu -> >> job %zu on runner #%zu\n", ix(),\
                    constraint.jobToWait_, constraint.lrtToWait_);\
        }\
        log::print<log::LRT>(log::blue, "INFO", "Runner #%zu -> Input Fifo(s):\n", ix());\
        for (auto &fifo : job.inputFifoArray_) {\
            log::print<log::LRT>(log::blue, "INFO", "Runner #%zu -> >> size: %zu -- address: %zu -- offset: %zu\n", ix(), fifo.size_,\
            fifo.virtualAddress_, fifo.offset_);\
        }\
        log::print<log::LRT>(log::blue, "INFO", "Runner #%zu -> Output Fifo(s):\n", ix());\
        for (auto &fifo : job.outputFifoArray_) {\
            log::print<log::LRT>(log::blue, "INFO", "Runner #%zu -> >> size: %zu -- address: %zu -- offset: %zu\n", ix(), fifo.size_,\
            fifo.virtualAddress_, fifo.offset_);\
        }\
    }

/* === Static function === */

spider::array<void *> createInputFifos(const spider::array<spider::RTFifo> &fifos,
                                       spider::MemoryInterface *memoryInterface) {
    spider::array<void *> inputBuffersArray{ fifos.size(), nullptr, StackID::RUNTIME };
    std::transform(std::begin(fifos), std::end(fifos), std::begin(inputBuffersArray),
                   [&memoryInterface](const spider::RTFifo &fifo) -> void * {
                       if (!fifo.size_) {
                           return nullptr;
                       }
                       void *buffer = nullptr;
                       if (fifo.attribute_ == spider::FifoAttribute::RW_EXT) {
                           buffer = spider::archi::platform()->getExternalBuffer(fifo.virtualAddress_);
                       } else {
                           buffer = memoryInterface->read(fifo.virtualAddress_, fifo.count_);
                       }
                       return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + fifo.offset_);
                   });
    return inputBuffersArray;
}

spider::array<void *> createOutputFifos(const spider::array<spider::RTFifo> &fifos,
                                        spider::MemoryInterface *memoryInterface) {
    spider::array<void *> outputBuffersArray{ fifos.size(), nullptr, StackID::RUNTIME };
    std::transform(std::begin(fifos), std::end(fifos), std::begin(outputBuffersArray),
                   [&memoryInterface](const spider::RTFifo &fifo) -> void * {
                       if (fifo.attribute_ == spider::FifoAttribute::RW_OWN) {
                           return memoryInterface->allocate(fifo.virtualAddress_, fifo.size_, fifo.count_);
                       }
                       void *buffer = nullptr;
                       if (fifo.attribute_ == spider::FifoAttribute::RW_EXT) {
                           buffer = spider::archi::platform()->getExternalBuffer(fifo.virtualAddress_);
                       } else {
                           buffer = memoryInterface->read(fifo.virtualAddress_);
                       }
                       return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(buffer) + fifo.offset_);
                   });
    return outputBuffersArray;
}

/* === Method(s) implementation === */

void spider::JITMSRTRunner::run(bool infiniteLoop) {
    bool run = true;
    if (infiniteLoop) {
        log::info("Runner #%zu -> hello from thread %"
                  PRId32
                  "\n", ix(), this_thread::get_affinity());
    }
    bool waitForJob = false;
    while (run && !stop_) {
        /* == Check for notifications == */
        bool blockingPop = (infiniteLoop && finished_) || waitForJob;
        while (!stop_ && readNotification(blockingPop)) {
            blockingPop = pause_;
        }
        if (stop_) {
            LOG_STOP();
            return;
        }

        /* == If there is a job available, do it == */
        if (start_ && (jobQueueCurrentPos_ != jobQueue_.size())) {
            auto &job = jobQueue_[jobQueueCurrentPos_];
            waitForJob = !isJobRunnable(job);
            if (!waitForJob) {
                /* == Run the job == */
                LOG_JOB_START();
                runJob(job);
                lastJobStamp_ = job.ix_;
                jobQueueCurrentPos_++;
                LOG_JOB_END();
            }
        }
        bool finishedIteration =
                (receivedEnd_ && (jobQueueCurrentPos_ == jobCount_)) || (!infiniteLoop && jobQueue_.empty());
        if (finishedIteration) {
            /* == Check if we need to broadcast == */
            if (shouldBroadcast_) {
                shouldBroadcast_ = false;
                broadcastCurrentJobStamp();
            }

            /* == Send finished notification (after broadcast to avoid potential miss-synchronization) == */
            sendFinishedNotification();

            /* == Reset state == */
            if (!repeat_) {
                clearJobQueue();
            }
            finished_ = true;
            start_ = false;
            receivedEnd_ = false;

            /* == log == */
            LOG_END_ITER();

            /* == Exit condition based on infinite loop flag == */
            if (!infiniteLoop) {
                return;
            }
        }
        run = !finishedIteration || infiniteLoop;
    }
}

void spider::JITMSRTRunner::begin() {
    if (affinity_ >= 0) {
        this_thread::set_affinity(affinity_);
    }
    const auto *platform = archi::platform();
    if (attachedPE_ != platform->spiderGRTPE()) {
        run(true);
    }
}

/* === Private method(s) implementation === */

void spider::JITMSRTRunner::runJob(const JobMessage &job) {
    LOG_JOB();
    TraceMessage msgMemory{ };
    if (trace_) {
        msgMemory.taskIx_ = job.vertexIx_;
        msgMemory.startTime_ = time::now();
    }
    /* == Create input buffers == */
    auto inputBuffersArray = createInputFifos(job.inputFifoArray_, attachedPE_->cluster()->memoryInterface());

    /* == Create output buffers == */
    auto outputBuffersArray = createOutputFifos(job.outputFifoArray_, attachedPE_->cluster()->memoryInterface());

    /* == Allocate output parameter memory == */
    array<int64_t> outputParams{ static_cast<size_t>(job.outputParamCount_), 0, StackID::RUNTIME };

    if (trace_) {
        msgMemory.endTime_ = time::now();
        auto *communicator = rt::platform()->communicator();
        auto msgIx = communicator->push(msgMemory, archi::platform()->getGRTIx());
        communicator->pushTraceNotification(Notification{ NotificationType::TRACE_MEMORY, ix(), msgIx });
    }

    /* == Run the job == */
    const auto &kernel = (rt::platform()->getKernel(job.kernelIx_));
    if (kernel) {
        TraceMessage msgExec{ };
        if (trace_) {
            msgExec.taskIx_ = job.vertexIx_;
            msgExec.startTime_ = time::now();
        }
        (*kernel)(job.inputParams_.data(), outputParams.data(), inputBuffersArray.data(), outputBuffersArray.data());
        if (trace_) {
            msgExec.endTime_ = time::now();
            auto *communicator = rt::platform()->communicator();
            auto msgIx = communicator->push(msgExec, archi::platform()->getGRTIx());
            communicator->pushTraceNotification(Notification{ NotificationType::TRACE_TASK, ix(), msgIx });
        }
    }

    /* == Deallocate input buffers == */
    for (auto &inputFIFO : job.inputFifoArray_) {
        if (inputFIFO.attribute_ == FifoAttribute::RW_OWN) {
            auto *memoryInterface = attachedPE_->cluster()->memoryInterface();
            memoryInterface->deallocate(inputFIFO.virtualAddress_, inputFIFO.size_);
        }
    }

    /* == Notify other runtimes that need to know == */
    updateJobStamp(ix(), job.ix_);
    sendJobStampNotification(job.notificationFlagsArray_.get(), job.ix_);

    /* == Send output parameters == */
    sendParameters(job.vertexIx_, outputParams);

    /* == Send traces == */
}

bool spider::JITMSRTRunner::isJobRunnable(const JobMessage &job) const {
    for (const auto &constraint : job.execConstraints_) {
        const auto runner2WaitIx = constraint.lrtToWait_;
        const auto job2Wait = constraint.jobToWait_;
        const auto localJobStamp = localJobStampsArray_[runner2WaitIx];
        if ((localJobStamp == SIZE_MAX) || (localJobStamp < job2Wait)) {
            if (runner2WaitIx == ix()) {
                LOG_STATUS_ERROR();
                throwSpiderException("Runner #%zu -> bad job ix.", ix());
            }
            LOG_STATUS();
            return false;
        }
    }
    return true;
}

bool spider::JITMSRTRunner::readNotification(bool blocking) {
    Notification notification;
    if (blocking) {
        LOG_WAIT();
        rt::platform()->communicator()->pop(notification, ix());
    } else if (!rt::platform()->communicator()->try_pop(notification, ix())) {
        return false;
    }
    LOG_NOTIFICATION();
    switch (notification.type_) {
        case NotificationType::LRT_START_ITERATION:
            if (finished_) {
                start_ = true;
                finished_ = false;
                jobCount_ = 0;
                jobQueueCurrentPos_ = 0;
                break;
            } else {
                rt::platform()->communicator()->push(notification, ix());
                return false;
            }
        case NotificationType::LRT_END_ITERATION:
            if (!start_) {
                throwSpiderException("Runner #%zu -> received LRT_END_ITERATION before LRT_START_ITERATION.", ix());
            }
            receivedEnd_ = true;
            jobCount_ = jobQueue_.size();
            break;
        case NotificationType::LRT_CLEAR_ITERATION:
            clear();
            break;
        case NotificationType::LRT_RST_ITERATION:
            reset();
            break;
        case NotificationType::LRT_FINISHED_ITERATION:
            if (attachedPE_ != archi::platform()->spiderGRTPE()) {
                throwSpiderException("Runner #%zu --> received notification for GRT.", ix());
            }
            rt::platform()->registerFinishedRunner(notification.senderIx_);
            break;
        case NotificationType::LRT_REPEAT_ITERATION_EN:
            repeat_ = true;
            break;
        case NotificationType::LRT_REPEAT_ITERATION_DIS:
            repeat_ = false;
            break;
        case NotificationType::LRT_STOP:
            stop_ = true;
            break;
        case NotificationType::LRT_PAUSE:
            pause_ = true;
            break;
        case NotificationType::LRT_RESUME:
            pause_ = false;
            break;
        case NotificationType::TRACE_ENABLE:
            trace_ = true;
            break;
        case NotificationType::TRACE_DISABLE:
            trace_ = false;
            break;
        case NotificationType::JOB_ADD: {
            JobMessage message;
            rt::platform()->communicator()->pop(message, ix(), notification.notificationIx_);
            if (start_) {
                jobQueue_.emplace_back(std::move(message));
            }
        }
            break;
        case NotificationType::JOB_CLEAR_QUEUE:
            clearJobQueue();
            jobCount_ = 0;
            break;
        case NotificationType::JOB_BROADCAST_JOBSTAMP:
            broadcastCurrentJobStamp();
            break;
        case NotificationType::JOB_DELAY_BROADCAST_JOBSTAMP:
            shouldBroadcast_ = true;
            break;
        case NotificationType::JOB_UPDATE_JOBSTAMP:
            if (notification.senderIx_ == SIZE_MAX) {
                throwSpiderException("Runner #%zu -> received notification from bad ix: %zu\n",
                                     ix(), notification.senderIx_);
            }
            updateJobStamp(notification.senderIx_, notification.notificationIx_);
            break;
        default:
            LOG_UNHANDLED();
    }
    return true;
}

void spider::JITMSRTRunner::updateJobStamp(size_t lrtIx, size_t jobStampValue) {
    if (localJobStampsArray_.at(lrtIx) == SIZE_MAX ||
        (localJobStampsArray_[lrtIx] < jobStampValue)) {
        localJobStampsArray_[lrtIx] = jobStampValue;
        LOG_UPDATE();
    }
}

void spider::JITMSRTRunner::clear() {
    clearLocalJobStamps();
    clearJobQueue();
    jobCount_ = 0;
    lastJobStamp_ = SIZE_MAX;
    shouldBroadcast_ = false;
    start_ = false;
    receivedEnd_ = false;
    finished_ = true;
}

void spider::JITMSRTRunner::reset() {
    RTRunner::reset();
    jobCount_ = 0;
    lastJobStamp_ = SIZE_MAX;
    shouldBroadcast_ = false;
    start_ = false;
    receivedEnd_ = false;
    finished_ = true;
}

