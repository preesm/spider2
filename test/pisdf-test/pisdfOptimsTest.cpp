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
#include <memory/memory.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/srdag/SRDAGGraph.h>
#include <api/spider.h>
#include <graphs-tools/transformation/optims/optimizations.h>
#include <graphs-tools/exporter/SRDAGDOTExporter.h>

class pisdfOptimsTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::start();
        spider::api::enableVerbose();
        spider::api::enableLogger(spider::log::OPTIMS);
    }

    void TearDown() override {
        spider::api::disableVerbose();
        spider::api::disableLogger(spider::log::OPTIMS);
        spider::quit();
    }
};

TEST_F(pisdfOptimsTest, falseReturn) {
    ASSERT_NO_THROW(spider::optims::optimize(nullptr));
    ASSERT_EQ(spider::optims::reduceRepeatFork(nullptr), false);
    ASSERT_EQ(spider::optims::reduceJoinJoin(nullptr), false);
    ASSERT_EQ(spider::optims::reduceJoinFork(nullptr), false);
    ASSERT_EQ(spider::optims::reduceForkFork(nullptr), false);
    ASSERT_EQ(spider::optims::reduceJoinEnd(nullptr), false);
    ASSERT_EQ(spider::optims::reduceInitEnd(nullptr), false);
    ASSERT_EQ(spider::optims::reduceUnitaryRateActors(nullptr), false);
}

TEST_F(pisdfOptimsTest, initEndTest) {
    /* == == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
        auto *init = srdag->createInitVertex("init");
        auto *end = srdag->createEndVertex("end");
        srdag->createEdge(init, 0, end, 0, 1);
        ASSERT_EQ(srdag->vertexCount(), 2);
        ASSERT_NO_THROW(spider::optims::reduceInitEnd(srdag));
        ASSERT_EQ(srdag->vertexCount(), 0);
        spider::destroy(graph);
        spider::destroy(srdag);
    }
    /* == == */
    {
        auto *graph = spider::api::createGraph("graph", 4, 1);
        auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
        auto *init = srdag->createInitVertex("init");
        auto *end = srdag->createEndVertex("end");
        auto *v = srdag->createVertex("v", 1);
        auto *v1 = srdag->createVertex("v1", 0, 1);
        srdag->createEdge(init, 0, v, 0, 1);
        srdag->createEdge(v1, 0, end, 0, 1);
        ASSERT_EQ(srdag->vertexCount(), 4);
        ASSERT_NO_THROW(spider::optims::reduceInitEnd(srdag));
        ASSERT_EQ(srdag->vertexCount(), 4);
        spider::destroy(graph);
        spider::destroy(srdag);
    }
}

TEST_F(pisdfOptimsTest, allOptimTest) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    auto *init = srdag->createInitVertex("init");
    auto *end = srdag->createEndVertex("end");
    auto *v = srdag->createVertex("v", 0, 1);
    auto *v1 = srdag->createVertex("v1", 1);
    auto *fork = srdag->createForkVertex("fork", 2);
    auto *join = srdag->createJoinVertex("join", 2);
    auto *head = srdag->createHeadVertex("head", 1);
    auto *tail = srdag->createTailVertex("tail", 1);
    auto *repeat = srdag->createRepeatVertex("repeat");
    auto *duplicate = srdag->createDuplicateVertex("duplicate", 1);


    srdag->createEdge(v, 0, fork, 0, 2);
    srdag->createEdge(fork, 0, head, 0, 1);
    srdag->createEdge(fork, 1, tail, 0, 1);
    srdag->createEdge(head, 0, join, 0, 1);
    srdag->createEdge(tail, 0, join, 1, 1);
    srdag->createEdge(join, 0, duplicate, 0, 1);
    srdag->createEdge(duplicate, 0, repeat, 0, 1);
    srdag->createEdge(repeat, 0, v1, 0, 1);
    srdag->createEdge(init, 0, end, 0, 1);

    ASSERT_NO_THROW(spider::optims::optimize(srdag));

    spider::destroy(graph);
    spider::destroy(srdag);
}

