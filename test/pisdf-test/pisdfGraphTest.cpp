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
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/params/Param.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <api/spider.h>
#include <graphs/pisdf/visitors/CloneVertexVisitor.h>

class pisdfGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::createAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::GENERAL, "alloc-test");
        spider::createAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::EXPRESSION, "alloc-test");
        spider::createAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::PISDF, "alloc-test");
        spider::createAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::ARCHI, "alloc-test");
        spider::createAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::CONSTRAINTS, "alloc-test");

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
    auto *param = spider::make<spider::pisdf::Param, StackID::PISDF>("width", spider::Expression(5));
    graph->addParam(param);

    spider::destroy(graph);
}