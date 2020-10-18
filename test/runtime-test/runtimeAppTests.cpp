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
#include <memory/allocator.h>
#include <memory/memory.h>
#include <memory/dynamic-policies/FreeListAllocatorPolicy.h>
#include <memory/dynamic-policies/GenericAllocatorPolicy.h>
#include <memory/static-policies/LinearStaticAllocator.h>
#include <api/spider.h>
#include <graphs/pisdf/Graph.h>
#include <scheduling/scheduler/Scheduler.h>
#include <scheduling/scheduler/srdag-based/ListScheduler.h>
#include <runtime/algorithm/srdag-based/SRDAGJITMSRuntime.h>
#include "appTest/stabilization/spider2-application.h"

extern bool spider2StopRunning;

constexpr auto LOOP_COUNT = 5U;

/* === Function(s) definition === */

class runtimeAppTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::start();
        /* == Creates the architecture == */
        spider::createUserPhysicalPlatform();
        /* == Creates the application graph == */
        graph_ = spider::createUserApplicationGraph();
        /* == Creates the runtime platform == */
        spider::api::createThreadRTPlatform();
        /* == Creates the application kernels == */
        spider::createUserApplicationKernels();
    }

    void TearDown() override {
        /* == Destroy the graph == */
        spider::api::destroyGraph(graph_);
        spider::quit();
    }

    spider::pisdf::Graph *graph_;
};

TEST_F(runtimeAppTest, TestStabilization) {
    auto context = spider::createRuntimeContext(graph_, spider::RuntimeConfig{
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
}

TEST_F(runtimeAppTest, TestStabilizationJIT) {
    auto context = spider::createRuntimeContext(graph_, spider::RuntimeConfig{
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
}

TEST_F(runtimeAppTest, TestStabilizationSRLess) {
    auto context = spider::createRuntimeContext(graph_, spider::RuntimeConfig{
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
}

TEST_F(runtimeAppTest, TestStabilizationSRLessJIT) {
    auto context = spider::createRuntimeContext(graph_, spider::RuntimeConfig{
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
}

TEST_F(runtimeAppTest, TestStabilizationNoSync) {
    auto context = spider::createRuntimeContext(graph_, spider::RuntimeConfig{
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
}