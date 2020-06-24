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

#include <common/Logger.h>
#include <graphs/pisdf/Graph.h>
#include <runtime/algorithm/JITMSRuntime.h>
#include <runtime/algorithm/FastJITMSRuntime.h>
#include "RuntimeTestCases.h"

extern bool spider2StopRunning;

/* === Function(s) definition === */

void spider::test::runtimeStaticFlat(spider::RuntimeType type, SchedulingPolicy algorithm,
                                     spider::FifoAllocatorType allocatorType) {
    auto *graph = spider::api::createGraph("topgraph", 1, 0, 0);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1, 0);
    spider::api::createEdge(vertex_0, 0, 4, vertex_1, 0, 1);

    spider::api::createThreadRTPlatform();
    spider::api::createRuntimeKernel(vertex_0,
                                     [](const int64_t *, int64_t *, void *[], void *output[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(output[0]);
                                         buffer[1] = 3;
                                         buffer[0] = 14;
                                         buffer[2] = 15;
                                         buffer[3] = 92;
                                         log::info("vertex_0 writing: 14 3 15..\n");
                                     });

    spider::api::createRuntimeKernel(vertex_1,
                                     [](const int64_t *, int64_t *, void *input[], void *[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(input[0]);
                                         log::info("vertex_1 reading %d\n", buffer[0]);
                                     });
    spider::Runtime *runtime;
    switch (type) {
        case spider::RuntimeType::JITMS:
            runtime = spider::make<spider::JITMSRuntime>(StackID::GENERAL, graph, algorithm, allocatorType);
            break;
        case spider::RuntimeType::FAST_JITMS:
            runtime = spider::make<spider::FastJITMSRuntime>(StackID::GENERAL, graph, algorithm, allocatorType);
            break;
        default:
            runtime = nullptr;
            break;
    }
    try {
        if (!runtime) {
            throw std::runtime_error("failed to create runtime.");
        }
        for (size_t i = 0; i < 10U && !spider2StopRunning; ++i) {
            runtime->execute();
        }
    } catch (spider::Exception &e) {
        throw std::runtime_error(e.what());
    }
    destroy(runtime);
    api::destroyGraph(graph);
}

void spider::test::runtimeStaticHierarchical(spider::RuntimeType type, SchedulingPolicy algorithm,
                                             spider::FifoAllocatorType allocatorType) {
    spider::api::createThreadRTPlatform();
    auto *graph = spider::api::createGraph("topgraph", 1, 0, 0);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1, 1);
    auto *subgraph = spider::api::createSubgraph(graph, "subgraph", 3, 4, 2, 1, 1);
    auto *vertex_3 = spider::api::createVertex(graph, "vertex_3", 1, 0);
    auto *input = spider::api::setInputInterfaceName(subgraph, 0, "input");
    auto *output = spider::api::setOutputInterfaceName(subgraph, 0, "output");
    auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 2, 2);
    auto *subsubgraph = spider::api::createSubgraph(subgraph, "subsubgraph", 3, 4, 2, 1, 1);
    auto *vertex_4 = spider::api::createVertex(subsubgraph, "vertex_4", 1, 1);


    spider::api::createEdge(vertex_0, 0, 3, vertex_1, 0, 1);
    spider::api::createEdge(vertex_1, 0, 2, subgraph, 0, 1);
    spider::api::createEdge(input, 0, 1, vertex_2, 0, 1);
    spider::api::createEdge(vertex_2, 0, 1, subsubgraph, 0, 1);
    spider::api::createEdge(api::getInputInterface(subsubgraph, 0), 0, 1, vertex_4, 0, 1);
    spider::api::createEdge(vertex_4, 0, 1, api::getOutputInterface(subsubgraph, 0), 0, 1);
    spider::api::createEdge(subsubgraph, 0, 1, output, 0, 1);
    spider::api::createEdge(subgraph, 0, 1, vertex_3, 0, 1);
    auto *edge = spider::api::createEdge(vertex_2, 1, 1, vertex_2, 1, 1);
    spider::api::createLocalDelay(edge, "1");
    spider::Runtime *runtime;
    switch (type) {
        case spider::RuntimeType::JITMS:
            runtime = spider::make<spider::JITMSRuntime>(StackID::GENERAL, graph, algorithm, allocatorType);
            break;
        case spider::RuntimeType::FAST_JITMS:
            runtime = spider::make<spider::FastJITMSRuntime>(StackID::GENERAL, graph, algorithm, allocatorType);
            break;
        default:
            runtime = nullptr;
            break;
    }
    try {
        if (!runtime) {
            throw std::runtime_error("failed to create runtime.");
        }
        for (size_t i = 0; i < 10U && !spider2StopRunning; ++i) {
            runtime->execute();
        }
    } catch (spider::Exception &e) {
        throw std::runtime_error(e.what());
    }
    destroy(runtime);
    api::destroyGraph(graph);
}

