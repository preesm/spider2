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
        LRT_END_ITERATION = 0,          /*!< Cross-check signal sent after last job */
        LRT_START_ITERATION,            /*!< Cross-check signal sent before first job */
        LRT_REPEAT_ITERATION_EN,        /*!< Signal LRT to repeat its complete iteration (indefinitely) */
        LRT_REPEAT_ITERATION_DIS,       /*!< Signal LRT to stop repeating iteration */
        LRT_FINISHED_ITERATION,         /*!< Signal that given LRT has finished its iteration */
        LRT_RST_ITERATION,              /*!< Signal LRT to restart current iteration */
        LRT_CLEAR_ITERATION,            /*!< Signal LRT to clear information relative to the last iteration */
        LRT_STOP,                       /*!< Signal LRT to stop */
        LRT_PAUSE,                      /*!< Signal LRT to freeze */
        LRT_RESUME,                     /*!< Signal LRT to un-freeze */
        TRACE_ENABLE,                   /*!< Signal LRT to enable its trace */
        TRACE_DISABLE,                  /*!< Signal LRT to disable its trace */
        TRACE_TASK,                     /*!< Signal that an execution trace of a task has been sent */
        TRACE_SCHEDULE,                 /*!< Signal that an execution trace of scheduling has been sent */
        TRACE_TRANSFO,                  /*!< Signal that an execution trace of transformation step has been sent */
        TRACE_PARAM,                    /*!< Signal that an execution trace of param resolution has been sent */
        TRACE_MEMORY,                   /*!< Signal that an execution trace of memory alloc has been sent */
        JOB_ADD,                        /*!< Signal LRT that a job is available in shared queue */
        JOB_CLEAR_QUEUE,                /*!< Signal LRT to clear its job queue (if LRT_REPEAT_ITERATION_EN, signal is ignored) */
        JOB_SENT_PARAM,                 /*!< Signal that LRT sent a ParameterMessage */
        JOB_BROADCAST_JOBSTAMP,         /*!< Signal LRT to broadcast its job stamp to everybody */
        JOB_DELAY_BROADCAST_JOBSTAMP,   /*!< Signal LRT to broadcast its job stamp to everybody after last job has been done */
        JOB_UPDATE_JOBSTAMP,            /*!< Signal LRT that an update of job stamp is pending */
        UNDEFINED,                      /*!< Undefined type of notification */
        First = LRT_END_ITERATION,      /*!< Sentry for EnumIterator::begin */
        Last = UNDEFINED,               /*!< Sentry for EnumIterator::end */
    };

    inline const char *notificationToString(NotificationType type) {
        switch (type) {
            case NotificationType::LRT_END_ITERATION:
                return "LRT_END_ITERATION";
            case NotificationType::LRT_START_ITERATION:
                return "LRT_START_ITERATION";
            case NotificationType::LRT_REPEAT_ITERATION_EN:
                return "LRT_REPEAT_ITERATION_EN";
            case NotificationType::LRT_REPEAT_ITERATION_DIS:
                return "LRT_REPEAT_ITERATION_DIS";
            case NotificationType::LRT_FINISHED_ITERATION:
                return "LRT_FINISHED_ITERATION";
            case NotificationType::LRT_RST_ITERATION:
                return "LRT_RST_ITERATION";
            case NotificationType::LRT_CLEAR_ITERATION:
                return "LRT_CLEAR_ITERATION";
            case NotificationType::LRT_STOP:
                return "LRT_STOP";
            case NotificationType::LRT_PAUSE:
                return "LRT_PAUSE";
            case NotificationType::LRT_RESUME:
                return "LRT_RESUME";
            case NotificationType::TRACE_ENABLE:
                return "TRACE_ENABLE";
            case NotificationType::TRACE_DISABLE:
                return "TRACE_DISABLE";
            case NotificationType::TRACE_TASK:
                return "TRACE_TASK";
            case NotificationType::TRACE_SCHEDULE:
                return "TRACE_SCHEDULE";
            case NotificationType::TRACE_TRANSFO:
                return "TRACE_TRANSFO";
            case NotificationType::TRACE_PARAM:
                return "TRACE_PARAM";
            case NotificationType::TRACE_MEMORY:
                return "TRACE_MEMORY";
            case NotificationType::JOB_ADD:
                return "JOB_ADD";
            case NotificationType::JOB_CLEAR_QUEUE:
                return "JOB_CLEAR_QUEUE";
            case NotificationType::JOB_SENT_PARAM:
                return "JOB_SENT_PARAM";
            case NotificationType::JOB_BROADCAST_JOBSTAMP:
                return "JOB_BROADCAST_JOBSTAMP";
            case NotificationType::JOB_DELAY_BROADCAST_JOBSTAMP:
                return "JOB_DELAY_BROADCAST_JOBSTAMP";
            case NotificationType::JOB_UPDATE_JOBSTAMP:
                return "JOB_UPDATE_JOBSTAMP";
            case NotificationType::UNDEFINED:
                return "UNDEFINED";
            default:
                return "DEFAULT";
        }
    }

    /* === Structure(s) definition === */

    struct Notification {

        Notification() = default;

        Notification(const Notification &) = default;

        Notification(Notification &&) noexcept = default;

        explicit Notification(NotificationType type,
                              size_t senderIx = SIZE_MAX,
                              size_t notificationIx = SIZE_MAX) : type_{ type },
                                                                  senderIx_{ senderIx },
                                                                  notificationIx_{ notificationIx } {

        }

        Notification &operator=(const Notification &) = default;

        Notification &operator=(Notification &&) noexcept = default;

        ~Notification() = default;

        /* === Struct member(s) === */

        NotificationType type_ = NotificationType::UNDEFINED; /*!< Primary type of the notification (ex: NotificationType::JOB). */
        size_t senderIx_ = SIZE_MAX;                          /*!< ID of the sender of the notification */
        size_t notificationIx_ = SIZE_MAX;                    /*!< Index of the notification to fetch (may be used for direct value passing for some notification). */
    };
}

#endif //SPIDER2_NOTIFICATION_H
