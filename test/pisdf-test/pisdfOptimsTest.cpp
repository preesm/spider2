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
#include <memory/alloc.h>
#include <graphs/pisdf/Types.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <api/spider.h>
#include <graphs-tools/transformation/optims/PiSDFGraphOptimizer.h>

class pisdfOptimsTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::createStackAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::GENERAL, "alloc-test");
        spider::createStackAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::EXPRESSION,
                                     "alloc-test");
        spider::createStackAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::PISDF, "alloc-test");
        spider::createStackAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::TRANSFO, "alloc-test");
        spider::api::enableVerbose();
        spider::api::enableLogger(spider::log::OPTIMS);
    }

    void TearDown() override {
        spider::api::disableVerbose();
        spider::api::disableLogger(spider::log::OPTIMS);
        spider::quit();
    }
};


TEST_F(pisdfOptimsTest, initEndTest) {
    /* == == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *init = spider::api::createInit(graph, "init");
        auto *end = spider::api::createEnd(graph, "end");
        spider::api::createEdge(init, 0, 1, end, 0, 1);
        ASSERT_EQ(graph->vertexCount(), 2);
        ASSERT_NO_THROW(PiSDFInitEndOptimizer()(graph));
        ASSERT_EQ(graph->vertexCount(), 0);
        spider::destroy(graph);
    }
    /* == == */
    {
        auto *graph = spider::api::createGraph("graph", 4, 1);
        auto *init = spider::api::createInit(graph, "init");
        auto *end = spider::api::createEnd(graph, "end");
        auto *v = spider::api::createVertex(graph, "v", 1);
        auto *v1 = spider::api::createVertex(graph, "v1", 0, 1);
        spider::api::createEdge(init, 0, 1, v, 0, 1);
        spider::api::createEdge(v1, 0, 1, end, 0, 1);
        ASSERT_EQ(graph->vertexCount(), 4);
        ASSERT_NO_THROW(PiSDFInitEndOptimizer()(graph));
        ASSERT_EQ(graph->vertexCount(), 4);
        spider::destroy(graph);
    }
}

TEST_F(pisdfOptimsTest, allOptimTest) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *init = spider::api::createInit(graph, "init");
    auto *end = spider::api::createEnd(graph, "end");
    auto *v = spider::api::createVertex(graph, "v", 0, 1);
    auto *v1 = spider::api::createVertex(graph, "v1", 1);
    auto *fork = spider::api::createFork(graph, "fork", 2);
    auto *join = spider::api::createJoin(graph, "join", 2);
    auto *head = spider::api::createHead(graph, "head", 1);
    auto *tail = spider::api::createTail(graph, "tail", 1);
    auto *repeat = spider::api::createRepeat(graph, "repeat");
    auto *duplicate = spider::api::createDuplicate(graph, "duplicate", 1);


    spider::api::createEdge(v, 0, 2, fork, 0, 2);
    spider::api::createEdge(fork, 0, 1, head, 0, 1);
    spider::api::createEdge(fork, 1, 1, tail, 0, 1);
    spider::api::createEdge(head, 0, 1, join, 0, 1);
    spider::api::createEdge(tail, 0, 1, join, 1, 1);
    spider::api::createEdge(join, 0, 1, duplicate, 0, 1);
    spider::api::createEdge(duplicate, 0, 1, repeat, 0, 1);
    spider::api::createEdge(repeat, 0, 1, v1, 0, 1);
    spider::api::createEdge(init, 0, 1, end, 0, 1);

    ASSERT_NO_THROW(PiSDFGraphOptimizer()(graph));

    spider::destroy(graph);
}