void spider::test::runtimeStaticFlatNoExec(spider::RuntimeType type, spider::SchedulingPolicy algorithm,
                                           spider::FifoAllocatorType allocatorType) {
    auto *graph = spider::api::createGraph("topgraph", 1, 0, 0);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 2);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 2, 0);
    auto *vertex_2 = spider::api::createVertex(graph, "vertex_2", 1, 1);
    spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 1);
    spider::api::createEdge(vertex_0, 1, 0, vertex_2, 0, 0);
    spider::api::createEdge(vertex_2, 0, 0, vertex_1, 1, 0);
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
    spider::Runtime *runtime;
    switch (type) {
        case spider::RuntimeType::JITMS:
            runtime = spider::make<spider::JITMSRuntime>(StackID::GENERAL, graph, algorithm, allocatorType);
            break;
        case spider::RuntimeType::FAST_JITMS:
            runtime = spider::make<spider::FastJITMSRuntime>(StackID::GENERAL, graph, algorithm, allocatorType);
            break;
        default:
            runtime = nullptr;
            break;
    }
    try {
        if (!runtime) {
            throw std::runtime_error("failed to create runtime.");
        }
        for (size_t i = 0; i < 10U && !spider2StopRunning; ++i) {
            runtime->execute();
        }
    } catch (spider::Exception &e) {
        throw std::runtime_error(e.what());
    }
    destroy(runtime);
    api::destroyGraph(graph);
}

void spider::test::runtimeStaticHierarchicalNoExec(spider::RuntimeType type, spider::SchedulingPolicy algorithm,
                                                   spider::FifoAllocatorType allocatorType) {
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
                                     [](const int64_t *, int64_t *, void *[], void *out[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(out[0]);
                                         buffer[0] = 3;
                                         spider::log::info("vertex_0:0 writing: 3..\n");
                                     });

    spider::api::createRuntimeKernel(vertex_1,
                                     [](const int64_t *, int64_t *, void *in[], void *[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(in[0]);
                                         spider::log::info("vertex_1 reading %d\n", buffer[0]);
                                     });

    spider::api::createRuntimeKernel(vertex_2,
                                     [](const int64_t *, int64_t *, void *in[], void *[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(in[0]);
                                         spider::log::info("vertex_2 reading %d\n", buffer[0]);
                                     });
    spider::Runtime *runtime;
    switch (type) {
        case spider::RuntimeType::JITMS:
            runtime = spider::make<spider::JITMSRuntime>(StackID::GENERAL, graph, algorithm, allocatorType);
            break;
        case spider::RuntimeType::FAST_JITMS:
            runtime = spider::make<spider::FastJITMSRuntime>(StackID::GENERAL, graph, algorithm, allocatorType);
            break;
        default:
            runtime = nullptr;
            break;
    }
    try {
        if (!runtime) {
            throw std::runtime_error("failed to create runtime.");
        }
        for (size_t i = 0; i < 10U && !spider2StopRunning; ++i) {
            runtime->execute();
        }
    } catch (spider::Exception &e) {
        throw std::runtime_error(e.what());
    }
    destroy(runtime);
    api::destroyGraph(graph);
}

void spider::test::runtimeDynamicHierarchical(spider::RuntimeType type, spider::SchedulingPolicy algorithm,
                                              spider::FifoAllocatorType allocatorType) {
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
                                     [](const int64_t *, int64_t *, void *[], void *out[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(out[0]);
                                         buffer[0] = 78;
//                                             spider::printer::printf("vertex_0 writing: %d..\n", buffer[0]);
                                     });

    spider::api::createRuntimeKernel(vertex_1,
                                     [](const int64_t *, int64_t *, void *[], void *out[]) -> void {
                                         auto *buffer = reinterpret_cast<char *>(out[0]);
//                                             spider::printer::printf("vertex_1 reading %d..\n",
//                                                                     reinterpret_cast<char *>(input[0])[0]);
                                         buffer[0] = 1;
//                                             buffer[1] = 2;
//                                             spider::printer::printf("vertex_1 writing %d..\n", buffer[0]);
//                                             spider::printer::printf("vertex_1 writing %c..\n", buffer[1]);
                                     });

    spider::api::createRuntimeKernel(width_setter,
                                     [](const int64_t *, int64_t *out, void *[], void *[]) -> void {
                                         static int64_t i = 10;
                                         out[0] = i;
                                         spider::printer::printf("width_setter: setting value: %" PRId64".\n",
                                                                 out[0]);
                                     });

    spider::api::createRuntimeKernel(sub_setter,
                                     [](const int64_t *, int64_t *out, void *[], void *[]) -> void {
                                         static int64_t i = 1;
                                         out[0] = i;
//                                             ++i;
                                         spider::printer::printf("sub_setter: setting value: %" PRId64".\n",
                                                                 out[0]);
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
    auto inherited_width = spider::api::createInheritedParam(subsubgraph, "width", width);
    auto width_derived = spider::api::createDerivedParam(subsubgraph, "width_derived", "width * sub_width");

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
    spider::Runtime *runtime;
    switch (type) {
        case spider::RuntimeType::JITMS:
            runtime = spider::make<spider::JITMSRuntime>(StackID::GENERAL, graph, algorithm, allocatorType);
            break;
        case spider::RuntimeType::FAST_JITMS:
            runtime = spider::make<spider::FastJITMSRuntime>(StackID::GENERAL, graph, algorithm, allocatorType);
            break;
        default:
            runtime = nullptr;
            break;
    }
    try {
        if (!runtime) {
            throw std::runtime_error("failed to create runtime.");
        }
        for (size_t i = 0; i < 10U && !spider2StopRunning; ++i) {
            runtime->execute();
        }
    } catch (spider::Exception &e) {
        throw std::runtime_error(e.what());
    }
    destroy(runtime);
    api::destroyGraph(graph);
}
