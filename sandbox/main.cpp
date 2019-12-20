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
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/InHeritedParam.h>
#include <graphs/pisdf/DynamicParam.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <api/spider.h>
#include <graphs-tools/exporter/PiSDFDOTExporter.h>
#include <runtime/algorithm/JITMSRuntime.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/schedule/exporter/SchedSVGGanttExporter.h>
#include <scheduling/scheduler/BestFitScheduler.h>
#include <runtime/interface/Notification.h>
#include <runtime/interface/Message.h>

#include <thread/Thread.h>
#include <thread/Barrier.h>
#include <common/Time.h>
#include <graphs-tools/numerical/brv.h>
#include <archi/MemoryInterface.h>

void createArchi();

void spiderTest();

int print() {
    fprintf(stderr, "coucou\n");
    return 0;
}

std::mutex mutex_;
spider::barrier barrier{ 3 };

void fn(int32_t id, int32_t affinity) {
    spider::this_thread::set_affinity(affinity);
    barrier.wait();
    {
        std::lock_guard<std::mutex> loc{ mutex_ };
        std::cout << "Thread #" << id << ": on CPU " << spider::this_thread::get_affinity() << "\n";
    }
    barrier.wait();
    std::lock_guard<std::mutex> loc{ mutex_ };
    std::cout << "Thread #" << id << ": on CPU " << spider::this_thread::get_affinity() << "\n";
}

void fn2(int32_t id, int32_t affinity) {
    spider::this_thread::set_affinity(affinity);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    barrier.wait();
    {
        std::lock_guard<std::mutex> loc{ mutex_ };
        std::cout << "Thread #" << id << ": on CPU " << spider::this_thread::get_affinity() << "\n";
    }
    barrier.wait();
    std::lock_guard<std::mutex> loc{ mutex_ };
    std::cout << "Thread #" << id << ": on CPU " << spider::this_thread::get_affinity() << "\n";
}


int main(int, char **) {

    spiderTest();

    std::cout << sizeof(spider::MemoryInterface) << std::endl;
    std::cout << sizeof(spider::pisdf::Vertex) << std::endl;
    std::cout << sizeof(spider::pisdf::ExecVertex) << std::endl;
    std::cout << sizeof(spider::pisdf::Graph) << std::endl;
    std::cout << sizeof(spider::pisdf::Edge) << std::endl;
    std::cout << sizeof(spider::Expression) << std::endl;
    std::cout << sizeof(spider::ExpressionElt) << std::endl;
    std::cout << sizeof(RPNElement) << std::endl;
    std::cout << sizeof(RPNOperator) << std::endl;
    std::cout << sizeof(RPNElementType) << std::endl;
    std::cout << sizeof(RPNElementSubType) << std::endl;
    std::cout << sizeof(std::string) << std::endl;

    return 0;
}

void spiderTest() {
    spider::start();
    spider::api::setStackAllocatorPolicy(StackID::PISDF, spider::AllocatorPolicy::LINEAR_STATIC, sizeof(uint64_t),
                                         16392);
    spider::api::enableLogger(spider::log::Type::TRANSFO);
    spider::api::enableLogger(spider::log::Type::OPTIMS);
    spider::api::enableVerbose();
//    spider::api::disableSRDAGOptims();

//    {
//        auto *buffer = spider::allocate<char, StackID::GENERAL>(32764);
//        spider::deallocate(buffer);
//    }

//    if (0)
    {
//        createArchi();

        auto *graph = spider::api::createUserApplicationGraph("topgraph", 15, 15, 1);

        /* === Creating vertices === */

        auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
        auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1, 1);
        auto *subgraph = spider::api::createSubgraph(graph, "subgraph", 3, 4, 2, 1, 1);
        auto *input = spider::api::setInputInterfaceName(subgraph, 0, "input");
        auto *output = spider::api::setOutputInterfaceName(subgraph, 0, "output");
        auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 1, 1);
        auto *vertex_3 = spider::api::createVertex(subgraph, "vertex_3", 1, 1);
        auto *vertex_4 = spider::api::createVertex(graph, "vertex_4", 1);
        auto *cfg = spider::api::createConfigActor(subgraph, "cfg", 0, 0);

        /* === Creating param === */

        spider::api::createStaticParam(subgraph, "height", 10);
        spider::api::createDynamicParam(subgraph, "width");

        /* === Creating edges === */

        spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 1);
        spider::api::createEdge(vertex_1, 0, 2, subgraph, 0, 1);
        spider::api::createEdge(input, 0, 1, vertex_2, 0, 1);
        spider::api::createEdge(vertex_2, 0, "width", vertex_3, 0, "1");
        spider::api::createEdge(vertex_3, 0, 1, output, 0, 1);
        spider::api::createEdge(subgraph, 0, 5, vertex_4, 0, 5);
//        spider::api::createEdge(cfg, 0, 1, vertex_2, 1, 1);

        /* === Export dot === */

        {
            auto exporter = spider::pisdf::PiSDFDOTExporter{ graph };
            exporter.printFromPath("./original.dot");
        }

//        for (auto j = 0; j < 10; ++j)
        {
            const auto &start = spider::time::now();
            spider::JITMSRuntime runtime(spider::pisdf::applicationGraph());
            runtime.execute();
            const auto &end = spider::time::now();
            std::cout << spider::time::duration::microseconds(start, end) << std::endl;
        }

        /* === Export dot === */

        {
            auto exporter = spider::pisdf::PiSDFDOTExporter{ graph };
            exporter.printFromPath("./new.dot");
        }


        {
            using namespace spider;
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


//            BestFitScheduler scheduler{graph};
//            scheduler.mappingScheduling();
//            spider::SVGGanttExporter ganttExporter{&scheduler.schedule()};
//            ganttExporter.print();
        }
    }
    spider::quit();
}

void createArchi() {
    spider::api::createPlatform(1, 4);

    auto *x86MemoryUnit = spider::api::createMemoryUnit(20000);

    auto *x86MemoryInterface = spider::api::createMemoryInterface(x86MemoryUnit);

    auto *x86Cluster = spider::api::createCluster(4, x86MemoryUnit, x86MemoryInterface);

    auto x86PECore0 = spider::api::createProcessingElement(0, 0, x86Cluster, "x86-Core0", spider::PEType::LRT);

    spider::api::createProcessingElement(1, 1, x86Cluster, "x86-Core1", spider::PEType::LRT);

    spider::api::createProcessingElement(2, 2, x86Cluster, "x86-Core2", spider::PEType::LRT);

    spider::api::createProcessingElement(3, 3, x86Cluster, "x86-Core3", spider::PEType::LRT);

    spider::api::setSpiderGRTPE(x86PECore0);
}
