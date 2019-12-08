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
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <api/spider.h>
#include <graphs-tools/transformation/optims/PiSDFInitEndOptimizer.h>

class pisdfOptimsTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::createStackAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::GENERAL, "alloc-test");
        spider::createStackAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::EXPRESSION, "alloc-test");
        spider::createStackAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::PISDF, "alloc-test");
        spider::createStackAllocator(spider::type<spider::AllocatorType::GENERIC>{ }, StackID::TRANSFO, "alloc-test");
        spider::api::enableVerbose();
        spider::api::enableLogger(spider::log::OPTIMS);
    }

    void TearDown() override {
        spider::api::disableVerbose();
        spider::api::disableLogger(spider::log::OPTIMS);
        spider::quit();
    }
};


TEST_F(pisdfOptimsTest, initEndTest) {
    auto *graph = spider::api::createGraph("graph", 2, 1);
    auto *init = spider::api::createInit(graph, "init");
    auto *end = spider::api::createEnd(graph, "end");
    spider::api::createEdge(init, 0, 1, end, 0, 1);
    ASSERT_EQ(graph->vertexCount(), 2);
    ASSERT_NO_THROW(PiSDFInitEndOptimizer()(graph));
    ASSERT_EQ(graph->vertexCount(), 0);
    spider::destroy(graph);
}