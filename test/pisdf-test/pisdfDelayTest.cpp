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
#include <graphs/pisdf/DynamicParam.h>
#include <graphs/pisdf/ExecVertex.h>
#include <archi/MemoryInterface.h>
#include <api/spider.h>

class pisdfDelayTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::start();

        spider::api::createPlatform(1, 1);

        memoryInterface_ = spider::api::createMemoryInterface(1024 * 1024 * 1024);

        auto *x86Cluster = spider::api::createCluster(1, memoryInterface_);

        auto x86PECore0 = spider::api::createProcessingElement(0, 0, x86Cluster, "x86-Core0", spider::PEType::LRT, 0);
        spider::api::setSpiderGRTPE(x86PECore0);

        graph_ = spider::make<spider::pisdf::Graph, StackID::PISDF>("graph", 4, 3, 0, 0, 0);
        auto *v0 = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("v0", 0, 1);
        auto *v1 = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("v1", 1, 0);
        auto *setter = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("setter", 0, 1);
        auto *getter = spider::make<spider::pisdf::ExecVertex, StackID::PISDF>("getter", 1, 0);
        graph_->addVertex(v0);
        graph_->addVertex(v1);
        graph_->addVertex(setter);
        graph_->addVertex(getter);
        edge_ = spider::make<spider::pisdf::Edge, StackID::PISDF>(v0, 0, spider::Expression(1), v1, 0,
                                                                  spider::Expression(1));
        graph_->addEdge(edge_);
    }

    void TearDown() override {
        spider::destroy(graph_);
        spider::quit();
    }

    spider::pisdf::Graph *graph_ = nullptr;
    spider::pisdf::Edge *edge_ = nullptr;
    spider::MemoryInterface *memoryInterface_;
};

TEST_F(pisdfDelayTest, delayPersistentTest) {
    spider::pisdf::Delay *delay = nullptr;
    ASSERT_NO_THROW((delay = spider::make<spider::pisdf::Delay, StackID::PISDF>(10, edge_)));
    memoryInterface_->allocate(0, 10);
    ASSERT_NO_THROW(delay->setMemoryAddress(0));
    ASSERT_NO_THROW(delay->setMemoryAddress(10)); // just for the warning to be covered
    ASSERT_NO_THROW(delay->setMemoryInterface(memoryInterface_));
}

TEST_F(pisdfDelayTest, delayCtorTest0) {
    ASSERT_THROW(spider::pisdf::Delay(0, nullptr, nullptr, 0, spider::Expression(), nullptr, 0,
                                      spider::Expression()), spider::Exception)
                                << "Delay() should throw with nullptr edge.";
}

TEST_F(pisdfDelayTest, delayCtorTest1) {
    auto *setter = graph_->vertex(2);
    ASSERT_NO_THROW((spider::make<spider::pisdf::Delay, StackID::PISDF>(0,
                                                                        edge_,
                                                                        setter, 0,
                                                                        spider::Expression(),
                                                                        nullptr,
                                                                        0,
                                                                        spider::Expression(), false)))
                                << "Delay::Delay() should not throw with valid parameters.";
}

TEST_F(pisdfDelayTest, delayCtorTest2) {
    auto *getter = graph_->vertex(3);
    ASSERT_THROW((spider::make_unique<spider::pisdf::Delay, StackID::PISDF>(0,
                                                                            edge_,
                                                                            nullptr, 0,
                                                                            spider::Expression(),
                                                                            getter,
                                                                            0,
                                                                            spider::Expression(), true)),
                 spider::Exception)
                                << "Delay::Delay() should fail. Persistent delays can not have getter nor setter.";
    auto *setter = graph_->vertex(2);
    ASSERT_THROW((spider::make_unique<spider::pisdf::Delay, StackID::PISDF>(0,
                                                                            edge_,
                                                                            setter, 0,
                                                                            spider::Expression(),
                                                                            nullptr,
                                                                            0,
                                                                            spider::Expression(), true)),
                 spider::Exception)
                                << "Delay::Delay() should throw. Persistent delays can not have getter nor setter.";
}

TEST_F(pisdfDelayTest, delayCtorTest3) {
    auto *setter = graph_->vertex(2);
    auto *getter = graph_->vertex(3);
    ASSERT_NO_THROW((spider::make<spider::pisdf::Delay, StackID::PISDF>(0,
                                                                        edge_,
                                                                        setter, 0,
                                                                        spider::Expression(),
                                                                        getter,
                                                                        0,
                                                                        spider::Expression(), false)))
                                << "Delay::Delay() should not throw with valid parameters.";
    ASSERT_THROW((spider::pisdf::Delay(0, edge_, setter, 0, spider::Expression(), getter, 0,
                                       spider::Expression())), spider::Exception)
                                << "Delay::Delay() should throw. Edge can only have one Delay.";
}

TEST_F(pisdfDelayTest, delayValueNameTest) {
    auto *setter = graph_->vertex(2);
    auto *getter = graph_->vertex(3);
    spider::pisdf::Delay *delay = nullptr;
    ASSERT_NO_THROW((delay = spider::make<spider::pisdf::Delay, StackID::PISDF>(10,
                                                                                edge_,
                                                                                setter, 0,
                                                                                spider::Expression(),
                                                                                getter,
                                                                                0,
                                                                                spider::Expression(), false)))
                                << "Delay::Delay() should not throw with valid parameters.";
    ASSERT_EQ(delay->value(), 10) << "Delay::value() error. Value should be 10";
    ASSERT_EQ(delay->name(), "delay::" +
                             edge_->source()->name() + ":" + std::to_string(edge_->sourcePortIx()) + "--" +
                             edge_->sink()->name() + ":" + std::to_string(edge_->sinkPortIx()))
                                << "Delay::name() error.";
    ASSERT_EQ(delay->edge(), edge_) << "Delay::edge() failed.";
    ASSERT_EQ(delay->setter(), setter) << "Delay::setter() failed.";
    ASSERT_EQ(delay->getter(), getter) << "Delay::setter() failed.";
    ASSERT_EQ(delay->setterPortIx(), 0) << "Delay::setterPortIx() failed.";
    ASSERT_EQ(delay->getterPortIx(), 0) << "Delay::getterPortIx() failed.";
    ASSERT_EQ(delay->memoryAddress(), UINT64_MAX) << "Delay::memoryAddress() failed.";
    ASSERT_NO_THROW(delay->setMemoryAddress(0)); // nothing should happen
    spider::api::enableLogger(spider::log::GENERAL);
    spider::api::disableLogger(spider::log::GENERAL);
    ASSERT_EQ(delay->memoryAddress(), UINT64_MAX) << "Delay::memoryAddress() failed.";
    ASSERT_EQ(delay->isPersistent(), false) << "Delay::isPersistent() failed.";
    ASSERT_NE(delay->vertex(), nullptr) << "Delay::vertex() should not return nullptr.";
}