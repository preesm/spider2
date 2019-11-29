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
#include <spider-api/archi.h>
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

        RTRunner(PE *pe, size_t ix) : runningPE_{ pe }, runnerIx_{ ix } {
            auto *platform = spider::platform();
            localJobStampsArray_ = spider::array<uint32_t>{ platform->LRTCount(), UINT32_MAX, StackID::RUNTIME };
            jobQueue_ = spider::containers::vector<JobMessage>(StackID::RUNTIME);
        }

        virtual ~RTRunner() = default;

        /* === Method(s) === */

        virtual void run(bool infiniteLoop) = 0;

        /* === Getter(s) === */

        inline size_t ix() const {
            return runnerIx_;
        }

        inline PE *pe() const {
            return runningPE_;
        }

        /* === Setter(s) === */

    protected:
        spider::vector<JobMessage> jobQueue_{ spider::Allocator<JobMessage>(StackID::RUNTIME) };
        spider::array<uint32_t> localJobStampsArray_;
        PE *runningPE_ = nullptr;
        size_t runnerIx_ = SIZE_MAX;
        size_t jobQueueCurrentPos_ = 0;
        bool stop_ = false;

        inline void clearLocalJobStamps() {
            jobQueueCurrentPos_ = 0;
            jobQueue_.clear();
        }

        inline void broadcastJobStamps() {
            spider::Notification broadcastNotification{ spider::NotificationType::JOB,
                                                        spider::JobNotification::UPDATE_JOBSTAMP,
                                                        static_cast<int32_t >(ix()),
                                                        static_cast<int32_t>(jobQueueCurrentPos_) };
            for (size_t i = 0; i < spider::platform()->LRTCount(); ++i) {
                if (i != ix()) {
                    spider::rtPlatform()->communicator()->push(broadcastNotification, i);
                }
            }
        }
    };
}

#endif //SPIDER2_RTRUNNER_H