TEST_F(pisdfOptimsTest, unitaryTest) {
    /* == Testing unitary fork == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
        auto *v = srdag->createVertex("v", 0, 1);
        auto *fork = srdag->createForkVertex("fork", 1);
        auto *v1 = srdag->createVertex("v1", 1);
        srdag->createEdge(v, 0, fork, 0, 1);
        srdag->createEdge(fork, 0, v1, 0, 1);
        ASSERT_EQ(srdag->vertexCount(), 3);
        ASSERT_NO_THROW(spider::optims::reduceUnitaryRateActors(srdag));
        ASSERT_EQ(srdag->vertexCount(), 2);
        spider::destroy(graph);
        spider::destroy(srdag);
    }
    /* == Testing unitary join == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
        auto *v = srdag->createVertex("v", 0, 1);
        auto *join = srdag->createJoinVertex("join", 1);
        auto *v1 = srdag->createVertex("v1", 1);
        srdag->createEdge(v, 0, join, 0, 1);
        srdag->createEdge(join, 0, v1, 0, 1);
        ASSERT_EQ(srdag->vertexCount(), 3);
        ASSERT_NO_THROW(spider::optims::reduceUnitaryRateActors(srdag));
        ASSERT_EQ(srdag->vertexCount(), 2);
        spider::destroy(graph);
        spider::destroy(srdag);
    }
    /* == Testing unitary head == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
        auto *v = srdag->createVertex("v", 0, 1);
        auto *head = srdag->createHeadVertex("head", 1);
        auto *v1 = srdag->createVertex("v1", 1);
        srdag->createEdge(v, 0, head, 0, 1);
        srdag->createEdge(head, 0, v1, 0, 1);
        ASSERT_EQ(srdag->vertexCount(), 3);
        ASSERT_NO_THROW(spider::optims::reduceUnitaryRateActors(srdag));
        ASSERT_EQ(srdag->vertexCount(), 2);
        spider::destroy(graph);
        spider::destroy(srdag);
    }
    /* == Testing unitary tail == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
        auto *v = srdag->createVertex("v", 0, 1);
        auto *tail = srdag->createTailVertex("tail", 1);
        auto *v1 = srdag->createVertex("v1", 1);
        srdag->createEdge(v, 0, tail, 0, 1);
        srdag->createEdge(tail, 0, v1, 0, 1);
        ASSERT_EQ(srdag->vertexCount(), 3);
        ASSERT_NO_THROW(spider::optims::reduceUnitaryRateActors(srdag));
        ASSERT_EQ(srdag->vertexCount(), 2);
        spider::destroy(graph);
        spider::destroy(srdag);
    }
    /* == Testing unitary duplicate == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
        auto *v = srdag->createVertex("v", 0, 1);
        auto *duplicate = srdag->createDuplicateVertex("duplicate", 1);
        auto *v1 = srdag->createVertex("v1", 1);
        srdag->createEdge(v, 0, duplicate, 0, 1);
        srdag->createEdge(duplicate, 0, v1, 0, 1);
        ASSERT_EQ(srdag->vertexCount(), 3);
        ASSERT_NO_THROW(spider::optims::reduceUnitaryRateActors(srdag));
        ASSERT_EQ(srdag->vertexCount(), 2);
        spider::destroy(graph);
        spider::destroy(srdag);
    }
    /* == Testing unitary repeat == */
    {
        auto *graph = spider::api::createGraph("graph", 2, 1);
        auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
        auto *v = srdag->createVertex("v", 0, 1);
        auto *repeat = srdag->createRepeatVertex("repeat");
        auto *v1 = srdag->createVertex("v1", 1);
        srdag->createEdge(v, 0, repeat, 0, 1);
        srdag->createEdge(repeat, 0, v1, 0, 1);
        ASSERT_EQ(srdag->vertexCount(), 3);
        ASSERT_NO_THROW(spider::optims::reduceUnitaryRateActors(srdag));
        ASSERT_EQ(srdag->vertexCount(), 2);
        spider::destroy(graph);
        spider::destroy(srdag);
    }
}

