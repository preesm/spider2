/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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

#include <gtest/gtest.h>
#include <common/Exception.h>
#include <memory/dynamic-policies/GenericAllocatorPolicy.h>
#include <api/spider.h>
#include <graphs/pisdf/Graph.h>
#include <scheduling/scheduler/Scheduler.h>
#include <runtime/algorithm/srdag-based/SRDAGJITMSRuntime.h>
#include "appTest/stabilization/spider2-stabilization.h"
#include "appTest/reinforcement/spider2-reinforcement.h"

extern bool spider2StopRunning;

constexpr auto LOOP_COUNT = 5U;

/* === Function(s) definition === */

/* === Constants declaration === */

constexpr size_t CLUSTER_COUNT = 1;

constexpr size_t PE_COUNT = 1;

enum HardwareType : uint32_t {
    TYPE_X86,
};

enum HardwareID : uint32_t {
    PE_X86_CORE0,
};

class runtimeAppTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::start();
        /* == Creates the main platform == */
        spider::api::createPlatform(CLUSTER_COUNT, PE_COUNT);
        /* == Creates the intra MemoryInterface of the cluster == */
        auto *x86MemoryInterface = spider::api::createMemoryInterface(1073741824);
        /* == Creates the actual Cluster == */
        auto *x86Cluster = spider::api::createCluster(1, x86MemoryInterface);
        /* == Creates the processing element(s) of the cluster == */
        auto *x86Core0 = spider::api::createProcessingElement(TYPE_X86, PE_X86_CORE0, x86Cluster, "Core0", spider::PEType::LRT, 0);
        /* == Set the GRT == */
        spider::api::setSpiderGRTPE(x86Core0);
        /* == Creates the runtime platform == */
        spider::api::createThreadRTPlatform();
    }

    void TearDown() override {
        spider::quit();
    }
};

TEST_F(runtimeAppTest, TestStabilization) {
    auto *graph = spider::stab::createStabilization();
    spider::stab::createUserApplicationKernels();
    auto context = spider::createRuntimeContext(graph, spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::SRDAG_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::BEST_FIT,
            spider::FifoAllocatorType::DEFAULT,
            LOOP_COUNT,
    });
    ASSERT_NO_THROW(spider::run(context));
    spider::destroyRuntimeContext(context);
    spider::api::destroyGraph(graph);
}

TEST_F(runtimeAppTest, TestStabilizationJIT) {
    auto *graph = spider::stab::createStabilization();
    spider::stab::createUserApplicationKernels();
    auto context = spider::createRuntimeContext(graph, spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::SRDAG_BASED,
            spider::ExecutionPolicy::JIT,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::BEST_FIT,
            spider::FifoAllocatorType::DEFAULT,
            LOOP_COUNT,
    });
    ASSERT_NO_THROW(spider::run(context));
    spider::destroyRuntimeContext(context);
    spider::api::destroyGraph(graph);
}

TEST_F(runtimeAppTest, TestStabilizationSRLess) {
    auto *graph = spider::stab::createStabilization();
    spider::stab::createUserApplicationKernels();
    auto context = spider::createRuntimeContext(graph, spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::BEST_FIT,
            spider::FifoAllocatorType::DEFAULT,
            LOOP_COUNT,
    });
    ASSERT_NO_THROW(spider::run(context));
    spider::destroyRuntimeContext(context);
    spider::api::destroyGraph(graph);
}

TEST_F(runtimeAppTest, TestStabilizationSRLessJIT) {
    auto *graph = spider::stab::createStabilization();
    spider::stab::createUserApplicationKernels();
    auto context = spider::createRuntimeContext(graph, spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::JIT,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::BEST_FIT,
            spider::FifoAllocatorType::DEFAULT,
            LOOP_COUNT,
    });
    ASSERT_NO_THROW(spider::run(context));
    spider::destroyRuntimeContext(context);
    spider::api::destroyGraph(graph);
}

TEST_F(runtimeAppTest, TestStabilizationNoSync) {
    auto *graph = spider::stab::createStabilization();
    spider::stab::createUserApplicationKernels();
    auto context = spider::createRuntimeContext(graph, spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::SRDAG_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::BEST_FIT,
            spider::FifoAllocatorType::DEFAULT_NOSYNC,
            LOOP_COUNT,
    });
    ASSERT_NO_THROW(spider::run(context));
    spider::destroyRuntimeContext(context);
    spider::api::destroyGraph(graph);
}

TEST_F(runtimeAppTest, TestReinforcement) {
    auto *graph = spider::rl::createReinforcementLearning();
    spider::rl::createUserApplicationKernels();
    auto context = spider::createRuntimeContext(graph, spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::SRDAG_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::BEST_FIT,
            spider::FifoAllocatorType::DEFAULT,
            LOOP_COUNT,
    });
    ASSERT_NO_THROW(spider::run(context));
    spider::destroyRuntimeContext(context);
    spider::api::destroyGraph(graph);
}

TEST_F(runtimeAppTest, TestReinforcementJIT) {
    auto *graph = spider::rl::createReinforcementLearning();
    spider::rl::createUserApplicationKernels();
    auto context = spider::createRuntimeContext(graph, spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::SRDAG_BASED,
            spider::ExecutionPolicy::JIT,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::BEST_FIT,
            spider::FifoAllocatorType::DEFAULT,
            LOOP_COUNT,
    });
    ASSERT_NO_THROW(spider::run(context));
    spider::destroyRuntimeContext(context);
    spider::api::destroyGraph(graph);
}

TEST_F(runtimeAppTest, TestReinforcementSRLess) {
    auto *graph = spider::rl::createReinforcementLearning();
    spider::rl::createUserApplicationKernels();
    auto context = spider::createRuntimeContext(graph, spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::BEST_FIT,
            spider::FifoAllocatorType::DEFAULT,
            LOOP_COUNT,
    });
    ASSERT_NO_THROW(spider::run(context));
    spider::destroyRuntimeContext(context);
    spider::api::destroyGraph(graph);
}

TEST_F(runtimeAppTest, TestReinforcementSRLessJIT) {
    auto *graph = spider::rl::createReinforcementLearning();
    spider::rl::createUserApplicationKernels();
    auto context = spider::createRuntimeContext(graph, spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::JIT,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::BEST_FIT,
            spider::FifoAllocatorType::DEFAULT,
            LOOP_COUNT,
    });
    ASSERT_NO_THROW(spider::run(context));
    spider::destroyRuntimeContext(context);
    spider::api::destroyGraph(graph);
}

TEST_F(runtimeAppTest, TestReinforcementNoSync) {
    auto *graph = spider::rl::createReinforcementLearning();
    spider::rl::createUserApplicationKernels();
    auto context = spider::createRuntimeContext(graph, spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::SRDAG_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::BEST_FIT,
            spider::FifoAllocatorType::DEFAULT_NOSYNC,
            LOOP_COUNT,
    });
    ASSERT_NO_THROW(spider::run(context));
    spider::destroyRuntimeContext(context);
    spider::api::destroyGraph(graph);
}