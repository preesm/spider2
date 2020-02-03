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

#include <runtime/algorithm/JITMSRuntime.h>
#include <graphs-tools/transformation/srdag/Transformation.h>
#include <graphs-tools/transformation/optims/optimizations.h>
#include <graphs-tools/exporter/PiSDFDOTExporter.h>
#include <scheduling/schedule/exporter/SchedSVGGanttExporter.h>
#include <scheduling/scheduler/BestFitScheduler.h>
#include <runtime/runner/RTRunner.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/interface/RTCommunicator.h>
#include <monitor/Monitor.h>
#include <api/runtime-api.h>
#include <api/config-api.h>
#include <common/Time.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <scheduling/schedule/exporter/SchedXMLGanttExporter.h>

/* === Static function(s) === */

static bool isGraphFullyStatic(const spider::pisdf::Graph *graph) {
    bool isFullyStatic = true;
    for (const auto &subgraph : graph->subgraphs()) {
        isFullyStatic &= isGraphFullyStatic(subgraph);
        if (!isFullyStatic) {
            break;
        }
    }
    return isFullyStatic;
}

/* === Method(s) implementation === */

spider::JITMSRuntime::JITMSRuntime(pisdf::Graph *graph, SchedulingAlgorithm schedulingAlgorithm) :
        Runtime(graph),
        srdag_{ make_unique<pisdf::Graph, StackID::RUNTIME>("srdag-" + graph->name()) },
        scheduler_{ makeScheduler(schedulingAlgorithm, srdag_.get()) } {
    isFullyStatic_ = isGraphFullyStatic(graph);
}

bool spider::JITMSRuntime::execute() {
    if (isFullyStatic_) {
        return staticExecute();
    }
    return dynamicExecute();
}

/* === Private method(s) === */

bool spider::JITMSRuntime::staticExecute() {
    static bool first = true;
    if (!first) {
        /* == Just reset the schedule and re-run it == */
        scheduler_->schedule().sendReadyJobs();
        rt::platform()->runner(0)->run(false);
        scheduler_->schedule().reset();
        return true;
    }
    first = false;
    /* == Apply first transformation of root graph == */
    auto &&rootJob = srdag::TransfoJob(graph_, SIZE_MAX, UINT32_MAX, true);
    rootJob.params_ = sbc::vector<std::shared_ptr<pisdf::Param>, StackID::TRANSFO>(graph_->params());
    auto &&resultRootJob = srdag::singleRateTransformation(rootJob, srdag_.get());

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

    /* == Apply graph optimizations == */
    if (api::optimizeSRDAG()) {
        //monitor_->startSampling();
        optims::optimize(srdag_.get());
        //monitor_->endSampling();
    }

    if (api::exportSRDAG()) {
        auto exporter = pisdf::PiSDFDOTExporter{ srdag_.get() };
        exporter.printFromPath("./srdag.dot");
    }

    /* == Schedule / Map current Single-Rate graph == */
    //monitor_->startSampling();
    scheduler_->update();
    scheduler_->mappingScheduling();
    //monitor_->endSampling();

    /* == If there are jobs left, run == */
    rt::platform()->runner(0)->run(false);

    /* == Reset the scheduler == */
    scheduler_->schedule().reset();

    return true;
}

