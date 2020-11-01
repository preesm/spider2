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
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs-tools/transformation/srdag/Transformation.h>
#include <api/spider.h>

class srdagTest : public ::testing::Test {
protected:

    void SetUp() override {
        spider::start();
    }

    void TearDown() override {
        spider::quit();
    }
};

/* === Function(s) definition === */


TEST_F(srdagTest, srdagFlatTest) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1);
    spider::srdag::TransfoJob rootJob{ nullptr };
    ASSERT_THROW(spider::srdag::singleRateTransformation(rootJob, spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph)), spider::Exception)
                                << "srdag::singleRateTransformation should throw for nullptr job.reference_";
    rootJob.reference_ = graph;
    ASSERT_THROW(spider::srdag::singleRateTransformation(rootJob, nullptr), spider::Exception)
                                << "srdag::singleRateTransformation should throw for nullptr srdag";
    {
        spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 1);
        auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
        ASSERT_NO_THROW(spider::srdag::singleRateTransformation(rootJob, srdag));
        /*
         * vertex_0_0 -> vertex_1_0
         */
        ASSERT_EQ(srdag->vertexCount(), 2);
        ASSERT_EQ(srdag->edgeCount(), 1);
        graph->removeEdge(graph->edges()[0].get());
        spider::destroy(srdag);
    }
    {
        spider::api::createEdge(vertex_0, 0, 2, vertex_1, 0, 1);
        auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
        ASSERT_NO_THROW(spider::srdag::singleRateTransformation(rootJob, srdag));
        /*
         *                    | -> vertex_1_0
         * vertex_0_0 -> fork | -> vertex_1_1
         */
        ASSERT_EQ(srdag->vertexCount(), 4);
        ASSERT_EQ(srdag->edgeCount(), 3);
        graph->removeEdge(graph->edges()[0].get());
        spider::destroy(srdag);
    }
    {
        spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 2);
        auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
        ASSERT_NO_THROW(spider::srdag::singleRateTransformation(rootJob, srdag));

        /*
         * vertex_0_0 -> |
         * vertex_0_1 -> | join -> vertex_1_0
         */
        ASSERT_EQ(srdag->vertexCount(), 4);
        ASSERT_EQ(srdag->edgeCount(), 3);
        graph->removeEdge(graph->edges()[0].get());
        spider::destroy(srdag);
    }
    spider::destroy(graph);
}