TEST_F(pisdfOptimsTest, forkForkTest) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    auto *v = srdag->createVertex("v", 0, 1);
    auto *v1 = srdag->createVertex("v1", 1);
    auto *v2 = srdag->createVertex("v2", 1);
    auto *v3 = srdag->createVertex("v3", 1);
    auto *v4 = srdag->createVertex("v4", 1);
    auto *v5 = srdag->createVertex("v5", 1);
    auto *fork = srdag->createForkVertex("fork", 3);
    auto *fork_0 = srdag->createForkVertex("fork_0", 2);
    auto *fork_1 = srdag->createForkVertex("fork_1", 2);
    srdag->createEdge(v, 0, fork, 0, 5);
    srdag->createEdge(fork, 0, v1, 0, 1);
    srdag->createEdge(fork, 1, fork_0, 0, 3);
    srdag->createEdge(fork, 2, v5, 0, 1);
    srdag->createEdge(fork_0, 0, v2, 0, 1);
    srdag->createEdge(fork_0, 1, fork_1, 0, 2);
    srdag->createEdge(fork_1, 0, v3, 0, 1);
    srdag->createEdge(fork_1, 1, v4, 0, 1);
    ASSERT_EQ(srdag->vertexCount(), 9);
    ASSERT_NO_THROW(spider::optims::reduceForkFork(srdag));
    ASSERT_EQ(srdag->vertexCount(), 7);
    spider::destroy(graph);
    spider::destroy(srdag);
}

TEST_F(pisdfOptimsTest, forkForkTest2) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    auto *v = srdag->createVertex("v", 0, 1);
    auto *v1 = srdag->createVertex("v1", 1);
    auto *v2 = srdag->createVertex("v2", 1);
    auto *v3 = srdag->createVertex("v3", 1);
    auto *v4 = srdag->createVertex("v4", 1);
    auto *v5 = srdag->createVertex("v5", 1);
    auto *v6 = srdag->createVertex("v6", 1);
    auto *fork_1 = srdag->createForkVertex("fork_1", 2);
    auto *fork_0 = srdag->createForkVertex("fork_0", 3);
    auto *fork = srdag->createForkVertex("fork", 2);
    auto *fork_2 = srdag->createForkVertex("fork_2", 2);
    srdag->createEdge(v, 0, fork, 0, 6);
    srdag->createEdge(fork, 0, fork_0, 0, 5);
    srdag->createEdge(fork, 1, v1, 0, 1);
    srdag->createEdge(fork_0, 0, fork_1, 0, 2);
    srdag->createEdge(fork_0, 1, v2, 0, 1);
    srdag->createEdge(fork_0, 2, fork_2, 0, 2);
    srdag->createEdge(fork_1, 0, v3, 0, 1);
    srdag->createEdge(fork_1, 1, v4, 0, 1);
    srdag->createEdge(fork_2, 0, v5, 0, 1);
    srdag->createEdge(fork_2, 1, v6, 0, 1);
    ASSERT_EQ(srdag->vertexCount(), 11);
    ASSERT_NO_THROW(spider::optims::reduceForkFork(srdag));
    ASSERT_EQ(srdag->vertexCount(), 8);
    ASSERT_EQ(srdag->vertex(7)->outputEdge(0)->sink(), v3);
    ASSERT_EQ(srdag->vertex(7)->outputEdge(1)->sink(), v4);
    ASSERT_EQ(srdag->vertex(7)->outputEdge(2)->sink(), v2);
    ASSERT_EQ(srdag->vertex(7)->outputEdge(3)->sink(), v5);
    ASSERT_EQ(srdag->vertex(7)->outputEdge(4)->sink(), v6);
    ASSERT_EQ(srdag->vertex(7)->outputEdge(5)->sink(), v1);
    spider::destroy(graph);
    spider::destroy(srdag);
}

TEST_F(pisdfOptimsTest, joinForkTest) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    auto *v = srdag->createVertex("v", 0, 1);
    auto *v1 = srdag->createVertex("v1", 0, 1);
    auto *v2 = srdag->createVertex("v2", 1);
    auto *v3 = srdag->createVertex("v3", 1);
    auto *v4 = srdag->createVertex("v4", 0, 1);
    auto *v5 = srdag->createVertex("v5", 1);
    auto *v6 = srdag->createVertex("v6", 1);
    auto *fork_0 = srdag->createForkVertex("fork_0", 3);
    auto *fork_1 = srdag->createForkVertex("fork_1", 2);
    auto *join_0 = srdag->createJoinVertex("join_0", 2);
    auto *join_1 = srdag->createJoinVertex("join_1", 2);
    srdag->createEdge(v, 0, join_0, 0, 1);
    srdag->createEdge(v1, 0, join_0, 1, 2);
    srdag->createEdge(join_0, 0, fork_0, 0, 3);
    srdag->createEdge(fork_0, 0, v2, 0, 1);
    srdag->createEdge(fork_0, 1, join_1, 0, 1);
    srdag->createEdge(fork_0, 2, v3, 0, 1);
    srdag->createEdge(v4, 0, join_1, 1, 1);
    srdag->createEdge(join_1, 0, fork_1, 0, 2);
    srdag->createEdge(fork_1, 0, v5, 0, 1);
    srdag->createEdge(fork_1, 1, v6, 0, 1);
    ASSERT_EQ(srdag->vertexCount(), 11);
    ASSERT_NO_THROW(spider::optims::reduceJoinFork(srdag));
    ASSERT_EQ(srdag->vertexCount(), 8);
    spider::destroy(graph);
    spider::destroy(srdag);
}

