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

#include <iostream>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/InHeritedParam.h>
#include <graphs/pisdf/DynamicParam.h>
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
#include <graphs-tools/helper/visitors/PiSDFDefaultVisitor.h>
#include <graphs-tools/transformation/srdag/Transformation.h>
#include <scheduling/schedule/ScheduleTask.h>
#include <containers/array_handle.h>
#include <csignal>
#include <runtime/runner/JITMSRTRunner.h>
#include <graphs-tools/transformation/srdagless/SRLessHandler.h>

void createUserPlatform();

void createRuntimePlatform();

void simpleTest();

void simpleNoExecHTest();

void simpleNoExecTest();

void spiderTest();

void spiderSmallTest();

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

void testGraphRec() {
    using namespace spider;
    spider::start();
    createUserPlatform();
    /* == Create the graph == */
    auto *graph = spider::api::createGraph(
            /* = Name of the application graph = */ "test",
            /* = Number of actors              = */  14,
            /* = Number of edges               = */  7,
            /* = Number of parameters          = */  0,
            /* = Number of input interfaces    = */  0,
            /* = Number of output interfaces   = */  0,
            /* = Number of config actors       = */  0);

    /* === Creates the actor(s) == */

    auto *vertex_I = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "I",
            /* = Number of input edges  = */  0,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL);

    /* == Set the timings of the vertex I == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_I, 0, "100");

    auto *vertex_B = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "B",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  0,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL);

    /* == Set the timings of the vertex B == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_B, 0, "100");

    auto *vertex_F = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "F",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  0,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL);

    /* == Set the timings of the vertex F == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_F, 0, "100");

    auto *vertex_A = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "A",
            /* = Number of input edges  = */  0,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL);

    /* == Set the timings of the vertex A == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_A, 0, "100");

    auto *vertex_G = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "G",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  0,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL);

    /* == Set the timings of the vertex G == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_G, 0, "100");

    auto *vertex_H = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "H",
            /* = Number of input edges  = */  0,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL);

    /* == Set the timings of the vertex H == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_H, 0, "100");

    auto *vertex_S = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "S",
            /* = Number of input edges  = */  0,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL);

    /* == Set the timings of the vertex S == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_S, 0, "100");

    auto *vertex_E = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "E",
            /* = Number of input edges  = */  1,
            /* = Number of output edges = */  0,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL);

    /* == Set the timings of the vertex E == */
    spider::api::setVertexExecutionTimingOnHWType(vertex_E, 0, "100");

    /* === Creates the edge(s) === */

    /* == Edge A[out] -> [in]B == */
    auto *edge_A_out__B_in = spider::api::createEdge(
            /* = Source vertex          = */  vertex_A,
            /* = Source port ix         = */  0,
            /* = Source rate expression = */ "(4) * 1",
            /* = Sink vertex            = */  vertex_B,
            /* = Sink port ix           = */  0,
            /* = sink rate expression   = */ "(2) * 1");

    /* == Set the delay on the edge == */
    spider::api::createLocalDelay(/* = Edge of the delay       = */  edge_A_out__B_in,
            /* = Expression of the delay = */ "(3) * 1",
                                                                     vertex_S,
                                                                     0,
                                                                     "(3) * 1",
                                                                     vertex_G,
                                                                     0,
                                                                     "(3) * 1");

    /* == Edge A_out__B_in[get] -> [in]G == */
    auto *edge_A_out__B_in_get__G_in = vertex_G->inputEdge(0);

    /* == Set the delay on the edge == */
    spider::api::createLocalDelay(/* = Edge of the delay       = */
            edge_A_out__B_in_get__G_in,
            /* = Expression of the delay = */ "(2) * 1",
            vertex_I,
            0,
            "2",
            vertex_E,
            0,
            "2");

    /* == Edge A_out__B_in_get__G_in[get] -> [in]E == */
    auto *edge_A_out__B_in_get__G_in_get__E_in = vertex_E->inputEdge(0);

    /* == Set the delay on the edge == */
    spider::api::createLocalDelay(/* = Edge of the delay       = */ edge_A_out__B_in_get__G_in_get__E_in,
            /* = Expression of the delay = */ "(5) * 1",
                                                                    vertex_H,
                                                                    0,
                                                                    "1",
                                                                    vertex_F,
                                                                    0,
                                                                    "5");


    auto *vertex_Q = spider::api::createVertexFromType(
            /* = Graph of the vertex    = */  graph,
            /* = Name of the actor      = */ "Q",
            /* = Number of input edges  = */  0,
            /* = Number of output edges = */  1,
            /* = Type of the vertex     = */  pisdf::VertexType::NORMAL);


    auto *edge_H_out = vertex_H->outputEdge(0);
    spider::api::createLocalDelay(/* = Edge of the delay       = */ edge_H_out,
            /* = Expression of the delay = */ "(3) * 1",
                                                                    vertex_Q,
                                                                    0,
                                                                    "1");

    spider::api::createThreadRTPlatform();
    spider::api::exportGraphToDOT(graph);
    spider::api::enableExportSRDAG();
    spider::api::disableSRDAGOptims();
    const auto start = time::now();
    auto context = spider::createRuntimeContext(graph, spider::RunMode::LOOP, 100, spider::RuntimeType::FAST_JITMS,
                                                spider::SchedulingPolicy::SRLESS_LIST_BEST_FIT);
    spider::run(context);
    spider::destroyRuntimeContext(context);
    const auto end = time::now();
    std::cout << time::duration::nanoseconds(start, end) << '\n';
    auto context2 = spider::createRuntimeContext(graph, spider::RunMode::LOOP, 100, spider::RuntimeType::JITMS,
                                                 spider::SchedulingPolicy::LIST_BEST_FIT);
    spider::run(context2);
    spider::destroyRuntimeContext(context2);
    spider::api::destroyGraph(graph);
    spider::quit();
}

