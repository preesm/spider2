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

#include <runtime/runner/RTRunner.h>
#include <api/runtime-api.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/interface/RTCommunicator.h>
#include <runtime/interface/Notification.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <archi/PE.h>
#include <containers/array_handle.h>

/* === Define(s) === */

#define LOG_NOTIFY() \
    if (log::enabled<log::LRT>()) {\
        log::info<log::LRT>("Runner #%zu -> notifying runner #%zu\n"\
                            "Runner #%zu -> sent job stamp: %zu\n",\
                            ix(), lrtIx,\
                            ix(), jobIx);\
    }

/* === Function(s) definition === */

spider::RTRunner::RTRunner(PE *attachedPe, size_t runnerIx, i32 affinity) : jobQueue_{ sbc::vector < JobMessage,
                                                                                       StackID::RUNTIME > { }},
                                                                            localJobStampsArray_{ array < size_t >
                                                                                                  { archi::platform()->LRTCount(),
                                                                                                    SIZE_MAX,
                                                                                                    StackID::RUNTIME }},
                                                                            attachedPE_{ attachedPe },
                                                                            runnerIx_{ runnerIx },
                                                                            affinity_{ affinity } {
}

void spider::RTRunner::clearLocalJobStamps() {
    localJobStampsArray_.assign(SIZE_MAX);
}

void spider::RTRunner::clearJobQueue() {
    jobQueueCurrentPos_ = 0;
    jobQueue_.clear();
}

void spider::RTRunner::broadcastCurrentJobStamp() const {
    Notification broadcastNotification{ NotificationType::JOB_UPDATE_JOBSTAMP,
                                        ix(),
                                        jobQueueCurrentPos_ };
    for (size_t i = 0; i < archi::platform()->LRTCount(); ++i) {
        if (i != ix()) {
            rt::platform()->communicator()->push(broadcastNotification, i);
        }
    }
}

void spider::RTRunner::sendFinishedNotification() const {
    Notification notification{ NotificationType::LRT_FINISHED_ITERATION, ix() };
    const auto *grt = archi::platform()->spiderGRTPE()->attachedLRT();
    rt::platform()->communicator()->push(notification, grt->virtualIx());
}

void spider::RTRunner::sendJobStampNotification(bool *notificationFlags, size_t jobIx) const {
    size_t lrtIx = 0;
    for (const auto &shouldNotify : array_handle<bool>{ notificationFlags, archi::platform()->LRTCount() }) {
        if (shouldNotify && lrtIx != ix()) {
            Notification updateJobStampNotification{ NotificationType::JOB_UPDATE_JOBSTAMP,
                                                     ix(),
                                                     jobIx };
            rt::platform()->communicator()->push(updateJobStampNotification, lrtIx);
            LOG_NOTIFY();
        }
        lrtIx++;
    }
}

void spider::RTRunner::sendParameters(size_t vertexIx, array<int64_t> &parameters) const {
    if (!parameters.empty()) {
        const auto *spiderGRT = archi::platform()->spiderGRTPE()->attachedLRT();
        auto paramMessage = ParameterMessage(vertexIx, std::move(parameters));
        auto index = rt::platform()->communicator()->push(std::move(paramMessage), spiderGRT->virtualIx());
        rt::platform()->communicator()->pushParamNotification(attachedPE_->virtualIx(), index);
    }
}