TEST_F(srdagTest, srdagFlatDelayTest) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1);
    auto *edge = spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 1);
    spider::api::createPersistentDelay(edge, "1");
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    {
        spider::srdag::TransfoJob rootJob{ graph };
        rootJob.params_ = graph->params();
        ASSERT_NO_THROW(spider::srdag::singleRateTransformation(rootJob, srdag));
        ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    }
    /*
     * init -> vertex_1
     * vertex_0 -> end
     */
    ASSERT_EQ(srdag->vertexCount(), 4);
    ASSERT_EQ(srdag->edgeCount(), 2);
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagFlatDelayTest1) {
    auto *graph = spider::api::createGraph("topgraph", 2, 3);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 3);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 3);
    spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 1);
    spider::api::createEdge(vertex_0, 1, 1, vertex_1, 1, 1);
    auto *edge = spider::api::createEdge(vertex_0, 2, 1, vertex_1, 2, 1);
    spider::api::createPersistentDelay(edge, "1");
    graph->removeEdge(graph->edges()[0].get());
    graph->removeEdge(graph->edges()[1].get());
    spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 1);
    spider::api::createEdge(vertex_0, 1, 1, vertex_1, 1, 1);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    {
        spider::srdag::TransfoJob rootJob{ graph };
        rootJob.params_ = graph->params();
        ASSERT_NO_THROW(spider::srdag::singleRateTransformation(rootJob, srdag));
        ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    }
    /*
     *       init -> | vertex_1
     *          | -> |
     * vertex_0 | -> end
     */
    ASSERT_EQ(srdag->vertexCount(), 4);
    ASSERT_EQ(srdag->edgeCount(), 4);
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagFlatDelayTest2) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1);
    auto *edge = spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 1);
    spider::api::createPersistentDelay(edge, "2");
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    {
        spider::srdag::TransfoJob rootJob{ graph };
        rootJob.params_ = graph->params();
        ASSERT_NO_THROW(spider::srdag::singleRateTransformation(rootJob, srdag));
        ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    }
    /*
     *              | -> vertex_1_0
     * init -> fork | -> |
     *     vertex_0_0 -> | join -> end
     */
    ASSERT_EQ(srdag->vertexCount(), 6);
    ASSERT_EQ(srdag->edgeCount(), 5);
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagFlatDelayTest3) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1);
    auto *edge = spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 2);
    spider::api::createPersistentDelay(edge, "1");
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    spider::srdag::TransfoJob rootJob{ graph };
    rootJob.params_ = graph->params();
    ASSERT_NO_THROW(spider::srdag::singleRateTransformation(rootJob, srdag));
    ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    /*
     *
     *       init -> |
     * vertex_0_0 -> | join -> vertex_1_0
     * vertex_0_1 -> end
     */
    ASSERT_EQ(srdag->vertexCount(), 6);
    ASSERT_EQ(srdag->edgeCount(), 4);
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagFlatDelayTest4) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1);
    auto *edge = spider::api::createEdge(vertex_0, 0, 2, vertex_1, 0, 1);
    spider::api::createPersistentDelay(edge, "1");
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    spider::srdag::TransfoJob rootJob{ graph };
    rootJob.params_ = graph->params();
    ASSERT_NO_THROW(spider::srdag::singleRateTransformation(rootJob, srdag));
    ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    /*
     * init -> vertex_1_0
     * vertex_0_0 -> fork | -> vertex_1_1
     *                    | -> end
     */
    ASSERT_EQ(srdag->vertexCount(), 6);
    ASSERT_EQ(srdag->edgeCount(), 4);
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagFlatDelayTest5) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1);
    auto *edge = spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 2);
    spider::api::createPersistentDelay(edge, "2");
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    spider::srdag::TransfoJob rootJob{ graph };
    rootJob.params_ = graph->params();
    ASSERT_NO_THROW(spider::srdag::singleRateTransformation(rootJob, srdag));
    ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    /*
     * init -> vertex_1_0
     * vertex_0_0 -> | join -> end
     * vertex_0_1 -> |
     */
    ASSERT_EQ(srdag->vertexCount(), 6);
    ASSERT_EQ(srdag->edgeCount(), 4);
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagFlatDelayTest6) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 1, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1, 1);
    spider::api::createEdge(vertex_1, 0, 1, vertex_0, 0, 1);
    auto *edge = spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 1);
    spider::api::createPersistentDelay(edge, "1");
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    spider::srdag::TransfoJob rootJob{ graph };
    rootJob.params_ = graph->params();
    ASSERT_NO_THROW(spider::srdag::singleRateTransformation(rootJob, srdag));
    ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    /*
     * init -> vertex_1_0 -> vertex_0_0 -> end
     */
    ASSERT_EQ(srdag->vertexCount(), 4);
    ASSERT_EQ(srdag->edgeCount(), 3);
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagFlatDelayTest7) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 1, 1);
    auto *edge = spider::api::createEdge(vertex_0, 0, 2, vertex_0, 0, 2);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    spider::srdag::TransfoJob rootJob{ graph };
    rootJob.params_ = graph->params();
    ASSERT_THROW(spider::srdag::singleRateTransformation(rootJob, srdag), spider::Exception)
                                << "SingleRateTransformer::singleRateLinkage should throw for self loop with no delay";
    srdag->removeVertex(srdag->vertex(0)); /* = Before failing, it did copy the vertex = */
    spider::api::createPersistentDelay(edge, "1");
    rootJob.params_ = graph->params();
    ASSERT_THROW(spider::srdag::singleRateTransformation(rootJob, srdag), spider::Exception)
                                << "SingleRateTransformer::singleRateLinkage should throw for self loop with insufficient delay";
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagFlatDelayTest8) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 1, 1);
    auto *edge = spider::api::createEdge(vertex_0, 0, 2, vertex_0, 0, 2);
    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    spider::srdag::TransfoJob rootJob{ graph };
    rootJob.params_ = graph->params();
    ASSERT_THROW(spider::srdag::singleRateTransformation(rootJob, srdag), spider::Exception)
                                << "SingleRateTransformer::singleRateLinkage should throw for self loop with no delay";
    srdag->removeVertex(srdag->vertex(0)); /* = Before failing, it did copy the vertex = */
    spider::api::createPersistentDelay(edge, "2");
    rootJob.params_ = graph->params();
    ASSERT_NO_THROW(spider::srdag::singleRateTransformation(rootJob, srdag));
    /*
     * init -> vertex_0_0 -> end
     */
    ASSERT_EQ(srdag->vertexCount(), 3);
    ASSERT_EQ(srdag->edgeCount(), 2);
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagHTest) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *subgraph = spider::api::createSubgraph(graph, "subgraph", 1, 2, 0, 1, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1);
    auto *input = spider::api::setInputInterfaceName(subgraph, 0, "input");
    auto *output = spider::api::setOutputInterfaceName(subgraph, 0, "output");
    auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 1, 1);
    spider::api::createEdge(vertex_0, 0, 1, subgraph, 0, 1);
    spider::api::createEdge(subgraph, 0, 1, vertex_1, 0, 1);
    spider::api::createEdge(input, 0, 1, vertex_2, 0, 1);
    spider::api::createEdge(vertex_2, 0, 1, output, 0, 1);

    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    spider::srdag::TransfoJob rootJob{ graph };
    std::pair<spider::srdag::JobStack, spider::srdag::JobStack> res;
    ASSERT_NO_THROW(res = spider::srdag::singleRateTransformation(rootJob, srdag));
    ASSERT_EQ(res.first.empty(), false)
                                << "srdag::singleRateTransformation should not return nullptr for static H graph";
    ASSERT_EQ(res.second.empty(), true) << "srdag::singleRateTransformation should return nullptr for static H graph";
    /*
     * vertex_0_0 -> subgraph_0 -> vertex_1_0
     */
    ASSERT_EQ(srdag->vertexCount(), 3);
    ASSERT_EQ(srdag->edgeCount(), 2);
    ASSERT_NO_THROW(spider::srdag::singleRateTransformation(res.first[0], srdag));
    ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    /*
     * vertex_0_0 -> vertex_2_0 -> vertex_1_0
     */
    ASSERT_EQ(srdag->vertexCount(), 3);
    ASSERT_EQ(srdag->edgeCount(), 2);
    spider::destroy(srdag);
    spider::destroy(graph);
}


