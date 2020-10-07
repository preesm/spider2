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

#include <runtime/algorithm/FastRuntime.h>
#include <api/runtime-api.h>
#include <graphs/pisdf/Graph.h>
#include <graphs-tools/transformation/srless/GraphFiring.h>
#include <graphs-tools/transformation/srless/GraphHandler.h>
#include <scheduling/ResourcesAllocator.h>
#include <scheduling/memory/FifoAllocator.h>
#include <scheduling/scheduler/SRLessGreedyScheduler.h>
#include <scheduling/mapper/BestFitMapper.h>
#include <runtime/runner/RTRunner.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/communicator/RTCommunicator.h>
#include <api/config-api.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::FastRuntime::FastRuntime(pisdf::Graph *graph,
                                 SchedulingPolicy schedulingPolicy,
                                 MappingPolicy mappingPolicy,
                                 ExecutionPolicy executionPolicy,
                                 FifoAllocatorType allocatorType) :
        Runtime(graph),
        resourcesAllocator_{ make_unique<sched::ResourcesAllocator, StackID::RUNTIME>(schedulingPolicy,
                                                                                      mappingPolicy,
                                                                                      executionPolicy,
                                                                                      allocatorType,
                                                                                      false) } {
    if (!rt::platform()) {
        throwSpiderException("JITMSRuntime need the runtime platform to be created.");
    }
    resourcesAllocator_->allocator()->allocatePersistentDelays(graph_);
}

bool spider::FastRuntime::execute() {
    return staticExecute();
}

/* === Private method(s) implementation === */

bool spider::FastRuntime::staticExecute() {
    /* == Time point used as reference == */
    if (api::exportTraceEnabled()) {
        startIterStamp_ = time::now();
    }
    if (false) {
        const auto grtIx = archi::platform()->getGRTIx();
        TraceMessage schedMsg{ };
        TRACE_SCHEDULE_START();
        /* == Send LRT_START_ITERATION notification == */
        rt::platform()->sendStartIteration();
        /* == Send LRT_END_ITERATION notification == */
        rt::platform()->sendEndIteration();
        TRACE_SCHEDULE_END();
        /* == Run and wait == */
        rt::platform()->runner(grtIx)->run(false);
        rt::platform()->waitForRunnersToFinish();
        /* == Runners should reset their parameters == */
        rt::platform()->sendResetToRunners();
        if (api::exportTraceEnabled()) {
            useExecutionTraces(graph_, resourcesAllocator_->schedule(), startIterStamp_);
        }
    } else {
        /* == Runners should repeat their iteration == */
//        rt::platform()->sendRepeatToRunners(true);
        auto start = spider::time::now();
        auto graphHandler = srless::GraphHandler(graph_, graph_->params(), 1u);
        auto end = spider::time::now();
        auto duration = time::duration::nanoseconds(start, end);
        printer::fprintf(stderr, "ir-time:    %lld ns\n", duration);
        TraceMessage schedMsg{ };
        TRACE_SCHEDULE_START();
        /* == Send LRT_START_ITERATION notification == */
        rt::platform()->sendStartIteration();
        start = spider::time::now();
        resourcesAllocator_->execute(&graphHandler);
        end = spider::time::now();
        duration = time::duration::nanoseconds(start, end);
        printer::fprintf(stderr, "alloc-time: %lld ns\n", duration);
        /* == Send LRT_END_ITERATION notification == */
        rt::platform()->sendEndIteration();
        TRACE_SCHEDULE_END();
        /* == Export pre-exec gantt if needed  == */
        if (api::exportGanttEnabled()) {
            exportPreExecGantt(resourcesAllocator_->schedule());
        }
        /* == If there are jobs left, run == */
        rt::platform()->runner(archi::platform()->getGRTIx())->run(false);
        rt::platform()->waitForRunnersToFinish();
        /* == Runners should reset their parameters == */
//        rt::platform()->sendResetToRunners();
        rt::platform()->sendClearToRunners();
        if (api::exportTraceEnabled()) {
            useExecutionTraces(graph_, resourcesAllocator_->schedule(), startIterStamp_);
        }
    }
    resourcesAllocator_->clear();
    iter_++;
    return true;
}

bool spider::FastRuntime::dynamicExecute() {
    return true;
}

void spider::FastRuntime::handleStaticGraph(pisdf::Graph *) {
    /* == Compute BRV == */
}