TEST_F(pisdfOptimsTest, unitaryTest) {
    /* == Testing unitary fork == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *v = spider::api::createVertex(graph, "v", 0, 1);
        auto *fork = spider::api::createFork(graph, "fork", 1);
        auto *v1 = spider::api::createVertex(graph, "v1", 1);
        spider::api::createEdge(v, 0, 1, fork, 0, 1);
        spider::api::createEdge(fork, 0, 1, v1, 0, 1);
        ASSERT_EQ(graph->vertexCount(), 3);
        ASSERT_NO_THROW(PiSDFUnitaryOptimizer()(graph));
        ASSERT_EQ(graph->vertexCount(), 2);
        spider::destroy(graph);
    }
    /* == Testing unitary join == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *v = spider::api::createVertex(graph, "v", 0, 1);
        auto *join = spider::api::createJoin(graph, "join", 1);
        auto *v1 = spider::api::createVertex(graph, "v1", 1);
        spider::api::createEdge(v, 0, 1, join, 0, 1);
        spider::api::createEdge(join, 0, 1, v1, 0, 1);
        ASSERT_EQ(graph->vertexCount(), 3);
        ASSERT_NO_THROW(PiSDFUnitaryOptimizer()(graph));
        ASSERT_EQ(graph->vertexCount(), 2);
        spider::destroy(graph);
    }
    /* == Testing unitary head == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *v = spider::api::createVertex(graph, "v", 0, 1);
        auto *head = spider::api::createHead(graph, "head", 1);
        auto *v1 = spider::api::createVertex(graph, "v1", 1);
        spider::api::createEdge(v, 0, 1, head, 0, 1);
        spider::api::createEdge(head, 0, 1, v1, 0, 1);
        ASSERT_EQ(graph->vertexCount(), 3);
        ASSERT_NO_THROW(PiSDFUnitaryOptimizer()(graph));
        ASSERT_EQ(graph->vertexCount(), 2);
        spider::destroy(graph);
    }
    /* == Testing unitary tail == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *v = spider::api::createVertex(graph, "v", 0, 1);
        auto *tail = spider::api::createTail(graph, "tail", 1);
        auto *v1 = spider::api::createVertex(graph, "v1", 1);
        spider::api::createEdge(v, 0, 1, tail, 0, 1);
        spider::api::createEdge(tail, 0, 1, v1, 0, 1);
        ASSERT_EQ(graph->vertexCount(), 3);
        ASSERT_NO_THROW(PiSDFUnitaryOptimizer()(graph));
        ASSERT_EQ(graph->vertexCount(), 2);
        spider::destroy(graph);
    }
    /* == Testing unitary duplicate == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *v = spider::api::createVertex(graph, "v", 0, 1);
        auto *duplicate = spider::api::createDuplicate(graph, "duplicate", 1);
        auto *v1 = spider::api::createVertex(graph, "v1", 1);
        spider::api::createEdge(v, 0, 1, duplicate, 0, 1);
        spider::api::createEdge(duplicate, 0, 1, v1, 0, 1);
        ASSERT_EQ(graph->vertexCount(), 3);
        ASSERT_NO_THROW(PiSDFUnitaryOptimizer()(graph));
        ASSERT_EQ(graph->vertexCount(), 2);
        spider::destroy(graph);
    }
    /* == Testing unitary repeat == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *v = spider::api::createVertex(graph, "v", 0, 1);
        auto *repeat = spider::api::createRepeat(graph, "repeat");
        auto *v1 = spider::api::createVertex(graph, "v1", 1);
        spider::api::createEdge(v, 0, 1, repeat, 0, 1);
        spider::api::createEdge(repeat, 0, 1, v1, 0, 1);
        ASSERT_EQ(graph->vertexCount(), 3);
        ASSERT_NO_THROW(PiSDFUnitaryOptimizer()(graph));
        ASSERT_EQ(graph->vertexCount(), 2);
        spider::destroy(graph);
    }
}

TEST_F(pisdfOptimsTest, forkForkTest) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *v = spider::api::createVertex(graph, "v", 0, 1);
    auto *v1 = spider::api::createVertex(graph, "v1", 1);
    auto *v2 = spider::api::createVertex(graph, "v2", 1);
    auto *v3 = spider::api::createVertex(graph, "v3", 1);
    auto *v4 = spider::api::createVertex(graph, "v4", 1);
    auto *v5 = spider::api::createVertex(graph, "v5", 1);
    auto *fork = spider::api::createFork(graph, "fork", 3);
    auto *fork_0 = spider::api::createFork(graph, "fork_0", 2);
    auto *fork_1 = spider::api::createFork(graph, "fork_1", 2);
    spider::api::createEdge(v, 0, 5, fork, 0, 5);
    spider::api::createEdge(fork, 0, 1, v1, 0, 1);
    spider::api::createEdge(fork, 1, 3, fork_0, 0, 3);
    spider::api::createEdge(fork, 2, 1, v5, 0, 1);
    spider::api::createEdge(fork_0, 0, 1, v2, 0, 1);
    spider::api::createEdge(fork_0, 1, 2, fork_1, 0, 2);
    spider::api::createEdge(fork_1, 0, 1, v3, 0, 1);
    spider::api::createEdge(fork_1, 1, 1, v4, 0, 1);
    ASSERT_EQ(graph->vertexCount(), 9);
    ASSERT_NO_THROW(PiSDFForkForkOptimizer()(graph));
    ASSERT_EQ(graph->vertexCount(), 7);
    spider::destroy(graph);
}

TEST_F(pisdfOptimsTest, forkForkTest2) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *v = spider::api::createVertex(graph, "v", 0, 1);
    auto *v1 = spider::api::createVertex(graph, "v1", 1);
    auto *v2 = spider::api::createVertex(graph, "v2", 1);
    auto *v3 = spider::api::createVertex(graph, "v3", 1);
    auto *v4 = spider::api::createVertex(graph, "v4", 1);
    auto *v5 = spider::api::createVertex(graph, "v5", 1);
    auto *v6 = spider::api::createVertex(graph, "v6", 1);
    auto *fork_1 = spider::api::createFork(graph, "fork_1", 2);
    auto *fork_0 = spider::api::createFork(graph, "fork_0", 3);
    auto *fork = spider::api::createFork(graph, "fork", 2);
    auto *fork_2 = spider::api::createFork(graph, "fork_2", 2);
    spider::api::createEdge(v, 0, 6, fork, 0, 6);
    spider::api::createEdge(fork, 0, 5, fork_0, 0, 5);
    spider::api::createEdge(fork, 1, 1, v1, 0, 1);
    spider::api::createEdge(fork_0, 0, 2, fork_1, 0, 2);
    spider::api::createEdge(fork_0, 1, 1, v2, 0, 1);
    spider::api::createEdge(fork_0, 2, 2, fork_2, 0, 2);
    spider::api::createEdge(fork_1, 0, 1, v3, 0, 1);
    spider::api::createEdge(fork_1, 1, 1, v4, 0, 1);
    spider::api::createEdge(fork_2, 0, 1, v5, 0, 1);
    spider::api::createEdge(fork_2, 1, 1, v6, 0, 1);
    ASSERT_EQ(graph->vertexCount(), 11);
    spider::api::exportGraphToDOT("./test_before.dot", graph);
    ASSERT_NO_THROW(PiSDFForkForkOptimizer()(graph));
    spider::api::exportGraphToDOT("./test_after.dot", graph);
    ASSERT_EQ(graph->vertexCount(), 8);
    spider::destroy(graph);
}

TEST_F(pisdfOptimsTest, joinForkTest) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *v = spider::api::createVertex(graph, "v", 0, 1);
    auto *v1 = spider::api::createVertex(graph, "v1", 0, 1);
    auto *v2 = spider::api::createVertex(graph, "v2", 1);
    auto *v3 = spider::api::createVertex(graph, "v3", 1);
    auto *v4 = spider::api::createVertex(graph, "v4", 0, 1);
    auto *v5 = spider::api::createVertex(graph, "v5", 1);
    auto *v6 = spider::api::createVertex(graph, "v6", 1);
    auto *fork_0 = spider::api::createFork(graph, "fork_0", 3);
    auto *fork_1 = spider::api::createFork(graph, "fork_1", 2);
    auto *join_0 = spider::api::createJoin(graph, "join_0", 2);
    auto *join_1 = spider::api::createJoin(graph, "join_1", 2);
    spider::api::createEdge(v, 0, 1, join_0, 0, 1);
    spider::api::createEdge(v1, 0, 2, join_0, 1, 2);
    spider::api::createEdge(join_0, 0, 3, fork_0, 0, 3);
    spider::api::createEdge(fork_0, 0, 1, v2, 0, 1);
    spider::api::createEdge(fork_0, 1, 1, join_1, 0, 1);
    spider::api::createEdge(fork_0, 2, 1, v3, 0, 1);
    spider::api::createEdge(v4, 0, 1, join_1, 1, 1);
    spider::api::createEdge(join_1, 0, 2, fork_1, 0, 2);
    spider::api::createEdge(fork_1, 0, 1, v5, 0, 1);
    spider::api::createEdge(fork_1, 1, 1, v6, 0, 1);
    ASSERT_EQ(graph->vertexCount(), 11);
    ASSERT_NO_THROW(PiSDFJoinForkOptimizer()(graph));
    ASSERT_EQ(graph->vertexCount(), 8);
    spider::destroy(graph);
}

TEST_F(pisdfOptimsTest, joinForkTest2) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *v = spider::api::createVertex(graph, "v", 0, 1);
    auto *v1 = spider::api::createVertex(graph, "v1", 0, 1);
    auto *v2 = spider::api::createVertex(graph, "v2", 0, 1);
    auto *v3 = spider::api::createVertex(graph, "v3", 1);
    auto *v4 = spider::api::createVertex(graph, "v4", 1);
    auto *fork_0 = spider::api::createFork(graph, "fork_0", 2);
    auto *join_0 = spider::api::createJoin(graph, "join_0", 3);
    spider::api::createEdge(v, 0, 1, join_0, 0, 1);
    spider::api::createEdge(v1, 0, 1, join_0, 1, 1);
    spider::api::createEdge(v2, 0, 1, join_0, 2, 1);
    spider::api::createEdge(join_0, 0, 3, fork_0, 0, 3);
    spider::api::createEdge(fork_0, 0, 2, v3, 0, 2);
    spider::api::createEdge(fork_0, 1, 1, v4, 0, 1);
    ASSERT_EQ(graph->vertexCount(), 7);
    ASSERT_NO_THROW(PiSDFJoinForkOptimizer()(graph));
    ASSERT_EQ(graph->vertexCount(), 6);
    spider::destroy(graph);
}

TEST_F(pisdfOptimsTest, joinForkTest3) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *v = spider::api::createVertex(graph, "v", 0, 1);
    auto *v1 = spider::api::createVertex(graph, "v1", 0, 1);
    auto *v2 = spider::api::createVertex(graph, "v2", 1);
    auto *v3 = spider::api::createVertex(graph, "v3", 1);
    auto *v4 = spider::api::createVertex(graph, "v4", 0, 1);
    auto *v5 = spider::api::createVertex(graph, "v5", 1);
    auto *fork_0 = spider::api::createFork(graph, "fork_0", 3);
    auto *join_0 = spider::api::createJoin(graph, "join_0", 3);
    spider::api::createEdge(v, 0, 2, join_0, 0, 2);
    spider::api::createEdge(v1, 0, 8, join_0, 1, 8);
    spider::api::createEdge(v4, 0, 3, join_0, 2, 3);
    spider::api::createEdge(join_0, 0, 13, fork_0, 0, 13);
    spider::api::createEdge(fork_0, 0, 2, v2, 0, 2);
    spider::api::createEdge(fork_0, 1, 6, v3, 0, 6);
    spider::api::createEdge(fork_0, 2, 2, v5, 0, 5);
    ASSERT_EQ(graph->vertexCount(), 8);
    ASSERT_NO_THROW(PiSDFJoinForkOptimizer()(graph));
    ASSERT_EQ(graph->vertexCount(), 8);
    spider::destroy(graph);
}

TEST_F(pisdfOptimsTest, joinForkTest4) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *v = spider::api::createVertex(graph, "v", 0, 1);
    auto *v1 = spider::api::createVertex(graph, "v1", 0, 1);
    auto *v2 = spider::api::createVertex(graph, "v2", 1);
    auto *v3 = spider::api::createVertex(graph, "v3", 1);
    auto *fork_0 = spider::api::createFork(graph, "fork_0", 2);
    auto *join_0 = spider::api::createJoin(graph, "join_0", 2);
    spider::api::createEdge(v, 0, 4, join_0, 0, 4);
    spider::api::createEdge(v1, 0, 3, join_0, 1, 3);
    spider::api::createEdge(join_0, 0, 7, fork_0, 0, 7);
    spider::api::createEdge(fork_0, 0, 5, v2, 0, 5);
    spider::api::createEdge(fork_0, 1, 2, v3, 0, 2);
    ASSERT_EQ(graph->vertexCount(), 6);
    ASSERT_NO_THROW(PiSDFJoinForkOptimizer()(graph));
    ASSERT_EQ(graph->vertexCount(), 6);
    spider::destroy(graph);
}

TEST_F(pisdfOptimsTest, joinJoinTest) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *v = spider::api::createVertex(graph, "v", 0, 1);
    auto *v1 = spider::api::createVertex(graph, "v1", 0, 1);
    auto *v2 = spider::api::createVertex(graph, "v2", 0, 1);
    auto *join_1 = spider::api::createJoin(graph, "join_1", 3);
    auto *v3 = spider::api::createVertex(graph, "v3", 0, 1);
    auto *join = spider::api::createJoin(graph, "join", 2);
    auto *v4 = spider::api::createVertex(graph, "v4", 0, 1);
    auto *join_0 = spider::api::createJoin(graph, "join_0", 2);
    auto *v5 = spider::api::createVertex(graph, "v5", 1, 0);
    spider::api::createEdge(v1, 0, 1, join, 0, 1);
    spider::api::createEdge(v2, 0, 1, join, 1, 1);
    spider::api::createEdge(join, 0, 2, join_0, 0, 2);
    spider::api::createEdge(v3, 0, 1, join_0, 1, 1);
    spider::api::createEdge(v, 0, 1, join_1, 0, 1);
    spider::api::createEdge(join_0, 0, 3, join_1, 1, 3);
    spider::api::createEdge(v4, 0, 1, join_1, 2, 1);
    spider::api::createEdge(join_1, 0, 5, v5, 0, 5);
    ASSERT_EQ(graph->vertexCount(), 9);
    ASSERT_NO_THROW(PiSDFJoinJoinOptimizer()(graph));
    ASSERT_EQ(graph->vertexCount(), 7);
    spider::destroy(graph);
}

TEST_F(pisdfOptimsTest, joinJoinTest2) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *v = spider::api::createVertex(graph, "v", 0, 1);
    auto *v1 = spider::api::createVertex(graph, "v1", 0, 1);
    auto *v2 = spider::api::createVertex(graph, "v2", 0, 1);
    auto *v3 = spider::api::createVertex(graph, "v3", 0, 1);
    auto *v4 = spider::api::createVertex(graph, "v4", 0, 1);
    auto *v5 = spider::api::createVertex(graph, "v5", 0, 1);
    auto *v6 = spider::api::createVertex(graph, "v6", 1);
    auto *join = spider::api::createJoin(graph, "join", 2);
    auto *join_0 = spider::api::createJoin(graph, "join_0", 2);
    auto *join_1 = spider::api::createJoin(graph, "join_1", 3);
    auto *join_2 = spider::api::createJoin(graph, "join_2", 2);
    spider::api::createEdge(v, 0, 1, join, 0, 1);
    spider::api::createEdge(v1, 0, 1, join, 1, 1);
    spider::api::createEdge(v2, 0, 1, join_0, 0, 1);
    spider::api::createEdge(join, 0, 2, join_0, 1, 2);
    spider::api::createEdge(v3, 0, 1, join_2, 0, 1);
    spider::api::createEdge(v4, 0, 1, join_2, 1, 1);
    spider::api::createEdge(v5, 0, 1, join_1, 0, 1);
    spider::api::createEdge(join_0, 0, 3, join_1, 1, 3);
    spider::api::createEdge(join_2, 0, 2, join_1, 2, 2);
    spider::api::createEdge(join_1, 0, 6, v6, 0, 6);
    ASSERT_EQ(graph->vertexCount(), 11);
    ASSERT_NO_THROW(PiSDFJoinJoinOptimizer()(graph));
    ASSERT_EQ(graph->vertexCount(), 8);
    spider::destroy(graph);
}

TEST_F(pisdfOptimsTest, joinEndTest) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *v = spider::api::createVertex(graph, "v", 0, 1);
    auto *v1 = spider::api::createVertex(graph, "v1", 0, 1);
    auto *v2 = spider::api::createVertex(graph, "v2", 0, 1);
    auto *join = spider::api::createJoin(graph, "join", 3);
    auto *end = spider::api::createEnd(graph, "end");
    spider::api::createEdge(v, 0, 1, join, 0, 1);
    spider::api::createEdge(v1, 0, 1, join, 1, 1);
    spider::api::createEdge(v2, 0, 1, join, 2, 1);
    spider::api::createEdge(join, 0, 3, end, 0, 3);
    ASSERT_EQ(graph->vertexCount(), 5);
    ASSERT_NO_THROW(PiSDFJoinEndOptimizer()(graph));
    ASSERT_EQ(graph->vertexCount(), 6);
    spider::destroy(graph);
}