TEST_F(srdagTest, srdagHTest1) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *subgraph = spider::api::createSubgraph(graph, "subgraph", 1, 2, 0, 1, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1);
    auto *input = spider::api::setInputInterfaceName(subgraph, 0, "input");
    auto *output = spider::api::setOutputInterfaceName(subgraph, 0, "output");
    auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 1, 1);
    spider::api::createEdge(vertex_0, 0, 1, subgraph, 0, 1);
    spider::api::createEdge(subgraph, 0, 1, vertex_1, 0, 1);
    spider::api::createEdge(input, 0, 1, vertex_2, 0, 1);
    spider::api::createEdge(vertex_2, 0, 1, output, 0, 1);

    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    spider::srdag::TransfoJob rootJob{ graph };
    std::pair<spider::srdag::JobStack, spider::srdag::JobStack> res;
    ASSERT_NO_THROW(res = spider::srdag::singleRateTransformation(rootJob, srdag));
    ASSERT_EQ(res.first.empty(), false)
                                << "srdag::singleRateTransformation should not return nullptr for static H graph";
    ASSERT_EQ(res.second.empty(), true) << "srdag::singleRateTransformation should return nullptr for static H graph";
    /*
     * vertex_0_0 -> subgraph_0 -> vertex_1_0
     */
    ASSERT_EQ(srdag->vertexCount(), 3);
    ASSERT_EQ(srdag->edgeCount(), 2);
    srdag->removeVertex(res.first[0].srdagInstance_);
    res.first[0].srdagInstance_ = nullptr;
    ASSERT_THROW(spider::srdag::singleRateTransformation(res.first[0], srdag), spider::Exception);
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagHTest2) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *subgraph = spider::api::createSubgraph(graph, "subgraph", 2, 2, 0, 1, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1);
    auto *input = spider::api::setInputInterfaceName(subgraph, 0, "input");
    auto *output = spider::api::setOutputInterfaceName(subgraph, 0, "output");
    auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 2, 1);
    auto *vertex_3 = spider::api::createVertex(subgraph, "vertex_3", 1, 1);
    auto *cfg = spider::api::createConfigActor(subgraph, "cfg", 0, 1);
    spider::api::createEdge(vertex_0, 0, 1, subgraph, 0, 1);
    spider::api::createEdge(subgraph, 0, 1, vertex_1, 0, 1);
    spider::api::createEdge(input, 0, 1, vertex_2, 0, 1);
    spider::api::createEdge(cfg, 0, 1, vertex_2, 1, 1);
    spider::api::createEdge(vertex_2, 0, 1, vertex_3, 0, 1);
    spider::api::createEdge(vertex_3, 0, 1, output, 0, 1);
    spider::api::createStaticParam(subgraph, "height", 10);
    spider::api::createDynamicParam(subgraph, "width");

    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    spider::srdag::TransfoJob rootJob{ graph };
    std::pair<spider::srdag::JobStack, spider::srdag::JobStack> res;
    ASSERT_NO_THROW(res = spider::srdag::singleRateTransformation(rootJob, srdag));
    ASSERT_NO_THROW(spider::api::exportGraphToDOT(graph, "graph.dot"));
    ASSERT_EQ(res.first.empty(), false)
                                << "srdag::singleRateTransformation should not return nullptr for static H graph";
    ASSERT_EQ(res.second.empty(), true)
                                << "srdag::singleRateTransformation should not return nullptr for static H graph";
    /*
     *  vertex_0_0 -> subgraph_0 -> vertex_1_ 0
     */
    ASSERT_EQ(srdag->vertexCount(), 3);
    ASSERT_EQ(srdag->edgeCount(), 2);
    ASSERT_NO_THROW(res = spider::srdag::singleRateTransformation(res.first[0], srdag));
    /*
     *     vertex_0_0 -> |
     *            cfg -> | subgraph_run -> vertex_1_ 0
     */
    ASSERT_EQ(srdag->vertexCount(), 4);
    ASSERT_EQ(srdag->edgeCount(), 3);
    ASSERT_NO_THROW(spider::srdag::singleRateTransformation(res.second[0], srdag));
    ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    /*
    *    vertex_0_0 -> |
    *           cfg -> | vertex_2_0 -> vertex_3_0 -> vertex_1_0
    */
    ASSERT_EQ(srdag->vertexCount(), 5);
    ASSERT_EQ(srdag->edgeCount(), 4);
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagHTest3) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *subgraph = spider::api::createSubgraph(graph, "subgraph", 2, 2, 0, 1, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1);
    auto *input = spider::api::setInputInterfaceName(subgraph, 0, "input");
    auto *output = spider::api::setOutputInterfaceName(subgraph, 0, "output");
    auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 1, 1);
    auto *cfg = spider::api::createConfigActor(subgraph, "cfg", 1, 1);
    spider::api::createEdge(vertex_0, 0, 1, subgraph, 0, 1);
    spider::api::createEdge(subgraph, 0, 1, vertex_1, 0, 1);
    spider::api::createEdge(input, 0, 1, cfg, 0, 1);
    spider::api::createEdge(cfg, 0, 1, vertex_2, 0, 1);
    spider::api::createEdge(vertex_2, 0, 1, output, 0, 1);
    spider::api::createStaticParam(subgraph, "height", 10);
    spider::api::createDynamicParam(subgraph, "width");

    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    spider::srdag::TransfoJob rootJob{ graph };
    std::pair<spider::srdag::JobStack, spider::srdag::JobStack> res;
    ASSERT_NO_THROW(res = spider::srdag::singleRateTransformation(rootJob, srdag));
    ASSERT_NO_THROW(spider::api::exportGraphToDOT(graph, "pisdf.dot"));
    ASSERT_EQ(res.first.empty(), false)
                                << "srdag::singleRateTransformation should not return nullptr for dynamic H graph";
    ASSERT_EQ(res.second.empty(), true) << "srdag::singleRateTransformation should return nullptr for dynamic H graph";
    /*
     * vertex_0_0 -> subgraph_0 -> vertex_1_0
     */
    ASSERT_EQ(srdag->vertexCount(), 3);
    ASSERT_EQ(srdag->edgeCount(), 2);
    ASSERT_NO_THROW(res = spider::srdag::singleRateTransformation(res.first[0], srdag));
    ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    ASSERT_EQ(res.second.empty(), false) << "srdag::singleRateTransformation should return nullptr for dynamic H graph";
    /*
     * vertex_0_0 -> cfg -> grun_subgraph_0 -> vertex_1_0
     */
    ASSERT_EQ(srdag->vertexCount(), 4);
    ASSERT_EQ(srdag->edgeCount(), 3);
    ASSERT_NO_THROW(spider::srdag::singleRateTransformation(res.second[0], srdag));
    ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    /*
    *  vertex_0_0 ->  cfg -> vertex_2_0 -> vertex_1_0
    */
    ASSERT_EQ(srdag->vertexCount(), 4);
    ASSERT_EQ(srdag->edgeCount(), 3);
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagHTest4) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *subgraph = spider::api::createSubgraph(graph, "subgraph", 2, 2, 0, 1, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1);
    auto *input = spider::api::setInputInterfaceName(subgraph, 0, "input");
    auto *output = spider::api::setOutputInterfaceName(subgraph, 0, "output");
    auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 1, 1);
    auto *vertex_3 = spider::api::createVertex(subgraph, "vertex_3", 1);
    auto *cfg = spider::api::createConfigActor(subgraph, "cfg", 0, 1);
    spider::api::createEdge(vertex_0, 0, 1, subgraph, 0, 1);
    spider::api::createEdge(subgraph, 0, 1, vertex_1, 0, 1);
    spider::api::createEdge(input, 0, 1, vertex_2, 0, 1);
    spider::api::createEdge(cfg, 0, 1, output, 0, 1);
    spider::api::createEdge(vertex_2, 0, 1, vertex_3, 0, 1);
    spider::api::createStaticParam(subgraph, "height", 10);
    spider::api::createDynamicParam(subgraph, "width");
    ASSERT_NO_THROW(spider::api::exportGraphToDOT(graph, "pisdf.dot"));

    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    spider::srdag::TransfoJob rootJob{ graph };
    std::pair<spider::srdag::JobStack, spider::srdag::JobStack> res;
    ASSERT_NO_THROW(res = spider::srdag::singleRateTransformation(rootJob, srdag));
    ASSERT_NO_THROW(spider::api::exportGraphToDOT(graph, "graph.dot"));
    ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    ASSERT_EQ(res.first.empty(), false)
                                << "srdag::singleRateTransformation staticJobStack should not be empty for static H graph";
    ASSERT_EQ(res.second.empty(), true)
                                << "srdag::singleRateTransformation dynamicJobStack should be empty for static H graph";
    /*
     * vertex_0_0 -> subgraph_0 -> vertex_0_1
     */
    ASSERT_EQ(srdag->vertexCount(), 3);
    ASSERT_EQ(srdag->edgeCount(), 2);
    ASSERT_NO_THROW(res = spider::srdag::singleRateTransformation(res.first[0], srdag));
    ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    ASSERT_EQ(res.second.empty(), false)
                                << "srdag::singleRateTransformation dynamicJobStack should not be empty for dynamic H graph";
    /*
     * cfg -> vertex_1_0
     * vertex_0_0 -> grun_subgraph_0
     */
    ASSERT_EQ(srdag->vertexCount(), 4);
    ASSERT_EQ(srdag->edgeCount(), 2);
    ASSERT_NO_THROW(spider::srdag::singleRateTransformation(res.second[0], srdag));
    ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    /*
     * cfg -> vertex_1_0
     * vertex_0_0 -> vertex_2_0 -> vertex_3_0
     */
    ASSERT_EQ(srdag->vertexCount(), 5);
    ASSERT_EQ(srdag->edgeCount(), 3);
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagHTest5) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *subgraph = spider::api::createSubgraph(graph, "subgraph", 2, 2, 0, 0, 0);
    auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 0, 1);
    auto *cfg = spider::api::createConfigActor(subgraph, "cfg", 1);
    spider::api::createEdge(vertex_2, 0, 1, cfg, 0, 1);
    spider::api::createDynamicParam(subgraph, "width");
    ASSERT_NO_THROW(spider::api::exportGraphToDOT(graph, "graph.dot"));

    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    spider::srdag::TransfoJob rootJob{ graph };
    std::pair<spider::srdag::JobStack, spider::srdag::JobStack> res;
    ASSERT_NO_THROW(res = spider::srdag::singleRateTransformation(rootJob, srdag));
    ASSERT_NO_THROW(spider::api::exportGraphToDOT(graph, "graph.dot"));
    ASSERT_THROW(spider::srdag::singleRateTransformation(res.first[0], srdag), spider::Exception)
                                << "srdag::singleRateTransformation should throw when cfg actors receive token from non interface actor.";
    spider::destroy(srdag);
    spider::destroy(graph);
}

