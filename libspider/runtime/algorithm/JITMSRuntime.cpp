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
#include <graphs-tools/transformation/optims/PiSDFGraphOptimizer.h>
#include <graphs-tools/exporter/PiSDFDOTExporter.h>
#include <scheduling/schedule/exporter/SchedSVGGanttExporter.h>
#include <scheduling/scheduler/BestFitScheduler.h>
#include <scheduling/schedule/exporter/SchedXMLGanttExporter.h>
#include <scheduling/scheduler/GreedyScheduler.h>
#include <monitor/Monitor.h>
#include <scheduling/schedule/exporter/SchedStatsExporter.h>
#include <api/runtime-api.h>
#include <runtime/runner/RTRunner.h>
#include <runtime/interface/RTCommunicator.h>


/* === Static function(s) === */

/**
 * @brief Checks that if a graph, or any of its subgraphs, is dynamic then it has at least one config actor.
 * @param graph Pointer to the graph to test.
 * @throws spider::Exception if graph is dynamic and does not have any config actor.
 */
static void checkDynamicConsistency(const spider::pisdf::Graph *const graph) {
    for (const auto &subgraph : graph->subgraphs()) {
        if (subgraph->dynamic() && !subgraph->configVertexCount()) {
            throwSpiderException("subgraph [%s] has dynamic parameters but does not contains any config actors.",
                                 subgraph->name().c_str());
        }
        checkDynamicConsistency(subgraph);
    }
}

/* === Method(s) implementation === */

bool spider::JITMSRuntime::execute() const {
    if (!graph_->graph() && graph_->dynamic()) {
        throwSpiderException("Can not run JITMS runtime on dynamic graph without containing graph.");
    }
    /* == Check model consistency for dynamic graphs == */
    checkDynamicConsistency(graph_);

    /* == Create the Single-Rate graph == */
    auto *srdag = api::createGraph("srdag-" + graph_->name(),
                                   0, /* = Number of actors = */
                                   0, /* = Number of edges = */
                                   0, /* = Number of parameters = */
                                   0, /* = Number of input interfaces = */
                                   0, /* = Number of output interfaces = */
                                   0 /* = Number of config actors = */);

    /* == Create the scheduler == */
    spider::BestFitScheduler scheduler{ srdag };

    /* == Apply first transformation of root graph == */
    auto &&rootJob = srdag::TransfoJob(graph_, SIZE_MAX, UINT32_MAX, true);
    rootJob.params_ = graph_->params();
    auto &&resultRootJob = srdag::singleRateTransformation(rootJob, srdag);

    /* == Initialize the job stacks == */
    auto staticJobStack = containers::vector<srdag::TransfoJob>(StackID::TRANSFO);
    auto dynamicJobStack = containers::vector<srdag::TransfoJob>(StackID::TRANSFO);
    updateJobStack(resultRootJob.first, staticJobStack);
    updateJobStack(resultRootJob.second, dynamicJobStack);

    size_t dynamicStackJobOffset = 0;
    while (!staticJobStack.empty() || !dynamicJobStack.empty()) {
        /* == Transform static jobs == */
        //monitor_->startSampling();
        transformStaticJobs(staticJobStack, dynamicJobStack, &scheduler, srdag);
        //monitor_->endSampling();

        /* == Apply graph optimizations == */
        if (api::optimizeSRDAG()) {
            //monitor_->startSampling();
            PiSDFGraphOptimizer()(srdag);
            //monitor_->endSampling();
        }

        /* == Schedule / Map current Single-Rate graph == */
        //monitor_->startSampling();
        scheduler.update();
        scheduler.mappingScheduling();
        auto ganttExporter = SchedXMLGanttExporter{ &scheduler.schedule(), srdag };
        ganttExporter.print();
        auto statsExporter = SchedStatsExporter{ &scheduler.schedule() };
        statsExporter.print();
        //monitor_->endSampling();

        /* == Wait for all parameters to be resolved == */
        if (!dynamicJobStack.empty()) {
            if (api::verbose() && log::enabled<log::Type::TRANSFO>()) {
                log::verbose<log::Type::TRANSFO>("Running graph with config actors..\n");
            }
            const auto &grtIx = archi::platform()->spiderGRTPE()->virtualIx();
            size_t readParam = 0;
            while (readParam != dynamicJobStack.size()) {
                Notification notification;
                rt::platform()->communicator()->popParamNotification(notification);
                if (notification.type_ == NotificationType::JOB &&
                    notification.subtype_ == JobNotification::SENT_PARAM) {
                    /* == Get the message == */
                    ParameterMessage message;
                    rt::platform()->communicator()->pop(message, grtIx, notification.notificationIx_);

                    /* == Get the config vertex == */
                    auto *cfg = srdag->configVertices()[message.vertexIx_];
                    auto &job = dynamicJobStack.at(message.vertexIx_ - dynamicStackJobOffset);
                    auto paramIterator = message.params_.begin();
                    for (const auto &index : cfg->reference()->outputParamVector()) {
                        job.params_[index]->setValue((*(paramIterator++)));
                        if (api::verbose() && log::enabled()) {
                            log::verbose("Received value #%" PRId64" for parameter [%s].\n",
                                         job.params_[index]->value(), job.params_[index]->name().c_str());
                        }
                    }
                    readParam++;
                } else {
                    throwSpiderException("expected parameter notification");
                }
            }
            dynamicStackJobOffset += dynamicJobStack.size();
        }

        /* == Transform dynamic jobs == */
        //monitor_->startSampling();
        transformDynamicJobs(staticJobStack, dynamicJobStack, &scheduler, srdag);
        //monitor_->endSampling();

        pisdf::PiSDFDOTExporter(srdag).printFromPath("./srdag_dyna.dot");
        /* == Schedule / Map current Single-Rate graph == */
        scheduler.update();
        scheduler.mappingScheduling();
        ganttExporter.print();
    }

    /* == Apply graph optimizations == */
    if (api::optimizeSRDAG()) {
        //monitor_->startSampling();
        PiSDFGraphOptimizer()(srdag);
        //monitor_->endSampling();
    }

    // TODO: export srdag if export is enabled

    /* == Destroy the sr-dag == */
    destroy(srdag);
    return true;
}

