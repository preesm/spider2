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
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/DynamicParam.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs-tools/numerical/dependencies.h>
#include <graphs-tools/numerical/brv.h>
#include <api/spider.h>

class pisdfDepTest : public ::testing::Test {
protected:
    spider::pisdf::Graph *graph_ = nullptr;

    void SetUp() override {
        spider::createStackAllocator(spider::allocType<spider::AllocatorType::GENERIC>{ }, StackID::GENERAL, "alloc-test");
        spider::createStackAllocator(spider::allocType<spider::AllocatorType::GENERIC>{ }, StackID::EXPRESSION,
                                     "alloc-test");
        spider::createStackAllocator(spider::allocType<spider::AllocatorType::GENERIC>{ }, StackID::PISDF, "alloc-test");
        spider::createStackAllocator(spider::allocType<spider::AllocatorType::GENERIC>{ }, StackID::TRANSFO, "alloc-test");

        /* === GRAPH === */

        /* vertex_0 -> Delay -> vertex_1 -> vertex 2 */


        /* == Create a graph == */
        graph_ = spider::api::createGraph("topgraph", 15, 15, 1);

        /* == Creating vertices == */
        auto *vertex_0 = spider::api::createVertex(graph_, "vertex_0", 0, 1);
        auto *vertex_1 = spider::api::createVertex(graph_, "vertex_1", 1, 1);
        auto *vertex_2 = spider::api::createVertex(graph_, "vertex_2", 1, 1);
        auto *vertex_3 = spider::api::createVertex(graph_, "vertex_3", 1);

        /* == Creating edges == */
        auto *edge = spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 2);
        spider::api::createDelay(edge, 1);
        spider::api::createEdge(vertex_1, 0, 1, vertex_2, 0, 1);
        spider::api::createEdge(vertex_2, 0, 2, vertex_3, 0, 1);

    }

    void TearDown() override {
        spider::destroy(graph_);
        spider::quit();
    }
};

TEST_F(pisdfDepTest, consTest) {
    ASSERT_NO_THROW(spider::brv::compute(graph_));
    ASSERT_EQ(spider::pisdf::computeConsLowerDep(graph_->edges()[0]->sinkRateExpression().evaluate(),
                                                 graph_->edges()[0]->sourceRateExpression().evaluate(),
                                                 0, graph_->edges()[0]->delay()->value()), -1) << "computeConsLowerDep: edge: 1 −> d=1 -> 2 should give -1 as lower dep for instance 0";
    ASSERT_EQ(spider::pisdf::computeConsUpperDep(graph_->edges()[0]->sinkRateExpression().evaluate(),
                                                 graph_->edges()[0]->sourceRateExpression().evaluate(),
                                                 0, graph_->edges()[0]->delay()->value()), 0) << "computeConsUpperDep: edge: 1 −> d=1 -> 2 should give 0 as upper dep for instance 0";
    ASSERT_EQ(spider::pisdf::computeConsLowerDep(graph_->edges()[1]->sinkRateExpression().evaluate(),
                                                 graph_->edges()[1]->sourceRateExpression().evaluate(),
                                                 1, 0), 1) << "computeConsLowerDep: edge:  1 −> d=0 -> 1 should give 1 as lower dep for instance 1";
    ASSERT_EQ(spider::pisdf::computeConsUpperDep(graph_->edges()[1]->sinkRateExpression().evaluate(),
                                                 graph_->edges()[1]->sourceRateExpression().evaluate(),
                                                 1, 0), 1) << "computeConsUpperDep: edge:  1 −> d=0 -> 1 should give 1 as upper dep for instance 1";
}

TEST_F(pisdfDepTest, prodTest) {
//    ASSERT_EQ(spider::pisdf::computeProdLowerDep(graph_->edges()[2]->sinkRateExpression().evaluate(),
//                                                 graph_->edges()[2]->sourceRateExpression().evaluate(),
//                                                 0, 0), 0) << "computeProdLowerDep: edge:  2 −> d=0 -> 1 should give 0 as lower dep for instance 0";
//    ASSERT_EQ(spider::pisdf::computeProdUpperDep(graph_->edges()[1]->sinkRateExpression().evaluate(),
//                                                 graph_->edges()[1]->sourceRateExpression().evaluate(),
//                                                 0, 0), 1) << "computeProdUpperDep: edge:  2 −> d=0 -> 1 should give 1 as upper dep for instance 0";

}