TEST_F(pisdfOptimsTest, joinForkTest2) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    auto *v = srdag->createVertex("v", 0, 1);
    auto *v1 = srdag->createVertex("v1", 0, 1);
    auto *v2 = srdag->createVertex("v2", 0, 1);
    auto *v3 = srdag->createVertex("v3", 1);
    auto *v4 = srdag->createVertex("v4", 1);
    auto *fork_0 = srdag->createForkVertex("fork_0", 2);
    auto *join_0 = srdag->createJoinVertex("join_0", 3);
    srdag->createEdge(v, 0, join_0, 0, 1);
    srdag->createEdge(v1, 0, join_0, 1, 1);
    srdag->createEdge(v2, 0, join_0, 2, 1);
    srdag->createEdge(join_0, 0, fork_0, 0, 3);
    srdag->createEdge(fork_0, 0, v3, 0, 2);
    srdag->createEdge(fork_0, 1, v4, 0, 1);
    ASSERT_EQ(srdag->vertexCount(), 7);
    ASSERT_NO_THROW(spider::optims::reduceJoinFork(srdag));
    ASSERT_EQ(srdag->vertexCount(), 6);
    spider::destroy(graph);
    spider::destroy(srdag);
}

TEST_F(pisdfOptimsTest, joinForkTest3) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    auto *v = srdag->createVertex("v", 0, 1);
    auto *v1 = srdag->createVertex("v1", 0, 1);
    auto *v2 = srdag->createVertex("v2", 1);
    auto *v3 = srdag->createVertex("v3", 1);
    auto *v4 = srdag->createVertex("v4", 0, 1);
    auto *v5 = srdag->createVertex("v5", 1);
    auto *fork_0 = srdag->createForkVertex("fork_0", 3);
    auto *join_0 = srdag->createJoinVertex("join_0", 3);
    srdag->createEdge(v, 0, join_0, 0, 2);
    srdag->createEdge(v1, 0, join_0, 1, 8);
    srdag->createEdge(v4, 0, join_0, 2, 3);
    srdag->createEdge(join_0, 0, fork_0, 0, 13);
    srdag->createEdge(fork_0, 0, v2, 0, 2);
    srdag->createEdge(fork_0, 1, v3, 0, 6);
    srdag->createEdge(fork_0, 2, v5, 0, 5);
    spider::api::exportGraphToDOT(graph, "./before.dot");
    ASSERT_EQ(srdag->vertexCount(), 8);
    ASSERT_NO_THROW(spider::optims::reduceJoinFork(srdag));
    ASSERT_EQ(srdag->vertexCount(), 8);
    auto exporter = spider::pisdf::SRDAGDOTExporter(srdag);
    exporter.printFromPath("./after.dot");
    spider::destroy(graph);
    spider::destroy(srdag);
}

TEST_F(pisdfOptimsTest, joinForkTest4) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    auto *v = srdag->createVertex("v", 0, 1);
    auto *v1 = srdag->createVertex("v1", 0, 1);
    auto *v2 = srdag->createVertex("v2", 1);
    auto *v3 = srdag->createVertex("v3", 1);
    auto *fork_0 = srdag->createForkVertex("fork_0", 2);
    auto *join_0 = srdag->createJoinVertex("join_0", 2);
    srdag->createEdge(v, 0, join_0, 0, 4);
    srdag->createEdge(v1, 0, join_0, 1, 3);
    srdag->createEdge(join_0, 0, fork_0, 0, 7);
    srdag->createEdge(fork_0, 0, v2, 0, 5);
    srdag->createEdge(fork_0, 1, v3, 0, 2);
    ASSERT_EQ(srdag->vertexCount(), 6);
    ASSERT_NO_THROW(spider::optims::reduceJoinFork(srdag));
    ASSERT_EQ(srdag->vertexCount(), 6);
    spider::destroy(graph);
    spider::destroy(srdag);
}

