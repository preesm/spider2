/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
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

#include <chrono>
#include <graphs/pisdf/params/Param.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/params/InHeritedParam.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <spider.h>
#include <graphs-tools/exporter/DOTExporter.h>
#include <runtime/master-slave/JITMSRuntime.h>

#include "Test.h"
#include "exp.h"

void createArchi();

void spiderTest();

int print() {
    fprintf(stderr, "coucou\n");
    return 0;
}

int main(int, char **) {
    spiderTest();
    return 0;
}

void spiderTest() {
    spider::start();
    spider::api::initStack(StackID::PISDF, "pisdf-stack", AllocatorType::FREELIST, 16392, FreeListPolicy::FIND_FIRST);
    spider::api::initStack(StackID::TRANSFO, "transfo-stack", AllocatorType::FREELIST, 16392,
                           FreeListPolicy::FIND_FIRST);
    spider::api::initStack(StackID::ARCHI, "archi-stack", AllocatorType::FREELIST, 16392,
                           FreeListPolicy::FIND_FIRST);
    spider::api::enableLogger<LOG_TRANSFO>();
    spider::api::enableLogger<LOG_OPTIMS>();
    spider::api::enableVerbose();

//    createArchi();

    for (std::uint32_t i = 0; i < 1; ++i) {
        auto *&graph = spider::pisdfGraph();
        graph = spider::api::createGraph("topgraph", 15, 15, 1);

        /* === Creating vertices === */

        auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
        auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1, 1);
        auto *subgraph = spider::api::createSubraph(graph, "subgraph", 3, 4, 2, 1, 1);
        auto *input = spider::api::setInputInterfaceName(subgraph, 0, "input");
        auto *output = spider::api::setOutputInterfaceName(subgraph, 0, "output");
        auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 2, 1);
        auto *vertex_3 = spider::api::createVertex(subgraph, "vertex_3", 1, 1);
        auto *vertex_4 = spider::api::createVertex(graph, "vertex_4", 1);

        auto *cfg = spider::api::createConfigActor(subgraph, "cfg", 0, 1);

        /* === Creating param === */


        /* === Creating edges === */

        auto *edge = spider::api::createEdge(vertex_0, 0, 5, vertex_1, 0, 5);
        spider::api::createEdge(vertex_1, 0, 2, subgraph, 0, 1);
        spider::api::createEdge(input, 0, 1, vertex_2, 0, 5);
        spider::api::createEdge(vertex_2, 0, 1, vertex_3, 0, 5);
        spider::api::createEdge(vertex_3, 0, 2, output, 0, 5);
        spider::api::createEdge(subgraph, 0, 5, vertex_4, 0, 5);
        spider::api::createEdge(cfg, 0, 15, vertex_2, 1, 1);
        spider::api::createDelay(edge, 3);

        /* === Creating param === */

        spider::api::createStaticParam(graph, "width", 10);
        spider::api::createStaticParam(subgraph, "height", 10);
        spider::api::createDynamicParam(subgraph, "width");

        {
            spider::JITMSRuntime runtime(graph);
            runtime.execute();
        }

        /* === Export dot === */

        {
            auto exporter = spider::pisdf::DOTExporter{ graph };
            exporter.print("./new.dot");
        }

//        {
//            spider::SRDAG::splitDynamicGraph(subgraph);
//            auto exporter = spider::PiSDF::DOTExporter{ graph };
//            exporter.print("./split.dot");
//        }


//        {
//            using namespace Spider;
//            Schedule schedule;
//            auto job = ScheduleJob(0, 0, 0, 0, 0);
//            job.setMappingStartTime(15);
//            job.setMappingEndTime(115);
//            schedule.add(std::move(job));
//            job = ScheduleJob(1, 1, 0, 0, 0);
//            job.setMappingStartTime(120);
//            job.setMappingEndTime(320);
//            schedule.add(std::move(job));
//            job = ScheduleJob(2, 2, 1, 0, 0);
//            job.setMappingStartTime(60);
//            job.setMappingEndTime(80);
//            schedule.add(std::move(job));
//            job = ScheduleJob(3, 3, 2, 0, 0);
//            job.setMappingStartTime(0);
//            job.setMappingEndTime(700);
//            schedule.add(std::move(job));
//
//            pisdfGraph() = graph;
//
//            SVGGanttExporter ganttExporter{&schedule};
//            ganttExporter.print();
//        }
    }
    spider::quit();
}

void createArchi() {
    spider::api::createPlatform();

    auto *x86MemoryUnit = spider::api::createMemoryUnit(nullptr, 20000);

    auto *x86Cluster = spider::api::createCluster(4, x86MemoryUnit);

    auto x86PECore0 = spider::api::createPE(0, 0, 0,
                                            x86Cluster,
                                            "x86-Core0",
                                            spider::PEType::LRT_PE,
                                            spider::HWType::PHYS_PE);

    spider::api::createPE(1, 1, 1,
                          x86Cluster,
                          "x86-Core1",
                          spider::PEType::LRT_PE,
                          spider::HWType::PHYS_PE);

    spider::api::createPE(2, 2, 2,
                          x86Cluster,
                          "x86-Core2",
                          spider::PEType::LRT_PE,
                          spider::HWType::PHYS_PE);

    spider::api::createPE(3, 3, 3,
                          x86Cluster,
                          "x86-Core3",
                          spider::PEType::LRT_PE,
                          spider::HWType::PHYS_PE);

    spider::api::setSpiderGRTPE(x86PECore0);
}
