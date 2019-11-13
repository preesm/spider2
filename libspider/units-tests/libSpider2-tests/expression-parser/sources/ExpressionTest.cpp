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

/* === Includes === */

#include "ExpressionTest.h"
#include <memory/Allocator.h>
#include <graphs-tools/expression-parser/Expression.h>
#include <spider-api/config.h>
#include <spider-api/pisdf.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <cmath>

/* === Methods implementation === */

ExpressionTest::ExpressionTest() = default;

ExpressionTest::~ExpressionTest() = default;

void ExpressionTest::SetUp() {
    AllocatorConfig cfg = AllocatorConfig();
    cfg.allocatorType = AllocatorType::FREELIST;
    cfg.size = 4096;
    Spider::initAllocator(StackID::GENERAL, cfg);
    Spider::initAllocator(StackID::EXPR_PARSER, cfg);
    Spider::initAllocator(StackID::PISDF, cfg);
}

void ExpressionTest::TearDown() {
    Spider::finalizeAllocators();
}

TEST_F(ExpressionTest, TestCreation) {
    EXPECT_NO_THROW(Expression(4));
    EXPECT_NO_THROW(Expression(""));
    Spider::PiSDF::Graph *graph = Spider::API::createGraph("test");
    EXPECT_THROW(Expression("width", graph->params()), Spider::Exception);
    EXPECT_THROW(Expression("width"), Spider::Exception);
    EXPECT_THROW(Expression("cos"), Spider::Exception);
    EXPECT_THROW(Expression("+"), Spider::Exception);
    EXPECT_THROW(Expression("max(1,)"), Spider::Exception);
    ASSERT_EQ(Expression(4).value(), 4);
}

TEST_F(ExpressionTest, TestString) {
    ASSERT_EQ(Expression(4).string(), "4.000000");
    ASSERT_EQ(Expression("").string(), "0.000000");
    ASSERT_EQ(Expression("4cos(0)").string(), "4.000000 ");
    auto *width = Spider::API::createStaticParam(nullptr, "width", 0);
    ASSERT_EQ(Expression("4cos(width)", {width}).string(), "4.000000 ");
    auto *height = Spider::API::createDynamicParam(nullptr, "height");
    ASSERT_EQ(Expression("cos(height)", {width, height}).string(), "height cos ");
    ASSERT_EQ(Expression("4min(1,height)", {width, height}).string(), "4 1 height min * ");
    Spider::destroy(width);
    Spider::deallocate(width);
    Spider::destroy(height);
    Spider::deallocate(height);
}

TEST_F(ExpressionTest, TestEvaluationOperators) {
    ASSERT_EQ(Expression("4*3").evaluateDBL(), 12.);
    ASSERT_EQ(Expression("4-3").evaluate(), 1);
    ASSERT_EQ(Expression("3-4").evaluate(), -1);
    ASSERT_EQ(Expression("4+ 3").evaluateDBL(), 7.);
    ASSERT_EQ(Expression("4/3").evaluateDBL(), 4. / 3);
    ASSERT_EQ(Expression("4/3*3").evaluateDBL(), 4.);
    ASSERT_EQ(Expression("4*4/3").evaluateDBL(), 16. / 3);
    ASSERT_EQ(Expression("4/3").evaluate(), 1);
    ASSERT_EQ(Expression("4^3").evaluateDBL(), std::pow(4, 3));
    ASSERT_EQ(Expression("4+4^3").evaluateDBL(), 68);
    ASSERT_EQ(Expression("4*4^3").evaluateDBL(), 256);
    ASSERT_EQ(Expression("5%3").evaluateDBL(), 2);
    ASSERT_EQ(Expression("(4*5)%3").evaluateDBL(), 2);
    ASSERT_EQ(Expression("4*5%3").evaluateDBL(), 8);
    ASSERT_EQ(Expression("4*(5%3)").evaluateDBL(), 8);
    ASSERT_EQ(Expression("4*(3 + 5)").evaluateDBL(), 32.);
    ASSERT_EQ(Expression("4*3 + 5").evaluateDBL(), 17.);
    ASSERT_EQ(Expression("(2+2)(2 + 2)").evaluateDBL(), 16.);
}

