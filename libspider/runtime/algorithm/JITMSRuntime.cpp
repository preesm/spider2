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

#ifndef _NO_BUILD_LEGACY_RT

#include <runtime/algorithm/JITMSRuntime.h>
#include <runtime/runner/RTRunner.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/communicator/RTCommunicator.h>
#include <graphs-tools/transformation/srdag/Transformation.h>
#include <graphs-tools/transformation/optims/optimizations.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <scheduling/ResourcesAllocator.h>
#include <scheduling/memory/FifoAllocator.h>
#include <scheduling/schedule/exporter/SchedXMLGanttExporter.h>
#include <scheduling/schedule/exporter/SchedStatsExporter.h>
#include <scheduling/task/VertexTask.h>
#include <api/runtime-api.h>
#include <api/config-api.h>
#include <api/spider.h>

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::JITMSRuntime::JITMSRuntime(pisdf::Graph *graph, const RuntimeConfig &cfg) :
        Runtime(graph),
        srdag_{ make_unique<srdag::Graph, StackID::RUNTIME>(graph) },
        resourcesAllocator_{ make_unique<sched::ResourcesAllocator, StackID::RUNTIME>(cfg.schedPolicy_,
                                                                                      cfg.mapPolicy_,
                                                                                      cfg.execPolicy_,
                                                                                      cfg.allocType_,
                                                                                      true) } {
    if (!rt::platform()) {
        throwSpiderException("JITMSRuntime need the runtime platform to be created.");
    }
    resourcesAllocator_->allocator()->allocatePersistentDelays(graph_);
    pisdf::recursiveSplitDynamicGraph(graph);
}

bool spider::JITMSRuntime::execute() {
    const auto grtIx = archi::platform()->spiderGRTPE()->attachedLRT()->virtualIx();
    /* == Time point used as reference == */
    if (api::exportTraceEnabled()) {
        startIterStamp_ = time::now();
    }

    /* == Apply first transformation of root graph == */
    TraceMessage transfoMsg{ };
    TRACE_TRANSFO_START()
    auto rootJob = srdag::TransfoJob(graph_);
    rootJob.params_ = graph_->params();
    auto resultRootJob = srdag::singleRateTransformation(rootJob, srdag_.get());

    /* == Initialize the job stacks == */
    auto staticJobStack = factory::vector<srdag::TransfoJob>(StackID::TRANSFO);
    auto dynamicJobStack = factory::vector<srdag::TransfoJob>(StackID::TRANSFO);
    updateJobStack(resultRootJob.first, staticJobStack);
    updateJobStack(resultRootJob.second, dynamicJobStack);
    TRACE_TRANSFO_END()

    /* == Transform, schedule and run == */
    while (!staticJobStack.empty() || !dynamicJobStack.empty()) {
        /* == Transform static jobs == */
        TRACE_TRANSFO_START()
        transformStaticJobs(staticJobStack, dynamicJobStack);
        TRACE_TRANSFO_END()

        /* == Apply graph optimizations == */
        if (api::shouldOptimizeSRDAG()) {
            TRACE_TRANSFO_START()
            optims::optimize(srdag_.get());
            TRACE_TRANSFO_END()
        }

        /* == Update schedule, run and wait == */
        scheduleRunAndWait();

        /* == Wait for all parameters to be resolved == */
        if (!dynamicJobStack.empty()) {
            if (log::enabled<log::TRANSFO>()) {
                log::info<log::TRANSFO>("Waiting fo dynamic parameters..\n");
            }
            TRACE_TRANSFO_START()
            size_t readParam = 0;
            while (readParam != dynamicJobStack.size()) {
                Notification notification;
                rt::platform()->communicator()->popParamNotification(notification);
                if (notification.type_ == NotificationType::JOB_SENT_PARAM) {
                    /* == Get the message == */
                    ParameterMessage message;
                    rt::platform()->communicator()->pop(message, grtIx, notification.notificationIx_);

                    /* == Get the config vertex == */
                    const auto *task = resourcesAllocator_->schedule()->task(message.taskIx_);
                    const auto *vertexTask = static_cast<const sched::VertexTask *>(task);
                    const auto *cfg = vertexTask->vertex();
                    auto paramIterator = message.params_.begin();
                    for (const auto &param : cfg->outputParamVector()) {
                        param->setValue((*(paramIterator++)));
                        if (log::enabled<log::TRANSFO>()) {
                            log::info<log::TRANSFO>("Parameter [%12s]: received value #%" PRId64".\n",
                                                    param->name().c_str(),
                                                    param->value());
                        }
                    }
                    readParam++;
                } else {
                    // LCOV_IGNORE: this is a sanity check, it should never happen and it is not testable from the outside.
                    throwSpiderException("expected parameter notification");
                }
                TRACE_TRANSFO_END()
            }

            /* == Transform dynamic jobs == */
            TRACE_TRANSFO_START()
            transformDynamicJobs(staticJobStack, dynamicJobStack);
            TRACE_TRANSFO_END()

            /* == Apply graph optimizations == */
            if (api::shouldOptimizeSRDAG()) {
                TRACE_TRANSFO_START()
                optims::optimize(srdag_.get());
                TRACE_TRANSFO_END()
            }

            /* == Update schedule, run and wait == */
            scheduleRunAndWait();
        }
    }

    /* == Export srdag if needed  == */
    if (api::exportSRDAGEnabled()) {
        Runtime::exportSRDAG(srdag_.get(), "./srdag.dot");
    }

    /* == Runners should clear their parameters == */
    rt::platform()->sendClearToRunners();

    /* == Export post-exec gantt if needed  == */
    if (api::exportTraceEnabled()) {
        useExecutionTraces(resourcesAllocator_->schedule(), startIterStamp_);
    }

    /* == Clear the srdag == */
    srdag_->clear();

    /* == Clear the resource allocator == */
    resourcesAllocator_->clear();
    return true;
}

