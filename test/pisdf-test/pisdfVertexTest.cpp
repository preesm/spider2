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
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/DynamicParam.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <api/spider.h>
#include <graphs/pisdf/visitors/CloneVertexVisitor.h>

class pisdVertexTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::createStackAllocator(spider::allocType<spider::AllocatorType::GENERIC>{ }, StackID::GENERAL, "alloc-test");
        spider::createStackAllocator(spider::allocType<spider::AllocatorType::GENERIC>{ }, StackID::EXPRESSION, "alloc-test");
        spider::createStackAllocator(spider::allocType<spider::AllocatorType::GENERIC>{ }, StackID::PISDF, "alloc-test");
        spider::createStackAllocator(spider::allocType<spider::AllocatorType::GENERIC>{ }, StackID::ARCHI, "alloc-test");
        spider::createStackAllocator(spider::allocType<spider::AllocatorType::GENERIC>{ }, StackID::CONSTRAINTS,
                                     "alloc-test");

        spider::api::createPlatform();

        auto *x86MemoryUnit = spider::api::createMemoryUnit(nullptr, 20000);

        auto *x86Cluster = spider::api::createCluster(1, x86MemoryUnit);

        auto x86PECore0 = spider::api::createPE(0, 0, 0,
                                                x86Cluster,
                                                "x86-Core0",
                                                spider::PEType::LRT_PE,
                                                spider::HWType::PHYS_PE);
        spider::api::setSpiderGRTPE(x86PECore0);
    }

    void TearDown() override {
        spider::quit();
    }
};

void testHierarchical() {
    ASSERT_EQ((spider::pisdf::ExecVertex().hierarchical()), false) << "Vertex::hierarchical() should be false except for graph.";
    ASSERT_EQ((spider::pisdf::InputInterface().hierarchical()), false) << "Vertex::hierarchical() should be false except for graph.";
    ASSERT_EQ((spider::pisdf::OutputInterface().hierarchical()), false) << "Vertex::hierarchical() should be false except for graph.";
    ASSERT_EQ((spider::pisdf::Graph().hierarchical()), true) << "Graph::hierarchical() should be true.";
}

void testExecutable() {
    ASSERT_EQ((spider::pisdf::ExecVertex().executable()), true) << "ExecVertex::executable() should be true.";
    ASSERT_EQ((spider::pisdf::ForkVertex().executable()), true) << "ForkVertex::executable() should be true.";
    ASSERT_EQ((spider::pisdf::JoinVertex().executable()), true) << "JoinVertex::executable() should be true.";
    ASSERT_EQ((spider::pisdf::HeadVertex().executable()), true) << "HeadVertex::executable() should be true.";
    ASSERT_EQ((spider::pisdf::TailVertex().executable()), true) << "TailVertex::executable() should be true.";
    ASSERT_EQ((spider::pisdf::ConfigVertex().executable()), true) << "ConfigVertex::executable() should be true.";
    ASSERT_EQ((spider::pisdf::RepeatVertex().executable()), true) << "RepeatVertex::executable() should be true.";
    ASSERT_EQ((spider::pisdf::DuplicateVertex().executable()), true) << "DuplicateVertex::executable() should be true.";
    ASSERT_EQ((spider::pisdf::InitVertex().executable()), true) << "InitVertex::executable() should be true.";
    ASSERT_EQ((spider::pisdf::EndVertex().executable()), true) << "EndVertex::executable() should be true.";
    ASSERT_EQ((spider::pisdf::InputInterface().executable()), false) << "Vertex::executable() should be false.";
    ASSERT_EQ((spider::pisdf::OutputInterface().executable()), false) << "Vertex::executable() should be false.";
    ASSERT_EQ((spider::pisdf::Graph().executable()), false) << "Vertex::executable() should be false.";
}

