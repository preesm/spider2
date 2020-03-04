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

#include <runtime/algorithm/FastJITMSRuntime.h>
#include <runtime/runner/RTRunner.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/interface/RTCommunicator.h>
#include <api/runtime-api.h>
#include <scheduling/scheduler/BestFitScheduler.h>
#include <scheduling/allocator/DefaultFifoAllocator.h>
#include <graphs-tools/helper/pisdf.h>

/* === Static function === */

static spider::FifoAllocator *makeFifoAllocator(spider::FifoAllocatorType type) {
    switch (type) {
        case spider::FifoAllocatorType::DEFAULT:
            return spider::make<spider::DefaultFifoAllocator, StackID::RUNTIME>();
        case spider::FifoAllocatorType::ARCHI_AWARE:
            break;
        default:
            throwSpiderException("unsupported type of FifoAllocator.");
    }
    return nullptr;
}

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::FastJITMSRuntime::FastJITMSRuntime(pisdf::Graph *graph,
                                           SchedulingAlgorithm schedulingAlgorithm,
                                           FifoAllocatorType type) :
        Runtime(graph),
        scheduler_{ makeScheduler(schedulingAlgorithm, graph) },
        fifoAllocator_{ makeFifoAllocator(type) } {
    scheduler_->setAllocator(fifoAllocator_.get());
    isFullyStatic_ = pisdf::isGraphFullyStatic(graph);
}

bool spider::FastJITMSRuntime::execute() {
    if (isFullyStatic_) {
        return staticExecute();
    }
    return dynamicExecute();
}

/* === Private method(s) implementation === */

bool spider::FastJITMSRuntime::staticExecute() {
    const auto grtIx = archi::platform()->spiderGRTPE()->attachedLRT()->virtualIx();
    static bool first = true;
    if (!first) {
        /* == Send LRT_START_ITERATION notification == */
        rt::platform()->sendStartIteration();

        /* == Just reset the schedule and re-run it == */
        scheduler_->schedule().sendReadyTasks();

        /* == Send LRT_END_ITERATION notification == */
        rt::platform()->sendEndIteration();

        /* == Run and wait == */
        rt::platform()->runner(grtIx)->run(false);
        rt::platform()->waitForRunnersToFinish();
        rt::platform()->sendClearToRunners();
        scheduler_->schedule().reset();
        return true;
    }
    first = false;
    return true;
}

bool spider::FastJITMSRuntime::dynamicExecute() {
    return true;
}