/* === Private method(s) === */

void spider::JITMSRuntime::scheduleRunAndWait() {
    TraceMessage schedMsg{ };
    TRACE_SCHEDULE_START()
    /* == Send LRT_START_ITERATION notification == */
    rt::platform()->sendStartIteration();
    /* == Schedule / Map current Single-Rate graph == */
    resourcesAllocator_->execute(srdag_.get());
    /* == Send JOB_DELAY_BROADCAST_JOBSTAMP notification == */
    rt::platform()->sendDelayedBroadCastToRunners();
    /* == Send LRT_END_ITERATION notification == */
    rt::platform()->sendEndIteration();
    TRACE_SCHEDULE_END()

    /* == Export pre-exec gantt if needed  == */
    if (api::exportGanttEnabled()) {
        exportPreExecGantt(resourcesAllocator_->schedule());
    }

    /* == If there are jobs left, run == */
    rt::platform()->runner(archi::platform()->getGRTIx())->run(false);
    rt::platform()->waitForRunnersToFinish();
}

/* === Transformation related methods === */

void spider::JITMSRuntime::updateJobStack(vector<srdag::TransfoJob> &src, vector<srdag::TransfoJob> &dest) {
    std::for_each(src.begin(), src.end(), [&dest](srdag::TransfoJob &job) {
        dest.emplace_back(std::move(job));
    });
}

void spider::JITMSRuntime::transformJobs(vector<srdag::TransfoJob> &iterJobStack,
                                         vector<srdag::TransfoJob> &staticJobStack,
                                         vector<srdag::TransfoJob> &dynamicJobStack) {
    for (auto &job : iterJobStack) {
        /* == Transform current job == */
        auto result = srdag::singleRateTransformation(job, srdag_.get());

        /* == Move static TransfoJob into static JobStack == */
        updateJobStack(result.first, staticJobStack);

        /* == Move dynamic TransfoJob into dynamic JobStack == */
        updateJobStack(result.second, dynamicJobStack);
    }
}

void spider::JITMSRuntime::transformStaticJobs(vector<srdag::TransfoJob> &staticJobStack,
                                               vector<srdag::TransfoJob> &dynamicJobStack) {
    auto tempJobStack = factory::vector<srdag::TransfoJob>(StackID::TRANSFO);
    while (!staticJobStack.empty()) {
        /* == Transform jobs of current static stack == */
        transformJobs(staticJobStack, tempJobStack, dynamicJobStack);
        /* == Swap vectors == */
        staticJobStack.swap(tempJobStack);
        tempJobStack.clear();
    }
}

void spider::JITMSRuntime::transformDynamicJobs(vector<srdag::TransfoJob> &staticJobStack,
                                                vector<srdag::TransfoJob> &dynamicJobStack) {
    auto tempJobStack = factory::vector<srdag::TransfoJob>(StackID::TRANSFO);
    /* == Transform jobs of current dynamic stack == */
    transformJobs(dynamicJobStack, staticJobStack, tempJobStack);
    /* == Swap vectors == */
    dynamicJobStack.swap(tempJobStack);
}

#endif