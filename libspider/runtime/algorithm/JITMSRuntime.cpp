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
#include <graphs-tools/exporter/DOTExporter.h>
#include <scheduling/schedule/exporter/SVGGanttExporter.h>
#include <scheduling/scheduler/BestFitScheduler.h>
#include <scheduling/schedule/exporter/XMLGanttExporter.h>
#include <scheduling/scheduler/GreedyScheduler.h>
#include <monitor/Monitor.h>


/* === Method(s) implementation === */

bool spider::JITMSRuntime::execute() const {
    // TODO: put root graph into a top graph
    auto *graph = spider::pisdfGraph();

    /* == Create the Single-Rate graph == */
    auto *srdag = api::createGraph("srdag-" + graph->name(),
                                   0, /* = Number of actors = */
                                   0, /* = Number of edges = */
                                   0, /* = Number of parameters = */
                                   0, /* = Number of input interfaces = */
                                   0, /* = Number of output interfaces = */
                                   0, /* = Number of config actors = */
                                   StackID::TRANSFO);

    /* == Apply first transformation of root graph == */
    auto &&rootJob = srdag::TransfoJob(graph, 0, UINT32_MAX);
    rootJob.params_ = graph->params();
    auto &&resultRootJob = srdag::singleRateTransformation(rootJob, srdag);

    /* == Initialize the job stacks == */
    auto staticJobStack{ std::move(resultRootJob.first) };
    auto dynamicJobStack{ std::move(resultRootJob.second) };

    while (!staticJobStack.empty()) {
        //monitor_->startSampling();
        while (!staticJobStack.empty()) {
            /* == Transform static graphs == */
            auto &&result = srdag::singleRateTransformation(staticJobStack.back(), srdag);

            /* == Pop the job == */
            staticJobStack.pop_back();

            /* == Move static TransfoJob into static JobStack == */
            std::for_each(result.first.begin(), result.first.end(), [&](srdag::TransfoJob &job) {
                staticJobStack.emplace_back(std::move(job));
            });

            /* == Move dynamic TransfoJob into dynamic JobStack == */
            std::for_each(result.second.begin(), result.second.end(), [&](srdag::TransfoJob &job) {
                dynamicJobStack.emplace_back(std::move(job));
            });
        }
        //monitor_->endSampling();

        /* == Apply graph optimizations == */
        if (api::optimizeSRDAG()) {
            //monitor_->startSampling();
            PiSDFGraphOptimizer()(srdag);
            //monitor_->endSampling();
        }

        /* == Schedule current Single-Rate graph == */
        //monitor_->startSampling();
        // TODO: add schedule
        //monitor_->endSampling();

        /* == Run graph for dynamic params to be resolved == */
        if (!dynamicJobStack.empty() && api::verbose() && log_enabled<LOG_TRANSFO>()) {
            log::verbose<LOG_TRANSFO>("Running graph with config actors..\n");
        }
        // TODO: run graph

        /* == Transform dynamic graphs == */
        //monitor_->startSampling();
        while (!dynamicJobStack.empty()) {
            if (api::verbose() && log_enabled<LOG_TRANSFO>()) {
                log::verbose<LOG_TRANSFO>("Resolved parameters.\n");
            }
            /* == Transform dynamic graphs == */
            const auto &job = dynamicJobStack.back();
            auto &&result = srdag::singleRateTransformation(job, srdag);

            /* == Pop the job == */
            dynamicJobStack.pop_back();

            /* == Move static TransfoJob into static JobStack == */
            std::for_each(result.first.begin(), result.first.end(), [&](srdag::TransfoJob &job) {
                staticJobStack.emplace_back(std::move(job));
            });

            /* == Move dynamic TransfoJob into dynamic JobStack == */
            std::for_each(result.second.begin(), result.second.end(), [&](srdag::TransfoJob &job) {
                dynamicJobStack.emplace_back(std::move(job));
            });
        }
        //monitor_->endSampling();
    }

    /* == Apply graph optimizations == */
    if (api::optimizeSRDAG()) {
        //monitor_->startSampling();
        PiSDFGraphOptimizer()(srdag);
        //monitor_->endSampling();
    }

    /* == Schedule and run final Single-Rate graph == */
    //monitor_->startSampling();
//    spider::BestFitScheduler scheduler{ srdag };
//    scheduler.mappingScheduling();
    //monitor_->endSampling();
    // TODO: run graph

    // TODO: export srdag if export is enabled

//    spider::XMLGanttExporter ganttExporter{&scheduler.schedule(), srdag};
//    ganttExporter.print();

    pisdf::DOTExporter(srdag).print("./srdag.dot");

    /* == Destroy the sr-dag == */
    destroy(srdag);
    return true;
}