TEST_F(srdagTest, srdagHTest6) {
    auto *graph = spider::api::createGraph("topgraph", 2, 1);
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *subgraph = spider::api::createSubgraph(graph, "subgraph", 1, 2, 0, 1, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1);
    auto *input = spider::api::setInputInterfaceName(subgraph, 0, "input");
    auto *output = spider::api::setOutputInterfaceName(subgraph, 0, "output");
    auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 2, 2);
    spider::api::createEdge(vertex_0, 0, 1, subgraph, 0, 1);
    spider::api::createEdge(subgraph, 0, 1, vertex_1, 0, 1);
    spider::api::createEdge(input, 0, 1, vertex_2, 0, 1);
    spider::api::createEdge(vertex_2, 0, 1, output, 0, 1);
    auto *edge = spider::api::createEdge(vertex_2, 1, 0, vertex_2, 1, 0);
    spider::api::createPersistentDelay(edge, "1");
    ASSERT_NO_THROW(spider::api::exportGraphToDOT(graph, "pisdf.dot"));

    auto *srdag = spider::make<spider::srdag::Graph, StackID::TRANSFO>(graph);
    spider::srdag::TransfoJob rootJob{ graph };
    std::pair<spider::srdag::JobStack, spider::srdag::JobStack> res;
    ASSERT_NO_THROW(res = spider::srdag::singleRateTransformation(rootJob, srdag));
    ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    ASSERT_EQ(res.first.empty(), false)
                                << "srdag::singleRateTransformation should not return nullptr for static H graph";
    ASSERT_EQ(res.second.empty(), true) << "srdag::singleRateTransformation should return nullptr for static H graph";
    /*
     * vertex_0_0 -> subgraph_0 -> vertex_1_0
     *     init   ->            -> end
     */
    ASSERT_EQ(srdag->vertexCount(), 5);
    ASSERT_EQ(srdag->edgeCount(), 4);
    ASSERT_NO_THROW(spider::srdag::singleRateTransformation(res.first[0], srdag));
    ASSERT_NO_THROW(srdag->exportToDOT("srdag.dot"));
    spider::destroy(srdag);
    spider::destroy(graph);
}