bool spider::JITMSRuntime::dynamicExecute() {
    /* == Apply first transformation of root graph == */
    auto &&rootJob = srdag::TransfoJob(graph_, SIZE_MAX, UINT32_MAX, true);
    rootJob.params_ = sbc::vector<std::shared_ptr<pisdf::Param>, StackID::TRANSFO>(graph_->params());
    auto &&resultRootJob = srdag::singleRateTransformation(rootJob, srdag_.get());

    /* == Initialize the job stacks == */
    auto staticJobStack = factory::vector<srdag::TransfoJob>(StackID::TRANSFO);
    auto dynamicJobStack = factory::vector<srdag::TransfoJob>(StackID::TRANSFO);
    updateJobStack(resultRootJob.first, staticJobStack);
    updateJobStack(resultRootJob.second, dynamicJobStack);

    while (!staticJobStack.empty() || !dynamicJobStack.empty()) {
        /* == Transform static jobs == */
        //monitor_->startSampling();
        transformStaticJobs(staticJobStack, dynamicJobStack);
        //monitor_->endSampling();

        /* == Apply graph optimizations == */
        if (api::optimizeSRDAG()) {
            //monitor_->startSampling();
            optims::optimize(srdag_.get());
            //monitor_->endSampling();
        }

        /* == Schedule / Map current Single-Rate graph == */
        //monitor_->startSampling();
        scheduler_->update();
        scheduler_->mappingScheduling();
        rt::platform()->runner(0)->run(false);
        //monitor_->endSampling();

        /* == Wait for all parameters to be resolved == */
        if (!dynamicJobStack.empty()) {
            if (log::enabled<log::TRANSFO>()) {
                log::info<log::TRANSFO>("Waiting fo dynamic parameters..\n");
            }
            const auto &grtIx = archi::platform()->spiderGRTPE()->virtualIx();
            size_t readParam = 0;
            while (readParam != dynamicJobStack.size()) {
                Notification notification;
                rt::platform()->communicator()->popParamNotification(notification);
                if (notification.type_ == NotificationType::JOB_SENT_PARAM) {
                    /* == Get the message == */
                    ParameterMessage message;
                    rt::platform()->communicator()->pop(message, grtIx, notification.notificationIx_);

                    /* == Get the config vertex == */
                    const auto *cfg = srdag_->vertex(message.vertexIx_);
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
                    throwSpiderException("expected parameter notification");
                }
            }

            /* == Transform dynamic jobs == */
            //monitor_->startSampling();
            transformDynamicJobs(staticJobStack, dynamicJobStack);
            //monitor_->endSampling();

            /* == Apply graph optimizations == */
            if (api::optimizeSRDAG()) {
                //monitor_->startSampling();
                optims::optimize(srdag_.get());
                //monitor_->endSampling();
            }

            /* == Schedule / Map current Single-Rate graph == */
            scheduler_->update();
            scheduler_->mappingScheduling();
        }
    }

    if (api::exportSRDAG()) {
        auto exporter = pisdf::PiSDFDOTExporter{ srdag_.get() };
        exporter.printFromPath("./srdag.dot");
    }

    /* == If there are jobs left, run == */
    rt::platform()->runner(0)->run(false);

    /* == Clear the srdag == */
    srdag_->clear();

    /* == Clear the scheduler == */
    scheduler_->clear();
    return true;
}

void spider::JITMSRuntime::updateJobStack(spider::vector<spider::srdag::TransfoJob> &src,
                                          spider::vector<spider::srdag::TransfoJob> &dest) const {
    std::for_each(src.begin(), src.end(), [&](spider::srdag::TransfoJob &job) {
        dest.emplace_back(std::move(job));
    });
}

void spider::JITMSRuntime::transformJobs(spider::vector<spider::srdag::TransfoJob> &iterJobStack,
                                         spider::vector<spider::srdag::TransfoJob> &staticJobStack,
                                         spider::vector<spider::srdag::TransfoJob> &dynamicJobStack) {
    for (auto &job : iterJobStack) {
        /* == Transform current job == */
        auto &&result = srdag::singleRateTransformation(job, srdag_.get());

        /* == Move static TransfoJob into static JobStack == */
        updateJobStack(result.first, staticJobStack);

        /* == Move dynamic TransfoJob into dynamic JobStack == */
        updateJobStack(result.second, dynamicJobStack);
    }
}

void spider::JITMSRuntime::transformStaticJobs(spider::vector<spider::srdag::TransfoJob> &staticJobStack,
                                               spider::vector<spider::srdag::TransfoJob> &dynamicJobStack) {
    auto tempJobStack = factory::vector<srdag::TransfoJob>(StackID::TRANSFO);
    while (!staticJobStack.empty()) {
        /* == Transform jobs of current static stack == */
        transformJobs(staticJobStack, tempJobStack, dynamicJobStack);
        /* == Swap vectors == */
        staticJobStack.swap(tempJobStack);
        tempJobStack.clear();
    }
}

void spider::JITMSRuntime::transformDynamicJobs(spider::vector<srdag::TransfoJob> &staticJobStack,
                                                spider::vector<srdag::TransfoJob> &dynamicJobStack) {
    auto tempJobStack = factory::vector<srdag::TransfoJob>(StackID::TRANSFO);
    /* == Transform jobs of current dynamic stack == */
    transformJobs(dynamicJobStack, staticJobStack, tempJobStack);
    /* == Swap vectors == */
    dynamicJobStack.swap(tempJobStack);
}
