/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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

#include <runtime/algorithm/StaticRuntime.h>
#include <runtime/runner/RTRunner.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/interface/RTCommunicator.h>
#include <api/runtime-api.h>
#include <graphs-tools/transformation/srdag/Transformation.h>
#include <graphs-tools/transformation/optims/optimizations.h>
#include <scheduling/scheduler/BestFitScheduler.h>
#include <scheduling/allocator/DefaultFifoAllocator.h>
#include <monitor/Monitor.h>
#include <api/config-api.h>
#include <scheduling/schedule/exporter/SchedXMLGanttExporter.h>
#include <scheduling/schedule/exporter/SchedStatsExporter.h>
#include <scheduling/schedule/exporter/SchedSVGGanttExporter.h>
#include <graphs-tools/helper/pisdf-helper.h>

/* === Static function === */

static void
updateJobStack(spider::vector<spider::srdag::TransfoJob> &src, spider::vector<spider::srdag::TransfoJob> &dest) {
    std::for_each(src.begin(), src.end(), [&dest](spider::srdag::TransfoJob &job) {
        dest.emplace_back(std::move(job));
    });
}

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::StaticRuntime::StaticRuntime(pisdf::Graph *graph,
                                     SchedulingPolicy schedulingAlgorithm,
                                     FifoAllocatorType type) :
        Runtime(graph),
        srdag_{ make_unique<pisdf::Graph, StackID::RUNTIME>("srdag-" + graph->name()) },
        scheduler_{ makeScheduler(schedulingAlgorithm, srdag_.get()) },
        fifoAllocator_{ makeFifoAllocator(type) } {
    scheduler_->setAllocator(fifoAllocator_.get());
    if (!rt::platform()) {
        throwSpiderException("JITMSRuntime need the runtime platform to be created.");
    }
    fifoAllocator_->allocatePersistentDelays(graph_);
}

bool spider::StaticRuntime::execute() {
    /* == Time point used as reference == */
    if (api::exportTraceEnabled()) {
        startIterStamp_ = time::now();
    }
    if (iter_) {
        run();
    } else {
        applyTransformationAndRun();
    }
    iter_++;
    return true;
}

/* === Private method(s) === */

void spider::StaticRuntime::applyTransformationAndRun() {
    /* == Runners should repeat their iteration == */
    rt::platform()->sendRepeatToRunners(true);
    TraceMessage transfoMsg{ };
    TRACE_TRANSFO_START();
    /* == Apply first transformation of root graph == */
    auto rootJob = srdag::TransfoJob(graph_);
    rootJob.params_ = graph_->params();
    auto resultRootJob = srdag::singleRateTransformation(rootJob, srdag_.get());
    /* == Initialize the job stacks == */
    auto staticJobStack = factory::vector<srdag::TransfoJob>(StackID::TRANSFO);
    updateJobStack(resultRootJob.first, staticJobStack);
    auto tempJobStack = factory::vector<srdag::TransfoJob>(StackID::TRANSFO);
    while (!staticJobStack.empty()) {
        for (auto &job : staticJobStack) {
            /* == Transform static graphs == */
            auto &&result = srdag::singleRateTransformation(job, srdag_.get());
            /* == Move static TransfoJob into static JobStack == */
            updateJobStack(result.first, tempJobStack);
        }

        /* == Swap vectors == */
        staticJobStack.swap(tempJobStack);
        tempJobStack.clear();
    }
    TRACE_TRANSFO_END();

    /* == Apply graph optimizations == */
    if (api::shouldOptimizeSRDAG()) {
        TRACE_TRANSFO_START();
        optims::optimize(srdag_.get());
        TRACE_TRANSFO_END();
    }

    /* == Export srdag if needed  == */
    if (api::exportSRDAGEnabled()) {
        api::exportGraphToDOT(srdag_.get(), "./srdag.dot");
    }

    /* == Update schedule, run and wait == */
    TraceMessage schedMsg{ };
    TRACE_SCHEDULE_START();
    /* == Send LRT_START_ITERATION notification == */
    rt::platform()->sendStartIteration();
    /* == Schedule / Map current Single-Rate graph == */
    scheduler_->update();
    scheduler_->execute();

    /* == Send LRT_END_ITERATION notification == */
    rt::platform()->sendEndIteration();
    TRACE_SCHEDULE_END();

    /* == Export pre-exec gantt if needed  == */
    if (api::exportGanttEnabled()) {
        exportPreExecGantt(&scheduler_->schedule());
    }

    /* == If there are jobs left, run == */
    rt::platform()->runner(archi::platform()->getGRTIx())->run(false);
    rt::platform()->waitForRunnersToFinish();

    /* == Runners should reset their parameters == */
    rt::platform()->sendResetToRunners();

    /* == Export post-exec gantt if needed  == */
    if (api::exportTraceEnabled()) {
        useExecutionTraces(srdag_.get(), &scheduler_->schedule(), startIterStamp_);
    }
}

void spider::StaticRuntime::run() {
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
    /* == Check if we need to re-schedule == */
    if (api::exportTraceEnabled()) {
        useExecutionTraces(srdag_.get(), &scheduler_->schedule(), startIterStamp_);
    }
}