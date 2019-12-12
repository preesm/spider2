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
#include <graphs/pisdf/DynamicParam.h>
#include <graphs/pisdf/ExecVertex.h>

class pisdfDelayTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::createStackAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::GENERAL, "alloc-test");
        spider::createStackAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::EXPRESSION, "alloc-test");
        spider::createStackAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::PISDF, "alloc-test");
    }

    void TearDown() override {
        spider::freeStackAllocators();
    }
};

TEST_F(pisdfDelayTest, delayTest) {
    ASSERT_THROW(spider::pisdf::Delay(spider::Expression(), nullptr, nullptr, 0, spider::Expression(), nullptr, 0, spider::Expression()), spider::Exception) << "Delay() should throw with nullptr edge.";
    auto *graph = spider::make<spider::pisdf::Graph, StackID::PISDF>("graph", 4, 3, 0, 0, 0);
    auto *v0 = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("v0", 0, 1);
    auto *v1 = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("v1", 1, 0);
    auto *setter = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("setter", 0, 1);
    auto *getter = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("getter", 1, 0);
    graph->addVertex(v0);
    graph->addVertex(v1);
    graph->addVertex(setter);
    graph->addVertex(getter);
    auto *edge = spider::make<spider::pisdf::Edge, StackID::PISDF>(v0, 0, spider::Expression(1), v1, 0, spider::Expression(1));
    graph->addEdge(edge);
    ASSERT_NO_THROW((spider::make<spider::pisdf::Delay, StackID::PISDF>(spider::Expression(), edge, setter, 0, spider::Expression(), nullptr, 0, spider::Expression()))) << "Delay() should not throw.";
    ASSERT_THROW(spider::pisdf::Delay(spider::Expression(), edge, nullptr, 0, spider::Expression(), getter, 0, spider::Expression(), true), spider::Exception)<< "persitent delay should not be able to have setter / getter.";
    ASSERT_THROW(spider::pisdf::Delay(spider::Expression(), edge, setter, 0, spider::Expression(), nullptr, 0, spider::Expression(), true), spider::Exception)<< "persitent delay should not be able to have setter / getter.";
    ASSERT_THROW(spider::pisdf::Delay(spider::Expression(), edge, setter, 0, spider::Expression(), getter, 0, spider::Expression()), spider::Exception) << "Edge should have only one Delay.";
    edge->removeDelay();
    ASSERT_NO_THROW((spider::make<spider::pisdf::Delay, StackID::PISDF>(spider::Expression(), edge, setter, 0, spider::Expression(), getter, 0, spider::Expression()))) << "Delay() should not throw.";
    edge->removeDelay();
    ASSERT_NO_THROW((spider::make<spider::pisdf::Delay, StackID::PISDF>(spider::Expression(), edge, nullptr, 0, spider::Expression(), getter, 0, spider::Expression()))) << "Delay() should not throw.";
    edge->removeDelay();
    auto *delay = spider::make<spider::pisdf::Delay, StackID::PISDF>(spider::Expression(10), edge, setter, 0, spider::Expression(), getter, 0, spider::Expression());
    ASSERT_EQ(delay->value(), 10) << "delay value failed";
    ASSERT_EQ(delay->name(), "delay-" +
    edge->source()->name() + "_" + std::to_string(edge->sourcePortIx()) + "--" +
    edge->sink()->name() + "_" + std::to_string(edge->sinkPortIx())) << "delay name failed";
    ASSERT_EQ(delay->edge(), edge) << "delay::edge() failed.";
    ASSERT_EQ(delay->setter(), setter) << "delay::setter() failed.";
    ASSERT_EQ(delay->getter(), getter) << "delay::setter() failed.";
    ASSERT_EQ(delay->setterPortIx(), 0) << "delay::setterPortIx() failed.";
    ASSERT_EQ(delay->getterPortIx(), 0) << "delay::getterPortIx() failed.";
    ASSERT_EQ(delay->memoryAddress(), UINT64_MAX) << "delay::memoryAddress() failed.";
    delay->setMemoryAddress(0);
    spider::api::enableLogger(spider::log::GENERAL);
    delay->setMemoryAddress(0); /* = this is just to have the warning covered... = */
    spider::api::disableLogger(spider::log::GENERAL);
    ASSERT_EQ(delay->memoryAddress(), 0) << "delay::memoryAddress() failed.";
    ASSERT_EQ(delay->isPersistent(), false) << "delay::isPersistent() failed.";
    ASSERT_NE(delay->vertex(), nullptr) << "delay::vertex() should not return nullptr.";

    graph->addParam(spider::make<spider::pisdf::DynamicParam, StackID::PISDF>("width"));
    edge->removeDelay();
    auto *delay2 = spider::make<spider::pisdf::Delay, StackID::PISDF>(spider::Expression("10width", graph->params()), edge, nullptr, 0, spider::Expression(), nullptr, 0, spider::Expression(), true);
    ASSERT_EQ(delay2->isPersistent(), true) << "delay::isPersistent() failed.";
    graph->param(0)->setValue(10);
    ASSERT_EQ(delay2->value(graph->params()), 100) << "delay value failed";
    spider::destroy(graph);
}