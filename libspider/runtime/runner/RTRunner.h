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
#ifndef SPIDER2_RTRUNNER_H
#define SPIDER2_RTRUNNER_H

/* === Include(s) === */

#include <cstdint>
#include <cstddef>
#include <containers/containers.h>
#include <thread/Thread.h>
#include <containers/array.h>
#include <api/runtime-api.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/interface/RTCommunicator.h>
#include <runtime/interface/Message.h>
#include <runtime/interface/Notification.h>

namespace spider {

    /* === Forward declaration(s) === */

    class PE;

    /* === Class definition === */

    class RTRunner {
    public:

        RTRunner(PE *attachedPE, size_t runnerIx, int32_t affinity = -1) : attachedPE_{ attachedPE },
                                                                           runnerIx_{ runnerIx },
                                                                           affinity_{ affinity } {
            auto *platform = archi::platform();
            localJobStampsArray_ = spider::array<size_t>{ platform->LRTCount(), SIZE_MAX, StackID::RUNTIME };
        }

        virtual ~RTRunner() = default;

        /* === Method(s) === */

        virtual void run(bool infiniteLoop) = 0;

        inline static void start(RTRunner *runner) {
            if (!runner) {
                throwSpiderException("nullptr runner.");
            }
            runner->begin();
        }

        virtual void begin() = 0;

        /* === Getter(s) === */

        inline size_t ix() const {
            return runnerIx_;
        }

        inline PE *attachedProcessingElement() const {
            return attachedPE_;
        }

    protected:
        stack_vector(jobQueue_, JobMessage, StackID::RUNTIME);
        spider::array<size_t> localJobStampsArray_;
        PE *attachedPE_ = nullptr;
        size_t runnerIx_ = SIZE_MAX;
        int32_t affinity_ = -1;
        size_t jobQueueCurrentPos_ = 0;
        bool stop_ = false;

        inline void clearLocalJobStamps() {
            jobQueueCurrentPos_ = 0;
            jobQueue_.clear();
        }

        inline void broadcastJobStamps() {
            Notification broadcastNotification{ NotificationType::JOB_UPDATE_JOBSTAMP,
                                                ix(),
                                                jobQueueCurrentPos_ };
            for (size_t i = 0; i < archi::platform()->LRTCount(); ++i) {
                if (i != ix()) {
                    rt::platform()->communicator()->push(broadcastNotification, i);
                }
            }
        }
    };
}

#endif //SPIDER2_RTRUNNER_H
