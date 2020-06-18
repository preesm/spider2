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
#include <graphs/pisdf/DynamicParam.h>
#include <graphs/pisdf/ExecVertex.h>
#include <api/spider.h>

class pisdfGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::start();
        spider::api::createPlatform(1, 1);

        auto *x86MemoryInterface = spider::api::createMemoryInterface(20000);

        auto *x86Cluster = spider::api::createCluster(1, x86MemoryInterface);

        auto x86PECore0 = spider::api::createProcessingElement(0, 0, x86Cluster, "x86-Core0", spider::PEType::LRT);

        spider::api::setSpiderGRTPE(x86PECore0);
    }

    void TearDown() override {
        spider::quit();
    }
};

TEST_F(pisdfGraphTest, graphTest) {
    /* == Ctor tests == */
    ASSERT_NO_THROW(spider::pisdf::Graph()) << "Graph() and ~Graph() should never throw";
    ASSERT_NO_THROW(spider::pisdf::Graph("", 1)) << "Graph() and ~Graph() should never throw";
    ASSERT_NO_THROW(spider::pisdf::Graph("", 0, 1)) << "Graph() and ~Graph() should never throw";
    ASSERT_NO_THROW(spider::pisdf::Graph("", 0, 0, 1)) << "Graph() and ~Graph() should never throw";
    ASSERT_NO_THROW(spider::pisdf::Graph("", 0, 0, 0, 1)) << "Graph() and ~Graph() should never throw";
    ASSERT_NO_THROW(spider::pisdf::Graph("", 0, 0, 0, 0, 1)) << "Graph() and ~Graph() should never throw";
    ASSERT_NO_THROW(spider::pisdf::Graph("", 0, 0, 0, 0, 0, 1)) << "Graph() and ~Graph() should never throw";

    auto *graph = spider::make<spider::pisdf::Graph, StackID::PISDF>("graph", 4, 2, 3);
    /* == Param tests == */
    auto param = spider::make_shared<spider::pisdf::Param, StackID::PISDF>("width", spider::Expression(5));
    ASSERT_NO_THROW(graph->addParam(param)) << "Graph::addParam() should not throw on valid value.";
    ASSERT_THROW(graph->addParam(param), spider::Exception) << "Graph::addParam() should throw for pre-existing param.";
    {
        auto param2 = spider::make_shared<spider::pisdf::Param, StackID::PISDF>("WIDTH", spider::Expression(5));
        ASSERT_THROW(graph->addParam(param2), spider::Exception)
                                    << "Graph::addParam() should throw for param with same name.";
    }
    graph->addParam(spider::make_shared<spider::pisdf::Param, StackID::PISDF>("height", spider::Expression(1)));

    ASSERT_NO_THROW(graph->addVertex(nullptr)) << "pisdf::Graph::addVertex should not throw for nullptr.";
    ASSERT_NO_THROW(graph->removeVertex(nullptr)) << "pisdf::Graph::removeVertex should not throw for nullptr.";
    ASSERT_NO_THROW(graph->addEdge(nullptr)) << "AbstractGraph::addEdge should not throw for nullptr.";
    ASSERT_NO_THROW(graph->removeEdge(nullptr)) << "AbstractGraph::removeEdge should not throw for nullptr.";
    ASSERT_NO_THROW(graph->removeParam(nullptr)) << "pisdf::Graph::removeParam should not throw for nullptr.";

    /* == Getter(s) test == */
    ASSERT_EQ(graph->paramFromName("WIDTH").get(), param.get());
    ASSERT_EQ(graph->paramFromName("width").get(), param.get());
    ASSERT_EQ(graph->paramFromName("wIdTh").get(), param.get());
    ASSERT_EQ(graph->paramFromName("height2").get(), nullptr);
    ASSERT_EQ(graph->vertexCount(), 0) << "Graph::vertexCount() failed.";
    ASSERT_EQ(graph->edgeCount(), 0) << "Graph::edgeCount() failed.";
    ASSERT_EQ(graph->paramCount(), 2) << "Graph::paramCount() failed.";
    ASSERT_EQ(graph->configVertexCount(), 0) << "Graph::configVertexCount() failed.";
    ASSERT_EQ(graph->subgraphCount(), 0) << "Graph::subgraphCount() failed.";
    ASSERT_EQ(graph->dynamic(), false) << "Graph::dynamic() failed.";
    ASSERT_EQ(graph->subIx(), SIZE_MAX) << "Graph::subIx() failed.";

    /* == Test subgraph == */
    auto *vertex_0 = spider::api::createVertex(graph, "vertex_0", 0, 1);
    auto *vertex_1 = spider::api::createVertex(graph, "vertex_1", 1, 1);
    auto *subgraph = spider::api::createSubgraph(graph, "subgraph", 3, 4, 2, 1, 1);
    auto *input = spider::api::setInputInterfaceName(subgraph, 0, "input");
    auto *output = spider::api::setOutputInterfaceName(subgraph, 0, "output");
    auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 2, 1);
    auto *vertex_3 = spider::api::createVertex(subgraph, "vertex_3", 1, 1);
    auto *vertex_4 = spider::api::createVertex(graph, "vertex_4", 1);
    auto *cfg = spider::api::createConfigActor(subgraph, "cfg", 0, 1);
    spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 1);
    spider::api::createEdge(vertex_1, 0, 1, subgraph, 0, 1);
    spider::api::createEdge(input, 0, 5, vertex_2, 0, 1);
    spider::api::createEdge(vertex_2, 0, 1, vertex_3, 0, 5);
    spider::api::createEdge(vertex_3, 0, 1, output, 0, 1);
    spider::api::createEdge(subgraph, 0, 5, vertex_4, 0, 5);
    spider::api::createEdge(cfg, 0, 15, vertex_2, 1, 1);

    /* == Getter tests == */
    ASSERT_EQ(graph->totalActorCount(), 6);
    ASSERT_EQ(graph->vertices().size(), 4) << "Graph::vertices() failed.";
    ASSERT_EQ(graph->subgraphs().size(), 1) << "Graph::subgraphs() failed.";
    ASSERT_EQ(subgraph->configVertices().size(), 1) << "Graph::configVertices() failed.";
    ASSERT_EQ(subgraph->inputInterfaceVector().size(), subgraph->inputEdgeCount())
                                << "Graph::inputInterfaceArray() failed.";
    ASSERT_EQ(subgraph->outputInterfaceVector().size(), subgraph->outputEdgeCount())
                                << "Graph::outputInterfaceArray() failed.";
    ASSERT_EQ(graph->vertex(0), vertex_0) << "Graph::vertex(ix) failed";
    ASSERT_EQ(graph->vertex(1), vertex_1) << "Graph::vertex(ix) failed";
    ASSERT_EQ(graph->vertex(2), subgraph) << "Graph::vertex(ix) failed";
    ASSERT_EQ(graph->vertex(3), vertex_4) << "Graph::vertex(ix) failed";
    spider::api::createDynamicParam(subgraph, "width");
    ASSERT_EQ(subgraph->dynamic(), true) << "Graph::dynamic() failed.";

    /* == Setter test == */
    ASSERT_NO_THROW(subgraph->addInputInterface(nullptr));
    ASSERT_NO_THROW(subgraph->addOutputInterface(nullptr));
    auto outputInterface = spider::make<spider::pisdf::Interface, StackID::PISDF>(spider::pisdf::VertexType::OUTPUT);
    auto inputInterface = spider::make<spider::pisdf::Interface, StackID::PISDF>(spider::pisdf::VertexType::INPUT);
    ASSERT_NO_THROW(subgraph->addInputInterface(outputInterface));
    ASSERT_NO_THROW(subgraph->addInputInterface(inputInterface));
    ASSERT_NO_THROW(subgraph->addOutputInterface(inputInterface));
    ASSERT_NO_THROW(subgraph->addOutputInterface(outputInterface));

    /* == Move Edge tests == */
    ASSERT_NO_THROW(graph->moveEdge(graph->edges()[0].get(), nullptr))
                                << "Graph::moveEdge() should not throw for nullptr graph";
    ASSERT_NO_THROW(graph->moveEdge(nullptr, subgraph)) << "Graph::moveEdge() should not throw for nullptr edge";
    ASSERT_NO_THROW(graph->moveEdge(graph->edges()[0].get(), subgraph)) << "Graph::moveEdge() should not throw";
    ASSERT_NO_THROW(subgraph->moveEdge(subgraph->edges()[subgraph->edgeCount() - 1].get(), graph))
                                << "Graph::moveEdge() should not throw";
    ASSERT_NO_THROW(graph->moveEdge(graph->edges()[0].get(), graph)) << "Graph::moveEdge() should not throw";


    /* == Remove param test == */
    ASSERT_NO_THROW(graph->addParam(spider::make_shared<spider::pisdf::DynamicParam, StackID::PISDF>("dyna1")));
    ASSERT_NO_THROW(graph->addParam(spider::make_shared<spider::pisdf::DynamicParam, StackID::PISDF>("dyna2")));
    ASSERT_EQ(graph->paramCount(), 4);
    ASSERT_NO_THROW(graph->removeParam(graph->params()[2]));

    /* == Move / Remove vertex test == */
    ASSERT_NO_THROW(graph->moveVertex(graph->vertex(0), nullptr))
                                << "Graph::moveVertex() should not throw for nullptr graph";
    ASSERT_NO_THROW(graph->moveVertex(nullptr, subgraph)) << "Graph::moveVertex() should not throw for nullptr vertex";
    ASSERT_NO_THROW(graph->moveVertex(graph->vertex(0), subgraph)) << "Graph::moveVertex() should not throw";
    ASSERT_NO_THROW(subgraph->moveVertex(subgraph->vertex(subgraph->vertexCount() - 1), graph))
                                << "Graph::moveVertex() should not throw";
    ASSERT_NO_THROW(graph->moveVertex(graph->vertex(0), graph)) << "Graph::moveVertex() should not throw";
    ASSERT_NO_THROW(subgraph->moveVertex(subgraph->configVertices()[0], graph))
                                << "Graph::moveVertex() should not throw";
    ASSERT_NO_THROW(graph->removeVertex(graph->configVertices()[0])) << "Graph::moveVertex() should not throw";
    ASSERT_NO_THROW(graph->removeVertex(subgraph)) << "Graph::moveVertex() should not throw";


    spider::destroy(graph);
}