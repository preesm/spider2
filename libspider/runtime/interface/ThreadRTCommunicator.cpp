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

#include <runtime/interface/ThreadRTCommunicator.h>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::ThreadRTCommunicator::ThreadRTCommunicator(size_t lrtCount) :
        notificationQueueVector_{ factory::vector<spider::Queue<Notification>>(lrtCount, StackID::RUNTIME) } { }

void spider::ThreadRTCommunicator::push(Notification notification, size_t receiver) {
    notificationQueueVector_.at(receiver).push(notification);
}

bool spider::ThreadRTCommunicator::pop(Notification &notification, size_t receiver) {
    return notificationQueueVector_.at(receiver).pop(notification);
}

bool spider::ThreadRTCommunicator::try_pop(Notification &notification, size_t receiver) {
    return notificationQueueVector_[receiver].try_pop(notification);
}

void spider::ThreadRTCommunicator::pushParamNotification(size_t sender, size_t messageIndex) {
    paramNotificationQueue_.push(Notification(NotificationType::JOB_SENT_PARAM, sender, messageIndex));
}

bool spider::ThreadRTCommunicator::popParamNotification(Notification &notification) {
    return paramNotificationQueue_.pop(notification);
}

void spider::ThreadRTCommunicator::pushTraceNotification(Notification notification) {
    traceNotificationQueue_.push(notification);
}

bool spider::ThreadRTCommunicator::popTraceNotification(Notification &notification) {
    return traceNotificationQueue_.try_pop(notification);
}

size_t spider::ThreadRTCommunicator::push(JobMessage message, size_t) {
    return jobMessageQueueArray_.push(std::move(message));
}

bool spider::ThreadRTCommunicator::pop(JobMessage &message, size_t, size_t ix) {
    return jobMessageQueueArray_.pop(message, ix);
}

size_t spider::ThreadRTCommunicator::push(ParameterMessage message, size_t) {
    return paramMessageQueueArray_.push(std::move(message));
}

bool spider::ThreadRTCommunicator::pop(ParameterMessage &message, size_t, size_t ix) {
    return paramMessageQueueArray_.pop(message, ix);
}

size_t spider::ThreadRTCommunicator::push(TraceMessage message, size_t) {
    return traceMessageQueueArray_.push(message);
}

bool spider::ThreadRTCommunicator::pop(TraceMessage &message, size_t, size_t ix) {
    return traceMessageQueueArray_.pop(message, ix);
}

#if defined(__GNUC__) || defined(__MINGW64__) || defined(__MINGW32__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-function"
#endif

/**
 * @brief Trick to pre-declare template specialization.
 */
static void foo() {
    spider::Queue<spider::Notification> a;
    spider::IndexedQueue<spider::JobMessage> b;
    spider::IndexedQueue<spider::ParameterMessage> c;
    spider::IndexedQueue<spider::TraceMessage> d;
}

#if defined(__GNUC__) || defined(__MINGW64__) || defined(__MINGW32__)
#pragma GCC diagnostic pop
#elif defined(__clang__)
#pragma clang diagnostic pop
#endif
