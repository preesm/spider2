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
#ifndef SPIDER2_RTCOMMUNICATOR_H
#define SPIDER2_RTCOMMUNICATOR_H

/* === Include(s) === */

#include <runtime/interface/Message.h>
#include <runtime/interface/Notification.h>

namespace spider {

    /* === Class definition === */

    class RTCommunicator {
    public:
        RTCommunicator() = default;

        virtual ~RTCommunicator() = default;

        /* === Method(s) === */

        /**
         * @brief Push a notification for a given target LRT.
         * @param notification  Notification to push.
         * @param receiver      Receiver of the notification.
         */
        virtual void push(Notification notification, size_t receiver) = 0;

        /**
         * @brief Blocking pop to get notification.
         * @param notification  Notification structure to be filled.
         * @param receiver      Receiver of the notification.
         * @return true.
         */
        virtual bool pop(Notification &notification, size_t receiver) = 0;

        /**
         * @brief Non-blocking pop to get notification.
         * @param notification  Notification structure to be filled.
         * @param receiver      Receiver of the notification.
         * @return true if succeed in pop, false else.
         */
        virtual bool try_pop(Notification &notification, size_t receiver) = 0;

        /**
         * @brief Push a notification regarding new parameter value.
         * @param sender        Index of the sender.
         * @param messageIndex  Index of the  @refitem ParameterMessage.
         */
        virtual void pushParamNotification(size_t sender, size_t messageIndex) = 0;

        /**
         * @brief Blocking pop a notification regarding new parameter value.
         * @param notification  Notification structure to be filled.
         * @return true.
         */
        virtual bool popParamNotification(Notification &notification) = 0;

        /**
         * @brief Push a notification regarding new trace value.
         * @param notification  Notification to push.
         */
        virtual void pushTraceNotification(Notification notification) = 0;

        /**
         * @brief Blocking pop a notification regarding new trace value.
         * @param notification  Notification structure to be filled.
         * @return true.
         */
        virtual bool popTraceNotification(Notification &notification) = 0;

        /**
         * @brief Push a JobMessage for a given target LRT.
         * @param message  Message to push.
         * @param receiver Receiver of the notification.
         * @return Index of the pushed message in the queue.
         */
        virtual size_t push(JobMessage message, size_t receiver) = 0;

        /**
         * @brief Pop a JobMessage.
         * @param message  Message structure to be filled.
         * @param receiver Receiver of the message.
         * @param ix       Index of the message in the queue.
         * @return true on success, false else.
         */
        virtual bool pop(JobMessage &message, size_t receiver, size_t ix) = 0;

        /**
         * @brief Push a ParameterMessage for a given target LRT.
         * @param message  Message to push.
         * @param receiver Receiver of the notification.
         * @return Index of the pushed message in the queue.
         */
        virtual size_t push(ParameterMessage message, size_t receiver) = 0;

        /**
         * @brief Pop a ParameterMessage.
         * @param message  Message structure to be filled.
         * @param receiver Receiver of the message.
         * @param ix       Index of the message in the queue.
         * @return true on success, false else.
         */
        virtual bool pop(ParameterMessage &message, size_t receiver, size_t ix) = 0;

        /**
         * @brief Push a TraceMessage for a given target LRT.
         * @param message  Message to push.
         * @param receiver Receiver of the notification.
         * @return Index of the pushed message in the queue.
         */
        virtual size_t push(TraceMessage message, size_t receiver) = 0;

        /**
         * @brief Pop a TraceMessage.
         * @param message  Message structure to be filled.
         * @param receiver Receiver of the message.
         * @param ix       Index of the message in the queue.
         * @return true on success, false else.
         */
        virtual bool pop(TraceMessage &message, size_t receiver, size_t ix) = 0;
    };
}
#endif //SPIDER2_RTCOMMUNICATOR_H
