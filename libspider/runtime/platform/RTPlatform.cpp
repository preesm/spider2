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

#include <runtime/platform/RTPlatform.h>
#include <runtime/interface/RTCommunicator.h>
#include <runtime/runner/RTRunner.h>
#include <archi/PE.h>
#include <archi/Platform.h>

/* === Define(s) === */

#define LOG_WAIT() \
    if (log::enabled<log::LRT>()) {\
        log::info<log::LRT>("GRT -> waiting for runner #%zu to finish..\n", i);\
    }

#define LOG_REGISTER() \
    if (log::enabled<log::LRT>()) {\
        log::info<log::LRT>("GRT -> registering runner #%zu as finished.\n", ix);\
    }

/* === Global stop boolean === */

bool spider2StopRunning = false;

/* === Function(s) definition === */

spider::RTPlatform::RTPlatform(size_t runnerCount) : runtimeKernelVector_{
        factory::vector < unique_ptr < RTKernel >> (StackID::RUNTIME) },
                                                     runnerArray_{ runnerCount, nullptr, StackID::RUNTIME },
                                                     finishedRunnerArray_{ runnerCount, false, StackID::RUNTIME } {

}

spider::RTPlatform::~RTPlatform() {
    for (auto &runner : runnerArray_) {
        destroy(runner);
    }
}

void spider::RTPlatform::addRunner(RTRunner *runner) {
    if (!runner) {
        return;
    }
    runnerArray_.at(runner->ix()) = runner;

    /* == Create the ressource to handle the runner (thread, process, etc.) == */
    this->createRunnerRessource(runner);
}

size_t spider::RTPlatform::addKernel(RTKernel *kernel) {
    if (!kernel) {
        return SIZE_MAX;
    }
    /* == Search if Kernel does not already exists == */
    auto res = std::find_if(runtimeKernelVector_.begin(), runtimeKernelVector_.end(),
                            [&kernel](const unique_ptr<RTKernel> &p) { return p.get() == kernel; });
    if (res == runtimeKernelVector_.end()) {
        kernel->setIx(runtimeKernelVector_.size());
        runtimeKernelVector_.emplace_back(kernel);
        res = runtimeKernelVector_.end() - 1;
    }
    return (*res)->ix();
}

void spider::RTPlatform::sendStartIteration() const {
    Notification startIterNotification{ NotificationType::LRT_START_ITERATION,
                                        archi::platform()->spiderGRTPE()->attachedLRT()->virtualIx() };
    for (size_t i = 0; i < archi::platform()->LRTCount(); ++i) {
        communicator()->push(startIterNotification, i);
    }
}

void spider::RTPlatform::sendEndIteration() const {
    Notification endIterNotification{ NotificationType::LRT_END_ITERATION,
                                      archi::platform()->spiderGRTPE()->attachedLRT()->virtualIx() };
    for (size_t i = 0; i < archi::platform()->LRTCount(); ++i) {
        communicator()->push(endIterNotification, i);
    }
}

void spider::RTPlatform::sendDelayedBroadCastToRunners() const {
    for (size_t i = 0; i < archi::platform()->LRTCount(); ++i) {
        communicator()->push(
                Notification{ NotificationType::JOB_DELAY_BROADCAST_JOBSTAMP, archi::platform()->getGRTIx() }, i);
    }
}

void spider::RTPlatform::sendClearToRunners() const {
    const auto grtIx = archi::platform()->getGRTIx();
    for (size_t i = 0; i < archi::platform()->LRTCount(); ++i) {
        communicator()->push(Notification{ NotificationType::LRT_CLEAR_ITERATION, grtIx }, i);
    }
}

void spider::RTPlatform::sendResetToRunners() const {
    const auto grtIx = archi::platform()->getGRTIx();
    for (size_t i = 0; i < archi::platform()->LRTCount(); ++i) {
        communicator()->push(Notification{ NotificationType::LRT_RST_ITERATION, grtIx }, i);
    }
}

void spider::RTPlatform::sendRepeatToRunners(bool value) const {
    NotificationType type = value ? NotificationType::LRT_REPEAT_ITERATION_EN
                                  : NotificationType::LRT_REPEAT_ITERATION_DIS;
    for (size_t i = 0; i < archi::platform()->LRTCount(); ++i) {
        communicator()->push(Notification{ type, archi::platform()->getGRTIx() }, i);
    }
}

void spider::RTPlatform::waitForRunnersToFinish() {
    const auto grtIx = archi::platform()->spiderGRTPE()->attachedLRT()->virtualIx();
    auto notifVector = factory::vector<Notification>(StackID::RUNTIME);
    /* == Wait for the notifications == */
    for (size_t i = 0; i < archi::platform()->LRTCount(); ++i) {
        while (!finishedRunnerArray_[i]) {
            LOG_WAIT();
            Notification notification;
            communicator()->pop(notification, grtIx);
            if (notification.type_ == NotificationType::LRT_FINISHED_ITERATION) {
                finishedRunnerArray_[i] = true;
            } else {
                notifVector.emplace_back(notification);
            }
        }
    }
    for (auto &notification : notifVector) {
        /* == push back notification == */
        communicator()->push(notification, grtIx);
    }
    /* == Reset values == */
    finishedRunnerArray_.assign(false);
}

void spider::RTPlatform::registerFinishedRunner(size_t ix) {
    LOG_REGISTER();
    finishedRunnerArray_.at(ix) = true;
}

spider::RTKernel *spider::RTPlatform::getKernel(size_t ix) const {
    if (ix >= runtimeKernelVector_.size()) {
        return nullptr;
    }
    return runtimeKernelVector_[ix].get();
}

void spider::RTPlatform::setCommunicator(RTCommunicator *communicator) {
    if (!communicator) {
        return;
    }
    if (communicator_) {
        throwSpiderException("already existing runtime communicator.");
    }
    communicator_ = make_unique(communicator);
}