void testType() {
    ASSERT_EQ((spider::pisdf::ExecVertex().subtype()), spider::pisdf::VertexType::NORMAL) << "ExecVertex::subtype() should be VertexType::NORMAL.";
    ASSERT_EQ((spider::pisdf::ForkVertex().subtype()), spider::pisdf::VertexType::FORK) << "ForkVertex::subtype() should be VertexType::FORK.";
    ASSERT_EQ((spider::pisdf::JoinVertex().subtype()), spider::pisdf::VertexType::JOIN) << "JoinVertex::subtype() should be VertexType::JOIN.";
    ASSERT_EQ((spider::pisdf::HeadVertex().subtype()), spider::pisdf::VertexType::HEAD) << "HeadVertex::subtype() should be VertexType::HEAD.";
    ASSERT_EQ((spider::pisdf::TailVertex().subtype()), spider::pisdf::VertexType::TAIL) << "TailVertex::subtype() should be VertexType::TAIL.";
    ASSERT_EQ((spider::pisdf::ConfigVertex().subtype()), spider::pisdf::VertexType::CONFIG) << "ConfigVertex::subtype() should be VertexType::CONFIG.";
    ASSERT_EQ((spider::pisdf::RepeatVertex().subtype()), spider::pisdf::VertexType::REPEAT) << "RepeatVertex::subtype() should be VertexType::REPEAT.";
    ASSERT_EQ((spider::pisdf::DuplicateVertex().subtype()), spider::pisdf::VertexType::DUPLICATE) << "DuplicateVertex::subtype() should be VertexType::DUPLICATE.";
    ASSERT_EQ((spider::pisdf::InitVertex().subtype()), spider::pisdf::VertexType::INIT) << "InitVertex::subtype() should be VertexType::INIT.";
    ASSERT_EQ((spider::pisdf::EndVertex().subtype()), spider::pisdf::VertexType::END) << "EndVertex::subtype() should be VertexType::END.";
    ASSERT_EQ((spider::pisdf::InputInterface().subtype()), spider::pisdf::VertexType::INPUT) << "InputInterface::subtype() should be VertexType::INPUT.";
    ASSERT_EQ((spider::pisdf::OutputInterface().subtype()), spider::pisdf::VertexType::OUTPUT) << "OutputInterface::subtype() should be VertexType::OUTPUT.";
    ASSERT_EQ((spider::pisdf::Graph().subtype()), spider::pisdf::VertexType::GRAPH) << "Graph::subtype() should be VertexType::GRAPH.";
    ASSERT_EQ((spider::pisdf::DelayVertex().subtype()), spider::pisdf::VertexType::DELAY) << "DelayVertex::subtype() should be VertexType::DELAY.";
}

void testCpyCtor() {
    auto test = spider::pisdf::ExecVertex();
    ASSERT_EQ((spider::pisdf::ExecVertex(test).subtype()), spider::pisdf::VertexType::NORMAL) << "ExecVertex::subtype() should be VertexType::NORMAL.";
    auto test2 = spider::pisdf::ForkVertex();
    ASSERT_EQ((spider::pisdf::ForkVertex(test2).subtype()), spider::pisdf::VertexType::FORK) << "ForkVertex::subtype() should be VertexType::FORK.";
    ASSERT_EQ((spider::pisdf::DuplicateVertex(spider::pisdf::DuplicateVertex()).subtype()), spider::pisdf::VertexType::DUPLICATE) << "DuplicateVertex::subtype() should be VertexType::DUPLICATE.";
    ASSERT_EQ((spider::pisdf::InitVertex(spider::pisdf::InitVertex()).subtype()), spider::pisdf::VertexType::INIT) << "InitVertex::subtype() should be VertexType::INIT.";
    ASSERT_EQ((spider::pisdf::EndVertex(spider::pisdf::EndVertex()).subtype()), spider::pisdf::VertexType::END) << "EndVertex::subtype() should be VertexType::END.";
    ASSERT_EQ((spider::pisdf::InputInterface(spider::pisdf::InputInterface()).subtype()), spider::pisdf::VertexType::INPUT) << "InputInterface::subtype() should be VertexType::INPUT.";
    ASSERT_EQ((spider::pisdf::OutputInterface(spider::pisdf::OutputInterface()).subtype()), spider::pisdf::VertexType::OUTPUT) << "OutputInterface::subtype() should be VertexType::OUTPUT.";
    ASSERT_EQ((spider::pisdf::Graph(spider::pisdf::Graph()).subtype()), spider::pisdf::VertexType::GRAPH) << "Graph::subtype() should be VertexType::GRAPH.";
    ASSERT_EQ((spider::pisdf::DelayVertex(spider::pisdf::DelayVertex()).subtype()), spider::pisdf::VertexType::DELAY) << "DelayVertex::subtype() should be VertexType::DELAY.";
}