/* === Private method(s) === */


void spider::JITMSRuntime::updateJobStack(spider::vector<spider::srdag::TransfoJob> &src,
                                          spider::vector<spider::srdag::TransfoJob> &dest) const {
    std::for_each(src.begin(), src.end(), [&](spider::srdag::TransfoJob &job) {
        job.ix_ = dest.size();
        dest.emplace_back(std::move(job));
    });
}

void spider::JITMSRuntime::transformStaticJobs(spider::vector<spider::srdag::TransfoJob> &staticJobStack,
                                               spider::vector<spider::srdag::TransfoJob> &dynamicJobStack,
                                               spider::Scheduler *scheduler,
                                               pisdf::Graph *srdag) const {
    auto tempJobStack = containers::vector<srdag::TransfoJob>(StackID::TRANSFO);
    while (!staticJobStack.empty()) {
        auto staticStackIterator = std::begin(staticJobStack);
        long staticStackIteratorPos = 0;
        while (staticStackIterator != std::end(staticJobStack)) {
            /* == Transform static graphs == */
            auto &job = (*(staticStackIterator));
            auto &&result = srdag::singleRateTransformation(job, srdag);

            /* == Adding parameters to the scheduler == */
            scheduler->addParameterVector(std::move(job.params_));

            /* == Move static TransfoJob into static JobStack == */
            updateJobStack(result.first, tempJobStack);

            /* == Move dynamic TransfoJob into dynamic JobStack == */
            updateJobStack(result.second, dynamicJobStack);

            /* == Update iterator (in case of change in size, previous may be invalidated == */
            staticStackIterator = std::begin(staticJobStack) + (++staticStackIteratorPos);
        }
        staticJobStack.swap(tempJobStack);
        tempJobStack.clear();
    }
}

void spider::JITMSRuntime::transformDynamicJobs(spider::vector<srdag::TransfoJob> &staticJobStack,
                                                spider::vector<srdag::TransfoJob> &dynamicJobStack,
                                                Scheduler *scheduler,
                                                pisdf::Graph *srdag) const {
    auto tempJobStack = containers::vector<srdag::TransfoJob>(StackID::TRANSFO);
    auto dynamicStackIterator = std::begin(dynamicJobStack);
    long dynamicStackIteratorPos = 0;
    while (dynamicStackIterator != std::end(dynamicJobStack)) {
        if (api::verbose() && log::enabled<log::Type::TRANSFO>()) {
            log::verbose<log::Type::TRANSFO>("Resolved parameters.\n");
        }
        /* == Transform dynamic graphs == */
        auto &job = (*(dynamicStackIterator));
        auto &&result = srdag::singleRateTransformation(job, srdag);

        /* == Adding parameters to the scheduler == */
        scheduler->addParameterVector(std::move(job.params_));

        /* == Move static TransfoJob into static JobStack == */
        updateJobStack(result.first, staticJobStack);

        /* == Move dynamic TransfoJob into dynamic JobStack == */
        updateJobStack(result.second, tempJobStack);

        /* == Update iterator (in case of change in size, previous may be invalidated == */
        dynamicStackIterator = std::begin(dynamicJobStack) + (++dynamicStackIteratorPos);
    }

    /* == Swap vectors == */
    dynamicJobStack.swap(tempJobStack);
}
