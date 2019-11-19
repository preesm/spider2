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

#include <runtime/master-slave/JITMSRuntime.h>
#include <graphs-tools/transformation/srdag/SRDAGTransformation.h>
#include <graphs-tools/transformation/optims/PiSDFGraphOptimizer.h>
#include <graphs-tools/exporter/DOTExporter.h>
#include <scheduling/schedule/exporter/SVGGanttExporter.h>
#include <scheduling/scheduler/BestFitScheduler.h>
#include <spider-api/scenario.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

bool spider::JITMSRuntime::execute() const {
    // TODO: put root graph into a top graph
    auto *graph = spider::pisdfGraph();

    /* == Create the Single-Rate graph == */
    auto *srdag = spider::api::createGraph("srdag-" + graph->name(),
                                           0, /* = Number of actors = */
                                           0, /* = Number of edges = */
                                           0, /* = Number of parameters = */
                                           0, /* = Number of input interfaces = */
                                           0, /* = Number of output interfaces = */
                                           0, /* = Number of config actors = */
                                           StackID::TRANSFO);
    spider::api::createScenario(srdag, StackID::SCENARIO);

    /* == Apply first transformation of root graph == */
    auto &&rootJob = spider::srdag::Job(graph, 0, UINT32_MAX);
    rootJob.params_ = graph->params();
    auto &&resultRootJob = spider::srdag::staticSingleRateTransformation(rootJob, srdag);

    /* == Initialize the job stacks == */
    spider::srdag::JobStack staticJobStack;
    spider::srdag::JobStack dynamicJobStack;
    staticJobStack.swap(resultRootJob.first);
    dynamicJobStack.swap(resultRootJob.second);

    while (!staticJobStack.empty()) {
        // TODO: add time monitoring
        while (!staticJobStack.empty()) {
            /* == Transform static graphs == */
            // TODO: add time monitoring
            auto &&result = spider::srdag::staticSingleRateTransformation(staticJobStack.back(), srdag);

            /* == Pop the job == */
            staticJobStack.pop_back();

            /* == Move static Job into static JobStack == */
            std::for_each(result.first.begin(), result.first.end(), [&](spider::srdag::Job &job) {
                staticJobStack.emplace_back(std::move(job));
            });

            /* == Move dynamic Job into dynamic JobStack == */
            std::for_each(result.second.begin(), result.second.end(), [&](spider::srdag::Job &job) {
                dynamicJobStack.emplace_back(std::move(job));
            });
        }

        /* == Apply graph optimizations == */
        if (spider::api::srdagOptim()) {
            // TODO: add time monitoring
            PiSDFGraphOptimizer()(srdag);
        }

        /* == Schedule current Single-Rate graph == */
        // TODO: add time monitoring
        // TODO: add schedule

        /* == Run graph for dynamic params to be resolved == */
        if (!dynamicJobStack.empty() && spider::api::verbose() && log_enabled<LOG_TRANSFO>()) {
            spider::log::verbose<LOG_TRANSFO>("Running graph with config actors..\n");
        }
        // TODO: run graph

        /* == Transform dynamic graphs == */
        while (!dynamicJobStack.empty()) {
            if (spider::api::verbose() && log_enabled<LOG_TRANSFO>()) {
                spider::log::verbose<LOG_TRANSFO>("Resolved parameters.\n");
            }
            /* == Transform dynamic graphs == */
            // TODO: add time monitoring
            const auto &job = dynamicJobStack.back();
            auto &&result = spider::srdag::staticSingleRateTransformation(job, srdag);

            /* == Pop the job == */
            dynamicJobStack.pop_back();

            /* == Move static Job into static JobStack == */
            std::for_each(result.first.begin(), result.first.end(), [&](spider::srdag::Job &job) {
                staticJobStack.emplace_back(std::move(job));
            });

            /* == Move dynamic Job into dynamic JobStack == */
            std::for_each(result.second.begin(), result.second.end(), [&](spider::srdag::Job &job) {
                dynamicJobStack.emplace_back(std::move(job));
            });
        }
    }

    /* == Apply graph optimizations == */
    if (spider::api::srdagOptim()) {
        // TODO: add time monitoring
        PiSDFGraphOptimizer()(srdag);
    }

    /* == Schedule and run final Single-Rate graph == */
    // TODO: add time monitoring
    // TODO: add schedule
    // TODO: run graph
    spider::BestFitScheduler scheduler{srdag};
    scheduler.mappingScheduling();
    spider::SVGGanttExporter ganttExporter{&scheduler.schedule(), srdag};
    ganttExporter.print();

    spider::pisdf::DOTExporter(srdag).print("./srdag.dot");

    /* == Destroy the sr-dag == */
    spider::destroy(srdag);
    spider::deallocate(srdag);
    return true;
}
