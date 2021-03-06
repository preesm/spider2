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

#include <runtime/algorithm/pisdf-based/PiSDFJITMSRuntime.h>
#include <graphs/pisdf/Graph.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>
#include <scheduling/ResourcesAllocator.h>
#include <scheduling/memory/FifoAllocator.h>
#include <runtime/runner/RTRunner.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/communicator/RTCommunicator.h>
#include <api/runtime-api.h>
#include <api/config-api.h>
#include <api/spider.h>
#include <graphs-tools/helper/pisdf-helper.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::PiSDFJITMSRuntime::PiSDFJITMSRuntime(pisdf::Graph *graph, const RuntimeConfig &cfg, bool isStatic) :
        Runtime(graph),
        resourcesAllocator_{ make_unique<sched::ResourcesAllocator, StackID::RUNTIME>(cfg.schedPolicy_,
                                                                                      cfg.mapPolicy_,
                                                                                      cfg.execPolicy_,
                                                                                      cfg.allocType_,
                                                                                      false) },
        isStatic_{ isStatic } {
    if (!rt::platform()) {
        throwSpiderException("JITMSRuntime need the runtime platform to be created.");
    }
    resourcesAllocator_->allocator()->allocatePersistentDelays(graph_);
    pisdf::recursiveSplitDynamicGraph(graph_);
    graphHandler_ = make_unique<pisdf::GraphHandler, StackID::TRANSFO>(graph_, graph_->params(), 1u);
}

bool spider::PiSDFJITMSRuntime::execute() {
    if (isStatic_) {
        return staticExecute();
    }
    return dynamicExecute();
}

/* === Private method(s) implementation === */

bool spider::PiSDFJITMSRuntime::staticExecute() {
    /* == Time point used as reference == */
    if (api::exportTraceEnabled()) {
        startIterStamp_ = time::now();
    }
    if (iter_) {
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
            fprintf(stderr, "static applications are not monitored beyond first iteration.\n");
        }
    } else {
        /* == Runners should repeat their iteration == */
        rt::platform()->sendRepeatToRunners(true);
        TraceMessage schedMsg{ };
        TRACE_SCHEDULE_START();
        /* == Send LRT_START_ITERATION notification == */
        rt::platform()->sendStartIteration();
        resourcesAllocator_->execute(graphHandler_.get());
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
        rt::platform()->sendResetToRunners();
        if (api::exportTraceEnabled()) {
            useExecutionTraces(resourcesAllocator_->schedule(), startIterStamp_);
        }
    }
    resourcesAllocator_->clear();
    graphHandler_->clear();
    iter_++;
    return true;
}

bool spider::PiSDFJITMSRuntime::dynamicExecute() {
    /* == Time point used as reference == */
    if (api::exportTraceEnabled()) {
        startIterStamp_ = time::now();
    }
    /* == Resolve, schedule and run == */
    const auto grtIx = archi::platform()->getGRTIx();
    auto done = false;
    while (!done) {
        TraceMessage schedMsg{ };
        TRACE_SCHEDULE_START();
        /* == Send LRT_START_ITERATION notification == */
        rt::platform()->sendStartIteration();
        resourcesAllocator_->execute(graphHandler_.get());
        /* == Send JOB_DELAY_BROADCAST_JOBSTAMP notification == */
        rt::platform()->sendDelayedBroadCastToRunners();
        /* == Send LRT_END_ITERATION notification == */
        rt::platform()->sendEndIteration();
        TRACE_SCHEDULE_END();
        /* == Export pre-exec gantt if needed  == */
        if (api::exportGanttEnabled()) {
            exportPreExecGantt(resourcesAllocator_->schedule());
        }
        /* == If there are jobs left, run == */
        rt::platform()->runner(grtIx)->run(false);
        rt::platform()->waitForRunnersToFinish();

        /* == Wait for all parameters to be resolved == */
        const auto expectedParamCount = countExpectedNumberOfParams(graphHandler_.get());
        if (!expectedParamCount) {
            done = true;
        } else {
            if (log::enabled<log::TRANSFO>()) {
                log::info<log::TRANSFO>("Waiting fo dynamic parameters..\n");
            }
            TraceMessage transfoMsg{ };
            TRACE_TRANSFO_START();
            const auto *schedule = resourcesAllocator_->schedule();
            size_t readParam = 0;
            while (readParam != expectedParamCount) {
                Notification notification;
                rt::platform()->communicator()->popParamNotification(notification);
                if (notification.type_ == NotificationType::JOB_SENT_PARAM) {
                    /* == Get the message == */
                    ParameterMessage message;
                    rt::platform()->communicator()->pop(message, grtIx, notification.notificationIx_);
                    /* == Get the config vertex == */
                    auto *task = schedule->task(message.taskIx_);
                    task->receiveParams(message.params_);
                    readParam++;
                } else {
                    // LCOV_IGNORE: this is a sanity check, it should never happen and it is not testable from the outside.
                    throwSpiderException("expected parameter notification");
                }
            }
            TRACE_TRANSFO_END();
        }
    }

    /* == Runners should clear their parameters == */
    rt::platform()->sendClearToRunners();

    /* == Export post-exec gantt if needed  == */
    if (api::exportTraceEnabled()) {
        useExecutionTraces(resourcesAllocator_->schedule(), startIterStamp_);
    }
    /* == Clear the resources == */
    resourcesAllocator_->clear();
    graphHandler_->clear();
    return true;
}

size_t spider::PiSDFJITMSRuntime::countExpectedNumberOfParams(const pisdf::GraphHandler *graphHandler) const {
    size_t count = 0;
    for (const auto *firingHandler : graphHandler->firings()) {
        if (firingHandler->isResolved()) {
            for (const auto *subHandler : firingHandler->subgraphFirings()) {
                count += countExpectedNumberOfParams(subHandler);
            }
        } else {
            const auto &params = graphHandler->base()->getParams();
            count += static_cast<size_t> (std::count_if(std::begin(params), std::end(params),
                                                        [](const std::shared_ptr<pisdf::Param> &param) {
                                                            return param->type() == pisdf::ParamType::DYNAMIC;
                                                        }));
        }
    }
    return count;
}
