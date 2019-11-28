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
#include <memory/allocator.h>
#include <common/Exception.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/params/Param.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs-tools/expression-parser/Expression.h>

class expressionTest : public ::testing::Test {
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

TEST_F(expressionTest, expressionCtorTest) {
    ASSERT_NO_THROW(Expression(4)) << "Expression(int64_t) failed.";
    ASSERT_NO_THROW(Expression("")) << "Expression(std::string, {}) failed.";
    spider::pisdf::Graph *graph = spider::api::createGraph("test");
    ASSERT_THROW(Expression("width", graph->params()), spider::Exception) << "Parameterized Expression should throw when parameter is not found.";
    ASSERT_THROW(Expression("width"), spider::Exception) << "Parameterized Expression should throw when no parameter is given.";
    ASSERT_THROW(Expression("cos"), spider::Exception) << "Expression should throw when function is ill-formed.";
    ASSERT_THROW(Expression("+"), spider::Exception) << "Expression should throw when operator is missing an operand.";
    ASSERT_THROW(Expression("max(1,)"), spider::Exception) << "Expression should throw when function is missing an operand.";
    ASSERT_EQ(Expression(4).value(), 4) << "Expression evaluation failed.";
}

TEST_F(expressionTest, expression2StringTest) {
    ASSERT_EQ(Expression(4).string(), "4.000000") << "Expression to string from simple value failed.";
    ASSERT_EQ(Expression("").string(), "0.000000") << "Empty Expression should convert to 0.000000";
    ASSERT_EQ(Expression("4cos(0)").string(), "4.000000 ") << "Static Expression to string failed.";
    auto *width = spider::api::createStaticParam(nullptr, "width", 0);
    ASSERT_EQ(Expression("4cos(width)", {width}).string(), "4.000000 ") << "Static parameterized Expression to string failed";
    auto *height = spider::api::createDynamicParam(nullptr, "height");
    ASSERT_EQ(Expression("cos(height)", {width, height}).string(), "height cos ") << "Dynamic parameterized Expression to string failed.";
    ASSERT_EQ(Expression("4min(1,height)", {width, height}).string(), "4 1 height min * ") << "Dynamic parameterized Expression to string failed.";;
    spider::destroy(width);
    spider::destroy(height);
}

TEST_F(expressionTest, expressionOperatorsTest) {
    ASSERT_EQ(Expression("4*3").evaluateDBL(), 12.) << "Expression: simple multiplication failed.";
    ASSERT_EQ(Expression("4-3").evaluate(), 1) << "Expression: simple subtraction failed.";
    ASSERT_EQ(Expression("3-4").evaluate(), -1) << "Expression: negative subtraction failed.";
    ASSERT_EQ(Expression("4+ 3").evaluateDBL(), 7.) << "Expression: simple addition failed.";
    ASSERT_EQ(Expression("4/3").evaluateDBL(), 4. / 3) << "Expression: simple division failed.";
    ASSERT_EQ(Expression("4/3*3").evaluateDBL(), 4.) << "Expression: division -> multiplication priority ordering failed.";
    ASSERT_EQ(Expression("4*4/3").evaluateDBL(), 16. / 3) << "Expression: multiplication -> division priority ordering failed.";
    ASSERT_EQ(Expression("4/3").evaluate(), 1) << "Expression: division as int64_t failed.";
    ASSERT_EQ(Expression("4^3").evaluateDBL(), std::pow(4, 3)) << "Expression: power operator failed.";
    ASSERT_EQ(Expression("4+4^3").evaluateDBL(), 68) << "Expression: power -> addition priority ordering failed.";
    ASSERT_EQ(Expression("4*4^3").evaluateDBL(), 256) << "Expression: power -> multiplication priority ordering failed.";
    ASSERT_EQ(Expression("5%3").evaluateDBL(), 2) << "Expression: modulo failed.";
    ASSERT_EQ(Expression("(4*5)%3").evaluateDBL(), 2) << "Expression: (multiplication) -> modulo failed.";
    ASSERT_EQ(Expression("4*5%3").evaluateDBL(), 8) << "Expression: modulo -> multiplication priority ordering failed.";
    ASSERT_EQ(Expression("4*(5%3)").evaluateDBL(), 8) << "Expression: modulo -> multiplication priority ordering failed.";
    ASSERT_EQ(Expression("4*(3 + 5)").evaluateDBL(), 32.) << "Expression: (addition) -> multiplication priority ordering failed.";
    ASSERT_EQ(Expression("4*3 + 5").evaluateDBL(), 17.) << "Expression: multiplication -> addition priority ordering failed.";
    ASSERT_EQ(Expression("(2+2)(2 + 2)").evaluateDBL(), 16.) << "Expression: parenthesis implicit multiplication failed.";
}

TEST_F(expressionTest, stdContainersCtorTest) {
    ASSERT_NEAR(Expression("cos(pi)").evaluateDBL(), -1., 0.000001) << "Expression: cos(pi) failed.";
    ASSERT_NEAR(Expression("cos(0)").evaluateDBL(), 1., 0.000001) << "Expression: cos(0) failed.";
    ASSERT_NEAR(Expression("sin(Pi)").evaluateDBL(), 0., 0.000001) << "Expression: sin(Pi) failed.";
    ASSERT_NEAR(Expression("sin(PI/2)").evaluateDBL(), 1., 0.000001) << "Expression: sin(PI/2) failed.";
    ASSERT_NEAR(Expression("tan(4)").evaluateDBL(), Expression("sin(4) / cos(4)").evaluateDBL(), 0.000001) << "Expression: comparison tan(x) and sin(x)/cos(x) failed.";
    ASSERT_NEAR(Expression("tan((8/2))").evaluateDBL(),Expression("sin((8/2)) / cos((2^2))").evaluateDBL(),0.000001) << "Expression: comparison tan(x) and sin(x)/cos(x) failed.";
    ASSERT_NEAR(Expression("floor(1.2)").evaluateDBL(), 1., 0.000001) << "Expression: floor(x) failed.";
    ASSERT_NEAR(Expression("ceil(0.2)").evaluateDBL(), 1., 0.000001) << "Expression: ceil(x) failed.";
    ASSERT_NEAR(Expression("log(0.2)").evaluateDBL(), std::log(0.2), 0.000001) << "Expression: log(x) failed.";
    ASSERT_NEAR(Expression("log2(0.2)").evaluateDBL(), std::log2(0.2), 0.000001) << "Expression: log2(x) failed.";
    ASSERT_NEAR(Expression("4log2(0.2)").evaluateDBL(), 4 * std::log2(0.2), 0.000001) << "Expression: n*log2(x) failed.";
    ASSERT_NEAR(Expression("4cos(0.2)4").evaluateDBL(), 16 * std::cos(0.2), 0.000001) << "Expression: n*cos(x)*m failed.";
    ASSERT_NEAR(Expression("exp(0.2)").evaluateDBL(), std::exp(0.2), 0.000001)  << "Expression: exp(x) failed.";
    ASSERT_NEAR(Expression("exp(log(0.2))").evaluateDBL(), 0.2, 0.000001) << "Expression: exp(log(x)) failed.";
    ASSERT_NEAR(Expression("log(exp(0.2))").evaluateDBL(), 0.2, 0.000001) << "Expression: log(exp(x)) failed.";
    ASSERT_NEAR(Expression("sqrt(4)").evaluateDBL(), 2., 0.000001) << "Expression: sqrt(x) failed.";
    ASSERT_EQ(Expression("max(0.2, 0.21)").evaluateDBL(), 0.21) << "Expression: max(a, b) failed.";
    ASSERT_EQ(Expression("max(max(0.2,0.3*2), 0.21)").evaluateDBL(), 0.3 * 2.) << "Expression: max(max(a,b),c) failed.";
    ASSERT_EQ(Expression("min(0.2, 0.21)").evaluateDBL(), 0.2) << "Expression: min(a,b) failed.";
    ASSERT_EQ(Expression("min(min(0.2,0.1), 0.21)").evaluateDBL(), 0.1) << "Expression: min(min(a,b),c) failed.";
    ASSERT_EQ(Expression("min((0.2 + 0.1), 0.21)").evaluateDBL(), 0.21) << "Expression: min((a+b),c) failed.";
    ASSERT_EQ(Expression("min((0.2 * 0.1), 0.21)").evaluateDBL(), 0.2 * 0.1) << "Expression: min((a*b),c) failed.";
    ASSERT_EQ(Expression("min(0.2 * 0.1, 0.21)").evaluateDBL(), 0.2 * 0.1) << "Expression: min(a*b,c) failed.";
    ASSERT_EQ(Expression("min(0.2 * 0.1, 0.21)").dynamic(), false) << "Expression: dynamic() should evaluate to false for static expression.";
    spider::pisdf::Graph *graph = spider::api::createGraph("test", 0, 0, 1);
    auto *height =spider::api::createDynamicParam(graph, "height");
    ASSERT_EQ(Expression("cos(height)", graph->params()).evaluateDBL(graph->params()), 1.) << "Expression: parameterized function evaluation failed.";
    ASSERT_EQ(Expression("cos(height)", graph->params()).evaluate(graph->params()), 1) << "Expression: parameterized function evaluation to int64_t failed.";
    height->setValue(3);
    ASSERT_NEAR(Expression("cos(height)", graph->params()).evaluateDBL(graph->params()), -0.989992497, 0.001) << "Expression: parameterized function evaluation failed.";
    ASSERT_EQ(Expression("cos(height)", graph->params()).evaluate(graph->params()), 0) << "Expression: parameterized function evaluation to int64_t failed.";
    ASSERT_EQ(Expression("cos(height)", graph->params()).dynamic(), true) << "Expression: dynamic() should evaluate to true for dynamic expression.";
    spider::destroy(graph);
}