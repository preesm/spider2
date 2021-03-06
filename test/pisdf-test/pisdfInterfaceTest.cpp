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
#include <graphs/pisdf/Vertex.h>
#include <graphs-tools/helper/visitors/PiSDFDefaultVisitor.h>
#include <api/spider.h>

class pisdfInterfaceTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::start();
    }

    void TearDown() override {
        spider::quit();
    }
};

/* === Function(s) definition === */

TEST_F(pisdfInterfaceTest, creationTest) {
    {
        for (auto type : spider::EnumIterator<spider::pisdf::VertexType>{ }) {
            if ((type != spider::pisdf::VertexType::INPUT) &&
                (type != spider::pisdf::VertexType::OUTPUT)) {
                ASSERT_THROW(spider::pisdf::Interface(type, ""), spider::Exception)
                                            << "Interface() should throw for any type other than INPUT / OUTPUT";
            }
        }

        ASSERT_NO_THROW(spider::pisdf::Interface(spider::pisdf::VertexType::INPUT, ""))
                                    << "Interface() should not throw";
        ASSERT_NO_THROW(spider::pisdf::Interface(spider::pisdf::VertexType::INPUT, "input"))
                                    << "Interface(std::string) should not throw";
        ASSERT_NO_THROW(spider::pisdf::Interface(spider::pisdf::VertexType::OUTPUT, ""))
                                    << "Interface() should not throw";
        ASSERT_NO_THROW(spider::pisdf::Interface(spider::pisdf::VertexType::OUTPUT, "output"))
                                    << "Interface(std::string) should not throw";
    }
}

struct InterfaceVisitorTest final : public spider::pisdf::DefaultVisitor {
    int type = -1;

    void visit(spider::pisdf::Interface *i) override {
        type = i->subtype() == spider::pisdf::VertexType::OUTPUT;
    }
};

TEST_F(pisdfInterfaceTest, usageTest) {
    auto *graph = spider::make<spider::pisdf::Graph, StackID::PISDF>("graph", 1, 2, 0, 1, 1);
    auto *vertex = spider::make<spider::pisdf::Vertex, StackID::PISDF>(spider::pisdf::VertexType::NORMAL, "vertex", 1,
                                                                       1);
    graph->addVertex(vertex);
    auto *input = graph->inputInterface(0);
    auto *output = graph->outputInterface(0);
    graph->addEdge(spider::make<spider::pisdf::Edge, StackID::PISDF>(input, 0, spider::Expression(1),
                                                                     vertex, 0, spider::Expression(1)));
    graph->addEdge(spider::make<spider::pisdf::Edge, StackID::PISDF>(vertex, 0, spider::Expression(1),
                                                                     output, 0,
                                                                     spider::Expression(1)));
    ASSERT_EQ(input->opposite(), vertex) << "opposite of input interface failed.";
    ASSERT_EQ(output->opposite(), vertex) << "opposite of output interface failed.";
    ASSERT_EQ(input->subtype(), spider::pisdf::VertexType::INPUT) << "input interface subtype failed";
    ASSERT_EQ(output->subtype(), spider::pisdf::VertexType::OUTPUT) << "output interface subtype failed";
    auto *top = new spider::pisdf::Graph("top", 3, 2, 0);
    top->addVertex(graph);
    auto *v1 = spider::make<spider::pisdf::Vertex, StackID::PISDF>(spider::pisdf::VertexType::NORMAL, "v1", 0, 1);
    top->addVertex(v1);
    auto *v2 = spider::make<spider::pisdf::Vertex, StackID::PISDF>(spider::pisdf::VertexType::NORMAL, "v2", 1, 0);
    top->addVertex(v2);
    auto *e0 = spider::make<spider::pisdf::Edge, StackID::PISDF>(v1, 0, spider::Expression(1), graph, 0,
                                                                 spider::Expression(1));
    top->addEdge(e0);
    auto *e1 = spider::make<spider::pisdf::Edge, StackID::PISDF>(graph, 0, spider::Expression(1), v2, 0,
                                                                 spider::Expression(1));
    top->addEdge(e1);
    ASSERT_EQ(input->inputEdge(), e0) << "inputEdge of input interface failed";
    ASSERT_EQ(output->outputEdge(), e1) << "outputEdge of output interface failed";
    ASSERT_EQ(input->outputEdge(), graph->edges()[0].get()) << "outputEdge of input interface failed";
    ASSERT_EQ(output->inputEdge(), graph->edges()[1].get()) << "inputEdge of input interface failed";
    ASSERT_THROW(input->connectInputEdge(nullptr, 0), spider::Exception)
                                << "input interface can not have input edge connected to it.";
    ASSERT_THROW(output->connectOutputEdge(nullptr, 0), spider::Exception)
                                << "output interface can not have output edge connected to it.";
    {
        auto visitor = InterfaceVisitorTest();
        input->visit(&visitor);
        ASSERT_EQ(visitor.type, 0) << "input interface visit failed";
        output->visit(&visitor);
        ASSERT_EQ(visitor.type, 1) << "output interface visit failed";
    }
    {
        auto visitor = spider::pisdf::DefaultVisitor();
        ASSERT_THROW(input->visit(&visitor), spider::Exception) << "DefaultVisitor should throw for input interface";
        ASSERT_THROW(output->visit(&visitor), spider::Exception) << "DefaultVisitor should throw for output interface";
    }
    delete top;
}