TEST_F(ExpressionTest, TestEvaluationFunctions) {
    ASSERT_NEAR(Expression("cos(pi)").evaluateDBL(), -1., 0.000001);
    ASSERT_NEAR(Expression("cos(0)").evaluateDBL(), 1., 0.000001);
    ASSERT_NEAR(Expression("sin(Pi)").evaluateDBL(), 0., 0.000001);
    ASSERT_NEAR(Expression("sin(PI/2)").evaluateDBL(), 1., 0.000001);
    ASSERT_NEAR(Expression("tan(4)").evaluateDBL(), Expression("sin(4) / cos(4)").evaluateDBL(),
                0.000001);
    ASSERT_NEAR(Expression("tan((8/2))").evaluateDBL(),
                Expression("sin((8/2)) / cos((2^2))").evaluateDBL(),
                0.000001);
    ASSERT_NEAR(Expression("floor(1.2)").evaluateDBL(), 1., 0.000001);
    ASSERT_NEAR(Expression("ceil(0.2)").evaluateDBL(), 1., 0.000001);
    ASSERT_NEAR(Expression("log(0.2)").evaluateDBL(), std::log(0.2), 0.000001);
    ASSERT_NEAR(Expression("log2(0.2)").evaluateDBL(), std::log2(0.2), 0.000001);
    ASSERT_NEAR(Expression("4log2(0.2)").evaluateDBL(), 4 * std::log2(0.2), 0.000001);
    ASSERT_NEAR(Expression("4cos(0.2)4").evaluateDBL(), 16 * std::cos(0.2), 0.000001);
    ASSERT_NEAR(Expression("exp(0.2)").evaluateDBL(), std::exp(0.2), 0.000001);
    ASSERT_NEAR(Expression("exp(log(0.2))").evaluateDBL(), 0.2, 0.000001);
    ASSERT_NEAR(Expression("log(exp(0.2))").evaluateDBL(), 0.2, 0.000001);
    ASSERT_NEAR(Expression("sqrt(4)").evaluateDBL(), 2., 0.000001);
    ASSERT_EQ(Expression("max(0.2, 0.21)").evaluateDBL(), 0.21);
    ASSERT_EQ(Expression("max(max(0.2,0.3*2), 0.21)").evaluateDBL(), 0.3 * 2.);
    ASSERT_EQ(Expression("min(min(0.2,0.1), 0.21)").evaluateDBL(), 0.1);
    ASSERT_EQ(Expression("min(0.2, 0.21)").evaluateDBL(), 0.2);
    ASSERT_EQ(Expression("min((0.2 + 0.1), 0.21)").evaluateDBL(), 0.21);
    ASSERT_EQ(Expression("min((0.2 * 0.1), 0.21)").evaluateDBL(), 0.2 * 0.1);
    ASSERT_EQ(Expression("min(0.2 * 0.1, 0.21)").evaluateDBL(), 0.2 * 0.1);
    ASSERT_EQ(Expression("min(0.2 * 0.1, 0.21)").dynamic(), false);
    Spider::PiSDF::Graph *graph = Spider::API::createGraph("test", 0, 0, 1);
    Spider::API::createDynamicParam(graph, "height");
    ASSERT_EQ(Expression("cos(height)", graph->params()).evaluateDBL(graph->params()), 1.);
    ASSERT_EQ(Expression("cos(height)", graph->params()).evaluate(graph->params()), 1);
    ASSERT_EQ(Expression("cos(height)", graph->params()).dynamic(), true);
    Spider::destroy(graph);
    Spider::deallocate(graph);
}
