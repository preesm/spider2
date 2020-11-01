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

#include <gtest/gtest.h>
#include <common/Exception.h>
#include <memory/allocator.h>
#include <memory/memory.h>
#include <memory/dynamic-policies/FreeListAllocatorPolicy.h>
#include <memory/dynamic-policies/GenericAllocatorPolicy.h>
#include <memory/static-policies/LinearStaticAllocator.h>
#include <api/spider.h>
#include "RuntimeTestCases.h"

class runtimeMonoTestPiSDFRR : public ::testing::Test {
protected:
    void SetUp() override {
        spider::start();
        spider::api::enableExportSRDAG();
        spider::api::enableExportGantt();
        /* == Create the mono core platform == */
        spider::api::createPlatform(1, 1);
        auto *memoryInterface = spider::api::createMemoryInterface(1024 * 1024 * 1024);
        auto *cluster = spider::api::createCluster(1, memoryInterface);
        auto core = spider::api::createProcessingElement(0, 0, cluster, "Core0", spider::PEType::LRT, 0);
        spider::api::setSpiderGRTPE(core);
    }

    void TearDown() override {
        spider::quit();
    }
};

TEST_F(runtimeMonoTestPiSDFRR, TestStaticFlat) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticFlat(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestStaticFlatNoSync) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT_NOSYNC,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticFlat(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestStaticNoExecFlat) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticFlatNoExec(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestStaticNoExecFlatNoSync) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT_NOSYNC,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticFlatNoExec(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestStaticHierarchical) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticHierarchical(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestStaticHierarchicalNoSync) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT_NOSYNC,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticHierarchical(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestStaticNoExecHierarchical) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticHierarchicalNoExec(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestStaticNoExecHierarchicalNoSync) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT_NOSYNC,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticHierarchicalNoExec(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestDynamicHierarchical) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeDynamicHierarchical(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestDynamicHierarchicalNoSync) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::LIST,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT_NOSYNC,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeDynamicHierarchical(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestGreedyStaticFlat) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::GREEDY,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticFlat(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestGreedyStaticFlatNoSync) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::GREEDY,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT_NOSYNC,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticFlat(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestGreedyStaticNoExecFlat) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::GREEDY,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticFlatNoExec(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestGreedyStaticNoExecFlatNoSync) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::GREEDY,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT_NOSYNC,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticFlatNoExec(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestGreedyStaticHierarchical) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::GREEDY,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticHierarchical(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestGreedyStaticHierarchicalNoSync) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::GREEDY,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT_NOSYNC,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticHierarchical(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestGreedyStaticNoExecHierarchical) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::GREEDY,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticHierarchicalNoExec(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestGreedyStaticNoExecHierarchicalNoSync) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::GREEDY,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT_NOSYNC,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeStaticHierarchicalNoExec(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestGreedyDynamicHierarchical) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::GREEDY,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeDynamicHierarchical(runtimeConfig));
}

TEST_F(runtimeMonoTestPiSDFRR, TestGreedyDynamicHierarchicalNoSync) {
    const auto runtimeConfig = spider::RuntimeConfig{
            spider::RunMode::LOOP,
            spider::RuntimeType::PISDF_BASED,
            spider::ExecutionPolicy::DELAYED,
            spider::SchedulingPolicy::GREEDY,
            spider::MappingPolicy::ROUND_ROBIN,
            spider::FifoAllocatorType::DEFAULT_NOSYNC,
            10U,
    };
    ASSERT_NO_THROW(spider::test::runtimeDynamicHierarchical(runtimeConfig));
}

