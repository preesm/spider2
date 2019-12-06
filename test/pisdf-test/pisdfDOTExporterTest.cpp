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
#include <graphs/pisdf/params/Param.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs-tools/exporter/DOTExporter.h>
#include <api/spider.h>

class pisdfDotExporterTest : public ::testing::Test {
protected:
    spider::pisdf::Graph *graph_ = nullptr;

    void SetUp() override {
        spider::createAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::GENERAL, "alloc-test");
        spider::createAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::EXPRESSION, "alloc-test");
        spider::createAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::PISDF, "alloc-test");


        /* === GRAPH === */
        /* vertex_0 -> D -> vertex_1 -> subgraph -> vertex_4
         *                             /        \
         *                            /          \
         *               _________________________________________
         *              |                                         |
         *              | input -> vertex_2 -> vertex_3 -> output |
         *              |      cfg /          /        \          |
         *              |__________________  /          \ ________|
         *                                  /            \
         *               _________________________________________
         *              |                                         |
         *              |   in2 -> vertex_2 -> vertex_3 -> out2   |
         *              |_________________________________________|
         */



        /* == Create a graph == */
        graph_ = spider::api::createGraph("topgraph", 15, 15, 1);

        /* == Creating vertices == */
        auto *vertex_0 = spider::api::createVertex(graph_, "vertex_0", 0, 1);
        auto *vertex_1 = spider::api::createVertex(graph_, "vertex_1", 1, 1);
        auto *vertex_4 = spider::api::createVertex(graph_, "vertex_4", 1);
        auto *subgraph = spider::api::createSubraph(graph_, "subgraph", 3, 4, 2, 1, 1);
        auto *cfg = spider::api::createConfigActor(subgraph, "cfg", 0, 1);
        auto *input = spider::api::setInputInterfaceName(subgraph, 0, "input");
        auto *output = spider::api::setOutputInterfaceName(subgraph, 0, "output");
        auto *vertex_2 = spider::api::createVertex(subgraph, "vertex_2", 2, 1);
        auto *vertex_3 = spider::api::createSubraph(subgraph, "vertex_3", 1, 4,1, 1, 1);
        auto *in2 = spider::api::setInputInterfaceName(vertex_3, 0, "in2");
        auto *out2 = spider::api::setOutputInterfaceName(vertex_3, 0, "out2");
        auto *fork = spider::api::createFork(vertex_3, "fork", 2);
        auto *join = spider::api::createJoin(vertex_3, "join", 2);
        auto *head = spider::api::createHead(vertex_3, "head", 1);
        auto *tail = spider::api::createTail(vertex_3, "tail", 1);
        auto *repeat = spider::api::createRepeat(vertex_3, "repeat");
        auto *duplicate = spider::api::createDuplicate(vertex_3, "duplicate", 1);

        /* == Creating edges == */
        auto *edge = spider::api::createEdge(vertex_0, 0, 1, vertex_1, 0, 1);
        spider::api::createDelay(edge, 1);
        spider::api::createEdge(vertex_1, 0, "1", subgraph, 0, "1");
        spider::api::createEdge(input, 0, 5, vertex_2, 0, 1);
        spider::api::createEdge(vertex_2, 0, 1, vertex_3, 0, 5);
        spider::api::createEdge(vertex_3, 0, 1, output, 0, 1);
        spider::api::createEdge(subgraph, 0, 5, vertex_4, 0, 5);
        spider::api::createEdge(cfg, 0, 15, vertex_2, 1, 1);
        spider::api::createEdge(in2, 0, 5, fork, 0, 5);
        spider::api::createEdge(fork, 0, 3, head, 0, 3);
        spider::api::createEdge(fork, 1, 2, tail, 0, 2);
        spider::api::createEdge(head, 0, 3, join, 0, 3);
        spider::api::createEdge(tail, 0, 2, join, 1, 2);
        spider::api::createEdge(join, 0, 5, duplicate, 0, 5);
        spider::api::createEdge(duplicate, 0, 5, repeat, 0, 15);
        spider::api::createEdge(repeat, 0, 5, out2, 0, 15);

        /* == Creating param == */
        auto *param = spider::api::createStaticParam(graph_, "width", 10);
        spider::api::createInheritedParam(subgraph, "top-width", param);
        spider::api::createStaticParam(subgraph, "height", 10);
        auto *param2 =spider::api::createDynamicParam(subgraph, "width");
        spider::api::createInheritedParam(vertex_3, "up-width", param2);

    }

    void TearDown() override {
        spider::destroy(graph_);
        spider::quit();
    }
};

TEST_F(pisdfDotExporterTest, dotTest) {
    ASSERT_NO_THROW(spider::pisdf::DOTExporter{ graph_ }) << "DOTExporter::DOTExporter() failed";
    ASSERT_NO_THROW(spider::pisdf::DOTExporter{ graph_ }.print()) << "DOTExporter::print() failed";
    ASSERT_NO_THROW(spider::pisdf::DOTExporter{ graph_ }.print("./dot.dot")) << "DOTExporter::print(path) failed with valid path";
    ASSERT_THROW(spider::pisdf::DOTExporter{ graph_ }.print("XXX://INVALID_PATH"), spider::Exception) << "DOTExporter::print(path) should throw with invalid path";
}