TEST_F(pisdVertexTest, vertexTest) {
    {
        ASSERT_NO_THROW(spider::pisdf::ExecVertex()) << "ExecVertex() should never throw";
        ASSERT_NO_THROW(spider::pisdf::ExecVertex("", 1, 4)) << "ExecVertex() should never throw";
    }
    {
        /* == Checking init values == */
        auto *v = new spider::pisdf::ExecVertex();
        ASSERT_EQ(v->name(), "unnamed-execvertex");
        ASSERT_EQ(v->inputEdgeCount(), 0);
        ASSERT_EQ(v->outputEdgeCount(), 0);
        ASSERT_EQ(v->reference(), v);
        ASSERT_EQ(v->graph(), nullptr);
        ASSERT_EQ(v->constraints(), nullptr);
        ASSERT_EQ(v->ix(), UINT32_MAX);
        ASSERT_EQ(v->repetitionValue(), 1);
        ASSERT_EQ(v->jobIx(), UINT32_MAX);
        ASSERT_EQ(v->refinementIx(), UINT32_MAX);
        delete v;
    }
    {
        auto *v = new spider::pisdf::DelayVertex();
        ASSERT_THROW(v->setRepetitionValue(2), spider::Exception) << "DelayVertex::setRepetitionValue() should throw if value > 1";
        ASSERT_NO_THROW(v->setRepetitionValue(1)) << "DelayVertex::setRepetitionValue() should not throw if value == 1";
        delete v;
    }
    {
        auto *v = new spider::pisdf::ConfigVertex();
        ASSERT_THROW(v->setRepetitionValue(2), spider::Exception) << "ConfigVertex::setRepetitionValue() should throw if value > 1";
        ASSERT_NO_THROW(v->setRepetitionValue(1)) << "ConfigVertex::setRepetitionValue() should not throw if value == 1";
        delete v;
    }
    auto *graph = spider::make<spider::pisdf::Graph, StackID::PISDF>("graph", 4, 3, 0, 0, 0);
    auto *v0 = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("v0", 0, 1);
    auto *v1 = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("v1", 1, 0);
    auto *setter = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("setter", 0, 1);
    auto *getter = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("getter", 1, 0);
    graph->addVertex(v0);
    graph->addVertex(v1);
    graph->addVertex(setter);
    graph->addVertex(getter);
    ASSERT_EQ(v1->graph(), graph) << "Vertex::graph() bad value.";
    ASSERT_EQ(v1->ix(), 1) << "Vertex::ix() bad value.";
    ASSERT_EQ(v1->inputEdgeCount(), 1) << "Vertex::inputEdgeCount() bad value.";
    ASSERT_EQ(v1->outputEdgeCount(), 0) << "Vertex::outputEdgeCount() bad value.";
    ASSERT_EQ(v0->inputEdgeCount(), 0) << "Vertex::inputEdgeCount() bad value.";
    ASSERT_EQ(v0->outputEdgeCount(), 1) << "Vertex::outputEdgeCount() bad value.";
    ASSERT_NO_THROW(v0->createConstraints()) << "Vertex::createConstraints() should never throw.";
    ASSERT_NE(v0->createConstraints(), nullptr) << "Vertex::createConstraints() should never return nullptr.";
    ASSERT_NO_THROW(v0->setRepetitionValue(0)) << "Vertex::setRepetitionValue() should not throw for 0 value.";
    ASSERT_NO_THROW(v0->setRepetitionValue(1)) << "Vertex::setRepetitionValue() should not throw for any value.";
    ASSERT_NO_THROW(v0->setRepetitionValue(2)) << "Vertex::setRepetitionValue() should not throw for any value.";
    auto *edge = spider::make<spider::pisdf::Edge, StackID::PISDF>(v0, 0, spider::Expression(), v1, 0, spider::Expression());
    ASSERT_EQ(v0->outputEdge(0), edge) << "Vertex::connectOutputEdge() failed.";
    ASSERT_EQ(v1->inputEdge(0), edge) << "Vertex::connectInputEdge() failed.";
    ASSERT_EQ(v0->outputEdgeArray()[0], edge) << "Vertex::outputEdgeArray() failed.";
    ASSERT_EQ(v1->inputEdgeArray()[0], edge) << "Vertex::inputEdgeArray() failed.";

    /* == Test hierarchical property for every vertex == */
    testHierarchical();

    /* == Test executable property for every vertex == */
    testExecutable();

    /* == Test subtype property for every vertex == */
    testType();

    /* == Test setJobIx == */
    {
        spider::pisdf::ExecVertex vertex;
        ASSERT_EQ(vertex.jobIx(), UINT32_MAX) << "ExecVertex::jobIx() should return UINT32_MAX as default value.";
        ASSERT_NO_THROW(vertex.setJobIx(10)) << "ExecVertex::setJobIx() should never throw.";
        ASSERT_EQ(vertex.jobIx(), 10) << "ExecVertex::jobIx() bad value.";
    }

    /* == Test cpy ctor of each subtype == */
    testCpyCtor();

    auto visitor = spider::pisdf::CloneVertexVisitor(graph);
    spider::api::enableLogger(spider::log::GENERAL);
    v0->visit(&visitor);

    ASSERT_NO_THROW(v0->setName("toto")) << "Vertex::setName() should never throw.";
    ASSERT_EQ(v0->name(), "toto") << "Vertex::setName() should never throw.";
    spider::destroy(graph);
    spider::api::disableLogger(spider::log::GENERAL);
}