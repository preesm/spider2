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

#include <runtime/platform/ThreadRTPlatform.h>
#include <runtime/interface/RTCommunicator.h>
#include <runtime/runner/RTRunner.h>
#include <thread/Thread.h>
#include <archi/Platform.h>
#include <archi/PE.h>
#include <csignal>
#include <api/runtime-api.h>

extern bool spider2StopRunning;

/* === Function(s) definition === */

spider::ThreadRTPlatform::ThreadRTPlatform(size_t runnerCount) : RTPlatform(runnerCount),
                                                                 threadArray_{ runnerCount, nullptr,
                                                                               StackID::RUNTIME } {
    std::signal(SIGINT, [](int signal) {
        if (signal == SIGINT) {
            spider2StopRunning = true;
        }
    });
}

spider::ThreadRTPlatform::~ThreadRTPlatform() {
    /* == Send notification to exit to runners == */
    for (auto &runner : runnerArray_) {
        communicator_->push(Notification(NotificationType::LRT_STOP,
                                         archi::platform()->spiderGRTPE()->virtualIx()),
                            runner->ix());
    }

    /* == Wait for all the thread to finish == */
    for (auto &thread : threadArray_) {
        if (thread) {
            thread->join();
        }
    }

    /* == Destroy the threads == */
    for (auto &thread : threadArray_) {
        destroy(thread);
    }
}

void spider::ThreadRTPlatform::createRunnerRessource(spider::RTRunner *runner) {
    if (threadArray_.at(runner->ix())) {
        log::warning<log::LRT>("trying to create resource for runner #%zu more than once.\n",
                               runner->ix());
        return;
    }
    if (runner->attachedProcessingElement() != archi::platform()->spiderGRTPE()) {
        threadArray_.at(runner->ix()) = make<spider::thread, StackID::RUNTIME>(RTRunner::start, runner);
    } else {
        RTRunner::start(runner);
    }
}

void spider::ThreadRTPlatform::waitForRunnerToBeReady() {
    //sleep(2);
}
