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
#include <graphs/pisdf/common/Types.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs/pisdf/ExecVertex.h>

class pisdEdgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::createAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::GENERAL, "alloc-test");
        spider::createAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::EXPRESSION, "alloc-test");
        spider::createAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::PISDF, "alloc-test");
    }

    void TearDown() override {
        spider::freeAllocators();
    }
};

TEST_F(pisdEdgeTest, edgeTest) {
    auto *graph = spider::make<spider::pisdf::Graph, StackID::PISDF>("graph", 4, 3, 0, 0, 0);
    auto *v0 = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("v0", 0, 1);
    auto *v1 = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("v1", 1, 0);
    auto *setter = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("setter", 0, 1);
    auto *getter = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("getter", 1, 0);
    graph->addVertex(v0);
    graph->addVertex(setter);
    graph->addVertex(getter);
    ASSERT_THROW(spider::pisdf::Edge(nullptr, 0, spider::Expression(), v1, 0, spider::Expression()), spider::Exception) << "Edge(nullptr, ..) should throw";
    ASSERT_THROW(spider::pisdf::Edge(v0, 0, spider::Expression(), nullptr, 0, spider::Expression()), spider::Exception) << "Edge(.., nullptr) should throw";
    ASSERT_THROW(spider::pisdf::Edge(v0, 0, spider::Expression(), v1, 0, spider::Expression()), spider::Exception) << "Edge(.., ..) with different graph should throw";
    graph->addVertex(v1);
    ASSERT_NO_THROW(spider::pisdf::Edge(v0, 0, spider::Expression(), v1, 0, spider::Expression())) << "Edge(.., ..) should not throw";
    auto *edge = spider::make<spider::pisdf::Edge, StackID::PISDF>(v0, 0, spider::Expression(), v1, 0, spider::Expression());
    graph->addEdge(edge);
    ASSERT_THROW(edge->setSource(nullptr, 0, spider::Expression()), spider::Exception) << "Edge::setSource() should throw on nullptr";
    ASSERT_THROW(edge->setSink(nullptr, 0, spider::Expression()), spider::Exception) << "Edge::setSink() should throw on nullptr";
    ASSERT_EQ(edge->source(), v0) << "Edge::source() failed.";
    ASSERT_EQ(edge->sink(), v1) << "Edge::sink() failed.";
    ASSERT_EQ(edge->sourceFw(), v0) << "Edge::sourceFw() failed.";
    ASSERT_EQ(edge->sinkFw(), v1) << "Edge::sinkFw() failed.";
    ASSERT_EQ(edge->sourcePortIx(), 0) << "Edge::sourcePortIx() failed.";
    ASSERT_EQ(edge->sinkPortIx(), 0) << "Edge::sinkPortIx() failed.";
    ASSERT_NO_THROW(edge->setSource(setter, 0, spider::Expression())) << "Edge::setSource() should not throw on valid call.";
    ASSERT_NO_THROW(edge->setSink(getter, 0, spider::Expression())) << "Edge::setSource() should not throw on valid call.";
    ASSERT_EQ(edge->source(), setter) << "Edge::source() failed.";
    ASSERT_EQ(edge->sink(), getter) << "Edge::sink() failed.";
    ASSERT_NO_THROW(edge->setSource(v0, 0, spider::Expression())) << "Edge::setSource() should not throw on valid call.";
    ASSERT_NO_THROW(edge->setSink(v1, 0, spider::Expression())) << "Edge::setSource() should not throw on valid call.";

    ASSERT_EQ(edge->delay(), nullptr) << "delay should be nullptr on init.";
    auto *delay = spider::make<spider::pisdf::Delay, StackID::PISDF>(spider::Expression(10), edge, setter, 0, spider::Expression(), getter, 0, spider::Expression());
    ASSERT_EQ(edge->delay(), delay) << "delay should be set automatically on Edge.";
    ASSERT_EQ(edge->sourceRateExpression().value(), spider::Expression().value());
    ASSERT_EQ(edge->sinkRateExpression().value(), spider::Expression().value());
    ASSERT_NO_THROW(edge->setDelay(nullptr)) << "Edge::setDelay() with nullptr should not throw.";
    ASSERT_THROW(edge->setDelay(delay), spider::Exception);

    spider::destroy(graph);
}
