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
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/DynamicParam.h>
#include <graphs/pisdf/InHeritedParam.h>
#include <graphs/pisdf/visitors/DefaultVisitor.h>

class pisdfParamTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::createStackAllocator(spider::allocType<spider::AllocatorType::GENERIC>{ }, StackID::GENERAL,
                                     "alloc-test");
        spider::createStackAllocator(spider::allocType<spider::AllocatorType::GENERIC>{ }, StackID::EXPRESSION,
                                     "alloc-test");
        spider::createStackAllocator(spider::allocType<spider::AllocatorType::GENERIC>{ }, StackID::PISDF,
                                     "alloc-test");
    }

    void TearDown() override {
        spider::freeStackAllocators();
    }
};

TEST_F(pisdfParamTest, paramCreationTest) {
    {
        ASSERT_NO_THROW(spider::pisdf::Param("param", 31415)) << "Param(std::string, Graph*, int64_t) should not throw";
        ASSERT_NO_THROW(spider::pisdf::Param("param", spider::Expression(31415)))
                                    << "Param(std::string, Graph*, Expression &&) should not throw";
        ASSERT_NO_THROW(spider::pisdf::Param("param", spider::Expression("31415")))
                                    << "Param(std::string, Graph*, Expression &&) should not throw";
        ASSERT_THROW(spider::pisdf::Param("param", spider::Expression("width*31415")), spider::Exception)
                                    << "Param(std::string, Graph*, Expression &&) should throw with invalid parameter";
        auto param = spider::pisdf::Param("param", 31415);
        ASSERT_EQ(param.type(), spider::pisdf::ParamType::STATIC)
                                    << "Param::type() should be equal to spider::pisdf::ParamType::STATIC";
    }
    {
        ASSERT_NO_THROW(spider::pisdf::DynamicParam("param")) << "DynamicParam(std::string, Graph*) should not throw";
        ASSERT_NO_THROW(spider::pisdf::DynamicParam("param", spider::Expression(0)))
                                    << "DynamicParam(std::string, Graph*, Expression &&) should not throw";
        ASSERT_NO_THROW(spider::pisdf::DynamicParam("param", spider::Expression("31415")))
                                    << "DynamicParam(std::string, Graph*, Expression &&) should not throw";
        ASSERT_THROW(spider::pisdf::DynamicParam("param", spider::Expression("width*31415")), spider::Exception)
                                    << "DynamicParam(std::string, Graph*, Expression &&) should throw with invalid parameter";
        auto param = spider::pisdf::DynamicParam("param");
        ASSERT_EQ(param.expression().evaluate(), 0) << "DynamicParam::expression().evaluate() should be equal to 0";
        param.setValue(31415);
        ASSERT_EQ(param.expression().evaluate(), 31415)
                                    << "DynamicParam::expression().evaluate() should be equal to 31415";
        ASSERT_EQ(param.type(), spider::pisdf::ParamType::DYNAMIC)
                                    << "DynamicParam::type() should be equal to spider::pisdf::ParamType::DYNAMIC";
    }
    {
        auto param = spider::pisdf::Param("param", 31415);
        ASSERT_NO_THROW(spider::pisdf::InHeritedParam("param", &param))
                                    << "InHeritedParam(std::string, Graph*, Param *) should not throw";
        ASSERT_THROW(spider::pisdf::InHeritedParam("param", nullptr), spider::Exception)
                                    << "InHeritedParam(std::string, Graph*, Param *) should throw with nullptr parent.";
    }
    {
        auto param = spider::pisdf::DynamicParam("width");
        ASSERT_THROW(spider::pisdf::Param("param", spider::Expression("width*31415", { &param })), spider::Exception)
                                    << "Param(std::string, Graph*, Expression &&) should throw with dynamic parameter";
    }
}

struct ParamVisitorTest final : public spider::pisdf::DefaultVisitor {
    int type = -1;

    void visit(spider::pisdf::Param *) override {
        type = 0;
    }

    void visit(spider::pisdf::DynamicParam *) override {
        type = 1;
    }

    void visit(spider::pisdf::InHeritedParam *) override {
        type = 2;
    }
};

TEST_F(pisdfParamTest, paramTest) {
    {
        auto param = spider::pisdf::Param("param", 31415);
        ASSERT_THROW(param.setValue(272), spider::Exception) << "Static param should throw when calling setValue()";
        auto visitor = ParamVisitorTest();
        param.visit(&visitor);
        ASSERT_EQ(visitor.type, 0) << "Static param visitor failed.";
    }
    {
        auto *graph = new spider::pisdf::Graph();
        auto *param = spider::make<spider::pisdf::Param>("PArAM", 31415);
        ASSERT_EQ(param->name(), "param") << "name of param should be lower case converted.";
        ASSERT_NE(param->graph(), graph) << "param::graph() failed";
        ASSERT_EQ(param->ix(), UINT32_MAX) << "param::ix() should be equal to UINT32_MAX on init.";
        graph->addParam(param);
        ASSERT_EQ(param->graph(), graph) << "param::setGraph() failed";
        ASSERT_EQ(param->ix(), 0) << "param::ix() failed";
        delete graph;
    }
    {
        auto param = spider::pisdf::Param("param", 31415);
        auto param2 = spider::pisdf::InHeritedParam("param", &param);
        ASSERT_EQ(param2.parent(), &param) << "InheritedParam::parent() failed.";
        ASSERT_EQ(param2.dynamic(), false) << "InheritedParam with static parent should be static.";
        ASSERT_EQ(param2.value(), param.value()) << "InheritedParam should have same value as parent.";
        ASSERT_EQ(param2.type(), spider::pisdf::ParamType::INHERITED)
                                    << "InHeritedParam::type() should be equal to spider::pisdf::ParamType::INHERITED";
        auto visitor = ParamVisitorTest();
        param2.visit(&visitor);
        ASSERT_EQ(visitor.type, 2) << "Inherited param visitor failed.";
    }
    {
        auto visitor = ParamVisitorTest();
        auto param = spider::pisdf::DynamicParam("param");
        param.visit(&visitor);
        ASSERT_EQ(visitor.type, 1) << "Dynamic param visitor failed.";
    }
}

/* === Function(s) definition === */
