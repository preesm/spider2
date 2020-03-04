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
#ifndef SPIDER2_THREADRTCOMMUNICATOR_H
#define SPIDER2_THREADRTCOMMUNICATOR_H

/* === Include(s) === */

#include <runtime/interface/RTCommunicator.h>
#include <thread/Queue.h>
#include <thread/IndexedQueue.h>
#include <containers/array.h>
#include <containers/vector.h>

namespace spider {

    /* === Class definition === */

    class ThreadRTCommunicator final : public RTCommunicator {
    public:
        explicit ThreadRTCommunicator(size_t lrtCount);

        ~ThreadRTCommunicator() override = default;

        /* === Method(s) === */

        void push(Notification notification, size_t receiver) override;

        bool pop(Notification &notification, size_t receiver) override;

        bool try_pop(Notification &notification, size_t receiver) override;

        void pushParamNotification(size_t sender, size_t messageIndex) override;

        bool popParamNotification(Notification &notification) override;

        size_t push(JobMessage message, size_t receiver) override;

        bool pop(JobMessage &message, size_t receiver, size_t ix) override;

        size_t push(ParameterMessage message, size_t receiver) override;

        bool pop(ParameterMessage &message, size_t receiver, size_t ix) override;

        size_t push(TraceMessage message, size_t receiver) override;

        bool pop(TraceMessage &message, size_t receiver, size_t ix) override;

    private:
        vector<spider::Queue<Notification>> notificationQueueVector_;
        IndexedQueue<JobMessage, StackID::RUNTIME> jobMessageQueueArray_;
        IndexedQueue<ParameterMessage, StackID::RUNTIME> paramMessageQueueArray_;
        IndexedQueue<TraceMessage, StackID::RUNTIME> traceMessageQueueArray_;
    };

    /* === Inline method(s) === */

}

#endif //SPIDER2_THREADRTCOMMUNICATOR_H