TEST_F(pisdfOptimsTest, joinForkTest5) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    auto *v = srdag->createVertex("v", 0, 1);
    auto *v1 = srdag->createVertex("v1", 0, 1);
    auto *v2 = srdag->createVertex("v2", 1);
    auto *v3 = srdag->createVertex("v3", 1);
    auto *v4 = srdag->createVertex("v4", 1);
    auto *v5 = srdag->createVertex("v5", 1);
    auto *fork_0 = srdag->createForkVertex("fork_0", 4);
    auto *join_0 = srdag->createJoinVertex("join_0", 2);
    srdag->createEdge(v, 0, join_0, 0, 2);
    srdag->createEdge(v1, 0, join_0, 1, 2);
    srdag->createEdge(join_0, 0, fork_0, 0, 4);
    srdag->createEdge(fork_0, 0, v2, 0, 1);
    srdag->createEdge(fork_0, 1, v3, 0, 1);
    srdag->createEdge(fork_0, 2, v4, 0, 1);
    srdag->createEdge(fork_0, 3, v5, 0, 1);
    ASSERT_EQ(srdag->vertexCount(), 8);
    ASSERT_NO_THROW(spider::optims::reduceJoinFork(srdag));
    ASSERT_EQ(srdag->vertexCount(), 8);
    spider::destroy(graph);
    spider::destroy(srdag);
}

TEST_F(pisdfOptimsTest, joinJoinTest) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    auto *v = srdag->createVertex("v", 0, 1);
    auto *v1 = srdag->createVertex("v1", 0, 1);
    auto *v2 = srdag->createVertex("v2", 0, 1);
    auto *join_1 = srdag->createJoinVertex("join_1", 3);
    auto *v3 = srdag->createVertex("v3", 0, 1);
    auto *join = srdag->createJoinVertex("join", 2);
    auto *v4 = srdag->createVertex("v4", 0, 1);
    auto *join_0 = srdag->createJoinVertex("join_0", 2);
    auto *v5 = srdag->createVertex("v5", 1, 0);
    srdag->createEdge(v1, 0, join, 0, 1);
    srdag->createEdge(v2, 0, join, 1, 1);
    srdag->createEdge(join, 0, join_0, 0, 2);
    srdag->createEdge(v3, 0, join_0, 1, 1);
    srdag->createEdge(v, 0, join_1, 0, 1);
    srdag->createEdge(join_0, 0, join_1, 1, 3);
    srdag->createEdge(v4, 0, join_1, 2, 1);
    srdag->createEdge(join_1, 0, v5, 0, 5);
    ASSERT_EQ(srdag->vertexCount(), 9);
    ASSERT_NO_THROW(spider::optims::reduceJoinJoin(srdag));
    ASSERT_EQ(srdag->vertexCount(), 7);
    spider::destroy(graph);
    spider::destroy(srdag);
}

