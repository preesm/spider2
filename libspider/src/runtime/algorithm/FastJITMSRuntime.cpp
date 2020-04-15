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

#include <runtime/algorithm/FastJITMSRuntime.h>
#include <runtime/runner/RTRunner.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/interface/RTCommunicator.h>
#include <api/runtime-api.h>
#include <graphs-tools/transformation/srdagless/SRLessHandler.h>
#include <graphs-tools/transformation/srdag/Transformation.h>
#include <scheduling/scheduler/srdagless/SRLessScheduler.h>
#include <scheduling/allocator/DefaultFifoAllocator.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <api/config-api.h>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::FastJITMSRuntime::FastJITMSRuntime(pisdf::Graph *graph,
                                           SchedulingPolicy schedulingAlgorithm,
                                           FifoAllocatorType type) :
        Runtime(graph),
        scheduler_{ makeSRLessScheduler(graph, schedulingAlgorithm) },
        fifoAllocator_{ makeSRLessFifoAllocator(type) } {
    if (!scheduler_) {
        throwSpiderException("Failed to create scheduler.\n"
                             "Check compatibility between algorithm and runtime.");
    }
    scheduler_->setAllocator(fifoAllocator_.get());
    isFullyStatic_ = pisdf::isGraphFullyStatic(graph);
    if (!isFullyStatic_) {
        pisdf::recursiveSplitDynamicGraph(graph);
    }
}

bool spider::FastJITMSRuntime::execute() {
    if (isFullyStatic_) {
        return staticExecute();
    }
    return dynamicExecute();
}

/* === Private method(s) implementation === */

bool spider::FastJITMSRuntime::staticExecute() {
    handleStaticGraph(graph_);
    scheduler_->update();
    scheduler_->execute();
    exportPreExecGantt(&scheduler_->schedule(), "./sched.gantt");
    return true;
}

bool spider::FastJITMSRuntime::dynamicExecute() {
    return true;
}

void spider::FastJITMSRuntime::handleStaticGraph(pisdf::Graph *) {
    auto &handler = scheduler_->srLessHandler();
    /* == Compute BRV == */
    handler.resolveStatic();
//    handler_->flattenDependencies(graph, 0);
}
