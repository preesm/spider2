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
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/DynamicParam.h>
#include <graphs/pisdf/InHeritedParam.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <api/spider.h>
#include <graphs-tools/helper/visitors/PiSDFDefaultVisitor.h>

class pisdfVisitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::start();
    }

    void TearDown() override {
        spider::quit();
    }
};

struct TestDefaultVisitor final : public spider::pisdf::DefaultVisitor {

    void visit(spider::pisdf::Graph *) override {
        hitGraph_ = true;
    }


    void visit(spider::pisdf::ExecVertex *) override {
        hitExec_ = true;
    }

    void visit(spider::pisdf::Interface *) override {
        hitInterface_ = true;
    }

    bool hitExec_ = false;
    bool hitGraph_ = false;
    bool hitInterface_ = false;
};

TEST_F(pisdfVisitorTest, defaultTest) {
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::ExecVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "ExecVertex::visit should not throw for default visitor";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::ForkVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "ForkVertex::visit should not throw for default visitor";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::JoinVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "JoinVertex::visit should not throw for default visitor";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::HeadVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "HeadVertex::visit should not throw for default visitor";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::TailVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "TailVertex::visit should not throw for default visitor";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::RepeatVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "RepeatVertex::visit should not throw for default visitor";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::DuplicateVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "DuplicateVertex::visit should not throw for default visitor";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::DelayVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "DelayVertex::visit should not throw for default visitor";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::ConfigVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "ConfigVertex::visit should not throw for default visitor";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::Graph vertex;
        ASSERT_THROW(vertex.visit(&visitor), spider::Exception) << "DefaultVisitor should throw for graph.";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::InputInterface vertex;
        ASSERT_THROW(vertex.visit(&visitor), spider::Exception) << "DefaultVisitor should throw for input interface.";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::OutputInterface vertex;
        ASSERT_THROW(vertex.visit(&visitor), spider::Exception) << "DefaultVisitor should throw for input interface.";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::Param param("");
        ASSERT_THROW(param.visit(&visitor), spider::Exception) << "DefaultVisitor should throw for static param.";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::DynamicParam param("");
        ASSERT_THROW(param.visit(&visitor), spider::Exception) << "DefaultVisitor should throw for dynamic param.";
    }
    {
        spider::pisdf::DefaultVisitor visitor;
        spider::pisdf::Param p("");
        spider::pisdf::InHeritedParam param("", &p);
        ASSERT_THROW(param.visit(&visitor), spider::Exception) << "DefaultVisitor should throw for inherited param.";
    }
}

TEST_F(pisdfVisitorTest, defaultTest2) {
    {
        TestDefaultVisitor visitor;
        spider::pisdf::ExecVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "ExecVertex::visit should not throw for default visitor";
        ASSERT_EQ(visitor.hitExec_, true) << "ExecVertex::visit failed";
    }
    {
        TestDefaultVisitor visitor;
        spider::pisdf::ForkVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "ForkVertex::visit should not throw for default visitor";
        ASSERT_EQ(visitor.hitExec_, true) << "ForkVertex::visit failed";
    }
    {
        TestDefaultVisitor visitor;
        spider::pisdf::JoinVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "JoinVertex::visit should not throw for default visitor";
        ASSERT_EQ(visitor.hitExec_, true) << "JoinVertex::visit failed";
    }
    {
        TestDefaultVisitor visitor;
        spider::pisdf::HeadVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "HeadVertex::visit should not throw for default visitor";
        ASSERT_EQ(visitor.hitExec_, true) << "HeadVertex::visit failed";
    }
    {
        TestDefaultVisitor visitor;
        spider::pisdf::TailVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "TailVertex::visit should not throw for default visitor";
        ASSERT_EQ(visitor.hitExec_, true) << "TailVertex::visit failed";
    }
    {
        TestDefaultVisitor visitor;
        spider::pisdf::RepeatVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "RepeatVertex::visit should not throw for default visitor";
        ASSERT_EQ(visitor.hitExec_, true) << "RepeatVertex::visit failed";
    }
    {
        TestDefaultVisitor visitor;
        spider::pisdf::DuplicateVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "DuplicateVertex::visit should not throw for default visitor";
        ASSERT_EQ(visitor.hitExec_, true) << "DuplicateVertex::visit failed";
    }
    {
        TestDefaultVisitor visitor;
        spider::pisdf::DelayVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "DelayVertex::visit should not throw for default visitor";
        ASSERT_EQ(visitor.hitExec_, true) << "DelayVertex::visit failed";
    }
    {
        TestDefaultVisitor visitor;
        spider::pisdf::ConfigVertex vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "ConfigVertex::visit should not throw for default visitor";
        ASSERT_EQ(visitor.hitExec_, true) << "ConfigVertex::visit failed";
    }
    {
        TestDefaultVisitor visitor;
        spider::pisdf::Graph vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "TestDefaultVisitor should throw for graph.";
        ASSERT_EQ(visitor.hitGraph_, true) << "Graph::visit failed";
    }
    {
        TestDefaultVisitor visitor;
        spider::pisdf::InputInterface vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "TestDefaultVisitor should not throw for input interface.";
        ASSERT_EQ(visitor.hitInterface_, true) << "InputInterface::visit failed";
    }
    {
        TestDefaultVisitor visitor;
        spider::pisdf::OutputInterface vertex;
        ASSERT_NO_THROW(vertex.visit(&visitor)) << "TestDefaultVisitor should not throw for input interface.";
        ASSERT_EQ(visitor.hitInterface_, true) << "OutputInterface::visit failed";
    }
}