int main(int, char **) {
//    simpleNoExecHTest();
//    simpleNoExecTest();
//    testGraphRec();
    simpleTest();
//    spiderSmallTest();
//    spiderTest();
    return 0;
}

void simpleNoExecHTest() {
    spider::start();
    spider::api::enableExportSRDAG();
    spider::api::enableExportGantt();
    spider::api::enableVerbose();
    createUserPlatform();
    auto *graph = spider::api::createGraph("topgraph", 1, 0, 0);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 2);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 2, 0);
    auto *vertex_2 = spider::api::createSubgraph(graph, "vertex_2", 1, 2, 0, 1, 1);
    auto *vertex_3 = spider::api::createVertex(vertex_2, "vertex_3", 1, 1);
    auto *input = spider::api::setInputInterfaceName(vertex_2, 0, "in");
    auto *output = spider::api::setOutputInterfaceName(vertex_2, 0, "out");
    spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 1);
    spider::api::createEdge(vertex_0, 1, 0, vertex_2, 0, 0);
    spider::api::createEdge(vertex_2, 0, 0, vertex_1, 1, 0);
    spider::api::createEdge(input, 0, 0, vertex_3, 0, 1);
    spider::api::createEdge(vertex_3, 0, 1, output, 0, 0);
    spider::api::exportGraphToDOT(graph);
    spider::api::createThreadRTPlatform();
    spider::api::createRuntimeKernel(vertex_0,
                                     [](const int64_t *, int64_t *, void *[], void *output[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(output[0]);
                                         buffer[0] = 3;
                                         spider::log::info("vertex_0:0 writing: 3..\n");
                                     });

    spider::api::createRuntimeKernel(vertex_1,
                                     [](const int64_t *, int64_t *, void *input[], void *[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(input[0]);
                                         spider::log::info("vertex_1 reading %d\n", buffer[0]);
                                     });

    spider::api::createRuntimeKernel(vertex_2,
                                     [](const int64_t *, int64_t *, void *input[], void *[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(input[0]);
                                         spider::log::info("vertex_2 reading %d\n", buffer[0]);
                                     });
    auto context = spider::createRuntimeContext(graph, spider::RunMode::LOOP, 1, spider::RuntimeType::JITMS,
                                                spider::SchedulingPolicy::LIST_BEST_FIT);
    spider::run(context);
    spider::destroyRuntimeContext(context);
    spider::api::destroyGraph(graph);
    spider::quit();
}


void simpleNoExecTest() {
    spider::start();
    spider::api::enableExportSRDAG();
    spider::api::enableExportGantt();
    spider::api::enableVerbose();
    createUserPlatform();
    auto *graph = spider::api::createGraph("topgraph", 1, 0, 0);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 2);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 2, 0);
    auto *vertex_2 = spider::api::createVertex(graph, "vertex_2", 1, 1);
    spider::api::createEdge(vertex_0, 0, 2, vertex_1, 0, 1);
    spider::api::createEdge(vertex_0, 1, 2, vertex_2, 0, 1);
    spider::api::createEdge(vertex_2, 0, 1, vertex_1, 1, 1);
    spider::api::exportGraphToDOT(graph);
    spider::api::createThreadRTPlatform();
    spider::api::createRuntimeKernel(vertex_0,
                                     [](const int64_t *, int64_t *, void *[], void *output[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(output[0]);
                                         buffer[0] = 3;
                                         spider::log::info("vertex_0:0 writing: 3..\n");
//                                         reinterpret_cast<char *>(output[1])[0] = 14;
//                                         spider::log::info("vertex_0:1 writing: 14..\n");
                                     });

    spider::api::createRuntimeKernel(vertex_1,
                                     [](const int64_t *, int64_t *, void *input[], void *[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(input[0]);
                                         spider::log::info("vertex_1 reading %d\n", buffer[0]);
                                     });

    spider::api::createRuntimeKernel(vertex_2,
                                     [](const int64_t *, int64_t *, void *input[], void *[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(input[0]);
                                         spider::log::info("vertex_2 reading %d\n", buffer[0]);
                                     });
//    auto context = spider::createRuntimeContext(graph, spider::RunMode::LOOP, 1, spider::RuntimeType::JITMS,
//                                                spider::SchedulingPolicy::LIST_BEST_FIT);
    auto context = spider::createRuntimeContext(graph, spider::RunMode::LOOP, 1, spider::RuntimeType::FAST_JITMS,
                                                spider::SchedulingPolicy::SRLESS_LIST_BEST_FIT);
    spider::run(context);
    spider::destroyRuntimeContext(context);
    spider::api::destroyGraph(graph);
    spider::quit();
}


void simpleTest() {
    spider::start();
//    spider::api::enableExportGantt();
    spider::api::enableExportSRDAG();
//    spider::api::useSVGGanttExporter();
    createUserPlatform();
    auto *graph = spider::api::createGraph("topgraph", 1, 0, 0);
    auto *vertex_0 = spider::api::createVertex(graph, "A", 0, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "B", 1, 1);
    auto *vertex_2 = spider::api::createVertex(graph, "C", 1, 0);
//    auto *setter = spider::api::createVertex(graph, "S", 0, 1);
//    auto *getter = spider::api::createVertex(graph, "G", 1, 0);
    spider::api::createEdge(vertex_0, 0, 4, vertex_1, 0, 1);
    spider::api::createEdge(vertex_1, 0, 3, vertex_2, 0, 4);
//    auto *delay = spider::api::createLocalDelay(edge, "3", setter, 0, "1", getter, 0, "1");
//    auto *vertex_init = spider::api::createVertex(graph, "I", 0, 1);
//    auto *vertex_end = spider::api::createVertex(graph, "E", 1, 0);
//    spider::api::createLocalDelay(delay->vertex()->outputEdge(0), "2", vertex_init, 0, "1", vertex_end, 0, "2");
//    auto *vertex_d = spider::api::createVertex(graph, "D", 0, 1);
//    auto *vertex_h = spider::api::createVertex(graph, "H", 1, 0);
//    spider::api::createLocalDelay(delay->vertex()->inputEdge(0), "5", vertex_d, 0, "1", vertex_h, 0, "1");

    spider::api::createThreadRTPlatform();
    spider::api::exportGraphToDOT(graph);
    spider::api::createRuntimeKernel(vertex_0,
                                     [](const int64_t *, int64_t *, void *[], void *output[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(output[0]);
                                         buffer[1] = 3;
                                         buffer[0] = 14;
                                         buffer[2] = 15;
                                         buffer[3] = 92;
                                     });

    spider::api::createRuntimeKernel(vertex_1,
                                     [](const int64_t *, int64_t *, void *input[], void *[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(input[0]);
                                         spider::log::info("vertex_1 reading %d\n", buffer[0]);
                                     });
    auto start = spider::time::now();
    auto context = spider::createRuntimeContext(graph, spider::RunMode::LOOP, 1, spider::RuntimeType::FAST_JITMS,
                                                spider::SchedulingPolicy::SRLESS_LIST_BEST_FIT);
    spider::run(context);
    spider::destroyRuntimeContext(context);
    auto end = spider::time::now();
    std::cerr << "fast-jitms: " << spider::time::duration::nanoseconds(start, end) << std::endl;
    start = spider::time::now();
    auto context2 = spider::createRuntimeContext(graph, spider::RunMode::LOOP, 1, spider::RuntimeType::JITMS,
                                                 spider::SchedulingPolicy::LIST_BEST_FIT);
    spider::run(context2);
    spider::destroyRuntimeContext(context2);
    end = spider::time::now();
    std::cerr << "jitms: " << spider::time::duration::nanoseconds(start, end) << std::endl;
    spider::api::destroyGraph(graph);
    spider::quit();
}

void spiderSmallTest() {
    spider::start();
    createUserPlatform();
    spider::api::createThreadRTPlatform();
    auto *graph = spider::api::createGraph("topgraph", 1, 0, 0);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1, 1);
    auto *subgraph = spider::api::createSubgraph(graph, "subgraph", 3, 4, 2, 1, 1);
    auto *vertex_3 = spider::api::createVertex(graph, "vertex_3", 1, 0);
    auto *input = spider::api::setInputInterfaceName(subgraph, 0, "input");
    auto *output = spider::api::setOutputInterfaceName(subgraph, 0, "output");
    auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 2, 2);
    spider::api::createEdge(vertex_0, 0, 3, vertex_1, 0, 1);
    spider::api::createEdge(vertex_1, 0, 2, subgraph, 0, 1);
    spider::api::createEdge(input, 0, 1, vertex_2, 0, 1);
    spider::api::createEdge(vertex_2, 0, 1, output, 0, 1);
    spider::api::createEdge(subgraph, 0, 1, vertex_3, 0, 1);
    auto *edge = spider::api::createEdge(vertex_2, 1, 1, vertex_2, 1, 1);
    spider::api::createLocalDelay(edge, "1");
    spider::api::exportGraphToDOT(graph, "./graph.dot");
    size_t i = 0;
    spider::api::setVertexExecutionTimingOnHWType(vertex_2, i, "50");
    spider::api::setVertexExecutionTimingOnHWType(vertex_3, i, "200");
    spider::api::setVertexMappableOnPE(vertex_3, i++, false);
    spider::api::enableExportSRDAG();
    spider::api::enableExportGantt();
    {
        spider::JITMSRuntime runtime(graph, spider::SchedulingPolicy::LIST_BEST_FIT);
        runtime.execute();
    }
    spider::api::destroyGraph(graph);
    spider::quit();
}


void spiderTest() {
    spider::start();
    spider::api::enableLogger(spider::log::Type::TRANSFO);
//    spider::api::enableLogger(spider::log::Type::OPTIMS);
    spider::api::enableLogger(spider::log::Type::LRT);
//    spider::api::enableLogger(spider::log::Type::SCHEDULE);
//    spider::api::enableVerbose();

//    spider::api::enableExportSRDAG();
    spider::api::enableExportGantt();
    {
        createUserPlatform();
        auto *graph = spider::api::createGraph("topgraph", 15, 15, 1);

        /* === Creating vertices === */

        auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
        auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1, 1);
        auto *subgraph = spider::api::createSubgraph(graph, "subgraph", 3, 4, 2, 1, 1);
        auto *input = spider::api::setInputInterfaceName(subgraph, 0, "input");
        auto *output = spider::api::setOutputInterfaceName(subgraph, 0, "output");
        auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 1, 2);
        auto *vertex_3 = spider::api::createVertex(subgraph, "vertex_3", 1, 1);
        auto *vertex_4 = spider::api::createVertex(graph, "vertex_4", 2);
        auto *vertex_5 = spider::api::createVertex(graph, "vertex_5", 0, 1);
        auto *width_setter = spider::api::createConfigActor(subgraph, "width_setter");
        auto *subsubgraph = spider::api::createSubgraph(subgraph, "subsubgraph", 2, 4, 2, 1);
        auto *sub_setter = spider::api::createConfigActor(subsubgraph, "sub_setter");
        auto *vertex_6 = spider::api::createVertex(subsubgraph, "vertex_6", 1, 0);
        auto *sub_input = spider::api::setInputInterfaceName(subsubgraph, 0, "sub_input");
        /* === Create the runtime kernels === */
        spider::api::createThreadRTPlatform();

        spider::api::createRuntimeKernel(vertex_0,
                                         [](const int64_t *, int64_t *, void *[], void *output[]) -> void {
                                             auto *buffer = reinterpret_cast<char *>(output[0]);
                                             buffer[0] = 78;
//                                             spider::printer::printf("vertex_0 writing: %d..\n", buffer[0]);
                                         });

        spider::api::createRuntimeKernel(vertex_1,
                                         [](const int64_t *, int64_t *, void *input[], void *output[]) -> void {
                                             auto *buffer = reinterpret_cast<char *>(output[0]);
//                                             spider::printer::printf("vertex_1 reading %d..\n",
//                                                                     reinterpret_cast<char *>(input[0])[0]);
                                             buffer[0] = 1;
//                                             buffer[1] = 2;
//                                             spider::printer::printf("vertex_1 writing %d..\n", buffer[0]);
//                                             spider::printer::printf("vertex_1 writing %c..\n", buffer[1]);
                                         });

        spider::api::createRuntimeKernel(width_setter,
                                         [](const int64_t *, int64_t *output, void *[], void *[]) -> void {
                                             static int64_t i = 10;
                                             output[0] = i;
                                             spider::printer::printf("width_setter: setting value: %" PRId64".\n",
                                                                     output[0]);
                                         });

        spider::api::createRuntimeKernel(sub_setter,
                                         [](const int64_t *, int64_t *output, void *[], void *[]) -> void {
                                             static int64_t i = 1;
                                             output[0] = i;
//                                             ++i;
                                             spider::printer::printf("sub_setter: setting value: %" PRId64".\n",
                                                                     output[0]);
                                         });

        spider::api::createRuntimeKernel(vertex_2,
                                         [](const int64_t *, int64_t *, void *[], void *[]) -> void {
//                                             spider::printer::printf("vertex_2: hello.\n");
                                         });

        spider::api::createRuntimeKernel(vertex_3,
                                         [](const int64_t *, int64_t *, void *[], void *[]) -> void {
//                                             spider::math::factorial(1000);
                                             spider::printer::printf("vertex_3: %f.\n", std::sqrt(3.1415926535));
                                         });

        spider::api::createRuntimeKernel(vertex_4,
                                         [](const int64_t *, int64_t *, void *[], void *[]) -> void {
//                                             spider::printer::printf("vertex_4: hello.\n");
                                         });

        spider::api::createRuntimeKernel(vertex_5,
                                         [](const int64_t *, int64_t *, void *[], void *[]) -> void {
//                                             spider::printer::printf("vertex_5: hello.\n");
                                         });

        spider::api::createRuntimeKernel(vertex_6,
                                         [](const int64_t *inputParam, int64_t *, void *[], void *[]) -> void {
                                             spider::printer::printf("vertex_6: hello %" PRId64".\n", inputParam[0]);
                                         });


        /* === Creating param === */
        spider::api::createStaticParam(subgraph, "height", 10);
        auto width = spider::api::createDynamicParam(subgraph, "width");
        auto sub_width = spider::api::createDynamicParam(subsubgraph, "sub_width");
        auto inherited_width = spider::api::createInheritedParam(subsubgraph, "width", width.get());
        auto width_derived = spider::api::createDynamicParam(subsubgraph, "width_derived", "width * sub_width");

        /* === Set param to vertex === */

        spider::api::addOutputParamToVertex(width_setter, width);
        spider::api::addOutputParamToVertex(sub_setter, sub_width);
        spider::api::addInputParamToVertex(vertex_2, width);
        spider::api::addInputParamToVertex(vertex_6, sub_width);
        spider::api::addInputRefinementParamToVertex(vertex_6, width_derived);

        /* === Creating edges === */

        spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 1);
        spider::api::createEdge(vertex_1, 0, 2, subgraph, 0, 1);
        spider::api::createEdge(input, 0, 1, vertex_2, 0, 1);
        spider::api::createEdge(vertex_2, 0, "width", vertex_3, 0, "1");
        spider::api::createEdge(vertex_3, 0, 5, output, 0, 5);
        spider::api::createEdge(subgraph, 0, 5, vertex_4, 1, 5);
        spider::api::createEdge(vertex_5, 0, 1, vertex_4, 0, 1);
        spider::api::createEdge(vertex_2, 1, "10", subsubgraph, 0, "10");
        spider::api::createEdge(sub_input, 0, "10", vertex_6, 0, "sub_width");


        /* === Export dot === */
        spider::api::exportGraphToDOT(graph, "./original.dot");
        fprintf(stderr, "%zu\n", graph->totalActorCount());
        try {
            const auto &start = spider::time::now();
            auto context = spider::createRuntimeContext(graph, spider::RunMode::LOOP, 10, spider::RuntimeType::JITMS,
                                                        spider::SchedulingPolicy::LIST_BEST_FIT);
            spider::run(context);
            spider::destroyRuntimeContext(context);
            const auto &end = spider::time::now();
            std::cout << spider::time::duration::milliseconds(start, end) << std::endl;
        } catch (std::runtime_error &e) {
            fprintf(stderr, "%s\n", e.what());
        }

        /* === Export dot === */
        spider::api::exportGraphToDOT(graph, "./new.dot");
    }
    spider::quit();
}

void createUserPlatform() {
    spider::api::createPlatform(1, 1);

    auto *x86MemoryInterface = spider::api::createMemoryInterface(1024 * 1024 * 1024);

    auto *x86Cluster = spider::api::createCluster(1, x86MemoryInterface);

    auto x86PECore0 = spider::api::createProcessingElement(0, 0, x86Cluster, "x86-Core0", spider::PEType::LRT, 0);

//    spider::api::createProcessingElement(1, 1, x86Cluster, "x86-Core1", spider::PEType::LRT, 1);
//
//    spider::api::createProcessingElement(2, 2, x86Cluster, "x86-Core2", spider::PEType::LRT, 2);
//
//    spider::api::createProcessingElement(3, 3, x86Cluster, "x86-Core3", spider::PEType::LRT, 3);

    spider::api::setSpiderGRTPE(x86PECore0);
}

void createRuntimePlatform() {
}