TEST_F(pisdfOptimsTest, joinJoinTest2) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    auto *v = srdag->createVertex("v", 0, 1);
    auto *v1 = srdag->createVertex("v1", 0, 1);
    auto *v2 = srdag->createVertex("v2", 0, 1);
    auto *v3 = srdag->createVertex("v3", 0, 1);
    auto *v4 = srdag->createVertex("v4", 0, 1);
    auto *v5 = srdag->createVertex("v5", 0, 1);
    auto *v6 = srdag->createVertex("v6", 1);
    auto *join = srdag->createJoinVertex("join", 2);
    auto *join_0 = srdag->createJoinVertex("join_0", 2);
    auto *join_1 = srdag->createJoinVertex("join_1", 3);
    auto *join_2 = srdag->createJoinVertex("join_2", 2);
    srdag->createEdge(v, 0, join, 0, 1);
    srdag->createEdge(v1, 0, join, 1, 1);
    srdag->createEdge(v2, 0, join_0, 0, 1);
    srdag->createEdge(join, 0, join_0, 1, 2);
    srdag->createEdge(v3, 0, join_2, 0, 1);
    srdag->createEdge(v4, 0, join_2, 1, 1);
    srdag->createEdge(v5, 0, join_1, 0, 1);
    srdag->createEdge(join_0, 0, join_1, 1, 3);
    srdag->createEdge(join_2, 0, join_1, 2, 2);
    srdag->createEdge(join_1, 0, v6, 0, 6);
    ASSERT_EQ(srdag->vertexCount(), 11);
    ASSERT_NO_THROW(spider::optims::reduceJoinJoin(srdag));
    ASSERT_EQ(srdag->vertexCount(), 8);
    ASSERT_EQ(srdag->vertex(7)->inputEdge(0)->source(), v5);
    ASSERT_EQ(srdag->vertex(7)->inputEdge(1)->source(), v2);
    ASSERT_EQ(srdag->vertex(7)->inputEdge(2)->source(), v);
    ASSERT_EQ(srdag->vertex(7)->inputEdge(3)->source(), v1);
    ASSERT_EQ(srdag->vertex(7)->inputEdge(4)->source(), v3);
    ASSERT_EQ(srdag->vertex(7)->inputEdge(5)->source(), v4);
    spider::destroy(graph);
    spider::destroy(srdag);
}

TEST_F(pisdfOptimsTest, joinEndTest) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    auto *v = srdag->createVertex("v", 0, 1);
    auto *v1 = srdag->createVertex("v1", 0, 1);
    auto *v2 = srdag->createVertex("v2", 0, 1);
    auto *join = srdag->createJoinVertex("join", 3);
    auto *end = srdag->createEndVertex("end");
    srdag->createEdge(v, 0, join, 0, 1);
    srdag->createEdge(v1, 0, join, 1, 1);
    srdag->createEdge(v2, 0, join, 2, 1);
    srdag->createEdge(join, 0, end, 0, 3);
    ASSERT_EQ(srdag->vertexCount(), 5);
    ASSERT_NO_THROW(spider::optims::reduceJoinEnd(srdag));
    ASSERT_EQ(srdag->vertexCount(), 6);
    spider::destroy(graph);
    spider::destroy(srdag);
}

TEST_F(pisdfOptimsTest, repeatForkTest) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    auto *v = srdag->createVertex("v", 0, 1);
    auto *repeat = srdag->createRepeatVertex("repeat");
    auto *fork = srdag->createForkVertex("fork", 2);
    auto *v1 = srdag->createVertex("v1", 1, 0);
    auto *v2 = srdag->createVertex("v2", 1, 0);
    srdag->createEdge(v, 0, repeat, 0, 1);
    srdag->createEdge(repeat, 0, fork, 0, 2);
    srdag->createEdge(fork, 0, v1, 0, 1);
    srdag->createEdge(fork, 1, v2, 0, 1);
    ASSERT_EQ(srdag->vertexCount(), 5);
    ASSERT_NO_THROW(spider::optims::reduceRepeatFork(srdag));
    ASSERT_EQ(srdag->vertexCount(), 4);
    spider::destroy(graph);
    spider::destroy(srdag);
}

TEST_F(pisdfOptimsTest, repeatForkTest2) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    auto *v = srdag->createVertex("v", 0, 1);
    auto *repeat = srdag->createRepeatVertex("repeat");
    auto *fork = srdag->createForkVertex("fork", 3);
    auto *v1 = srdag->createVertex("v1", 1, 0);
    auto *v2 = srdag->createVertex("v2", 1, 0);
    auto *v3 = srdag->createVertex("v3", 1, 0);
    srdag->createEdge(v, 0, repeat, 0, 2);
    srdag->createEdge(repeat, 0, fork, 0, 3);
    srdag->createEdge(fork, 0, v1, 0, 1);
    srdag->createEdge(fork, 1, v2, 0, 1);
    srdag->createEdge(fork, 2, v3, 0, 1);
    ASSERT_EQ(srdag->vertexCount(), 6);
    ASSERT_NO_THROW(spider::optims::reduceRepeatFork(srdag));
    ASSERT_EQ(srdag->vertexCount(), 6);
    spider::destroy(graph);
    spider::destroy(srdag);
}