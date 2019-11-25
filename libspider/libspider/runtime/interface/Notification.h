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
 * encouraged to load and test t
 *
 * he software's suitability as regards their
 * requirements in conditions enabling the security of their systems and/or
 * data to be ensured and,  more generally, to use and operate it in the
 * same conditions as regards security.
 *
 * The fact that you are presently reading this means that you have had
 * knowledge of the CeCILL license and that you accept its terms.
 */
#ifndef SPIDER2_NOTIFICATION_H
#define SPIDER2_NOTIFICATION_H

/* === Include(s) === */

#include <cstdint>

namespace spider {

    /* === Enumaration(s) definition === */

    enum class NotificationType : uint16_t {
        LRT,              /*!< LRT management related type of notification */
        TRACE,            /*!< Trace related type of notification */
        JOB,              /*!< Job related type of notification */
        UNDEFINED,        /*!< Undefined type of notification */
        First = LRT,      /*!< Sentry for EnumIterator::begin */
        Last = UNDEFINED, /*!< Sentry for EnumIterator::end */
    };

    enum LRTNotifification : uint16_t {
        END_ITERATION = 0,    /*!< Cross-check signal sent after last JOB (if JOB_LAST_ID was not received) */
        REPEAT_ITERATION_EN,  /*!< Signal LRT to repeat its complete iteration (indefinitely) */
        REPEAT_ITERATION_DIS, /*!< Signal LRT to stop repeating iteration */
        FINISHED_ITERATION,   /*!< Signal that given LRT has finished its iteration */
        RST_ITERATION,        /*!< Signal LRT to restart current iteration */
        STOP,                 /*!< Signal LRT to stop */
        PAUSE,                /*!< Signal LRT to freeze */
        RESUME,               /*!< Signal LRT to un-freeze */
    };

    enum JobNotification : uint16_t {
        ADD = 0,                    /*!< Signal LRT that a job is available in shared queue */
        LAST_ID,                    /*!< Signal LRT what is the last job ID */
        CLEAR_QUEUE,                /*!< Signal LRT to clear its job queue (if LRT_REPEAT_ITERATION_EN, signal is ignored) */
        SENT_PARAM,                 /*!< Signal that LRT sent a ParameterMessage */
        BROADCAST_JOBSTAMP,         /*!< Signal LRT to broadcast its job stamp to everybody */
        DELAY_BROADCAST_JOBSTAMP,   /*!< Signal LRT to broadcast its job stamp to everybody after last job has been done */
        UPDATE_JOBSTAMP,            /*!< Signal LRT that an update of job stamp is pending */
    };

    enum TraceNotification : uint16_t {
        TRACE_ENABLE = 0,    /*!< Signal LRT to enable its trace */
        TRACE_DISABLE,       /*!< Signal LRT to disable its trace */
        TRACE_RST,           /*!< Signal LRT to reset its trace */
        TRACE_SENT,          /*!< Signal that a trace has been sent */
    };


    /* === Structure(s) definition === */

    struct Notification {

        Notification() = default;

        Notification(const Notification &) = default;

        Notification(Notification &&) = default;

        explicit Notification(NotificationType type,
                              uint16_t subtype = UINT16_MAX,
                              int32_t senderIx = -1,
                              int32_t notificationIx = -1) : type_{ type },
                                                             subtype_{ subtype },
                                                             senderIx_{ senderIx },
                                                             notificationIx_{ notificationIx } {

        }

        ~Notification() = default;

        /* === Struct member(s) === */

        NotificationType type_ = NotificationType::UNDEFINED;    /*!< Primary type of the notification (ex: NotificationType::JOB). */
        uint16_t subtype_ = UINT16_MAX;                          /*!< Sub-type of the notification (ex: JobNotification::ADD). */
        int32_t senderIx_ = -1;                                  /*!< ID of the sender of the notification */
        int32_t notificationIx_ = -1;                            /*!< Index of the notification to fetch (may be used for direct value passing for some notification). */
    };
}

#endif //SPIDER2_NOTIFICATION_H
