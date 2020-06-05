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
#include <memory/memory.h>
#include <common/Exception.h>
#include <graphs-tools/expression-parser/RPNConverter.h>
#include <api/spider.h>

class rpnconverterTest : public ::testing::Test {
protected:
    void SetUp() override {
        spider::start();
    }

    void TearDown() override {
        spider::quit();
    }
};

TEST_F(rpnconverterTest, rpnconverterCtorTest) {
    ASSERT_THROW(spider::rpn::extractPostfixElements("4*((4+3)"), spider::Exception)
                                << "RPNConverter: missing parenthesis ctor should throw.";
    ASSERT_THROW(spider::rpn::extractPostfixElements("4**3"), spider::Exception)
                                << "RPNConverter: double successive operators should throw.";
    ASSERT_THROW(spider::rpn::extractPostfixElements("4*"), spider::Exception)
                                << "RPNConverter: missing operand on operator should throw.";
    ASSERT_THROW(spider::rpn::extractPostfixElements("*4"), spider::Exception)
                                << "RPNConverter: missing operand on operator should throw.";
    ASSERT_NO_THROW(spider::rpn::extractPostfixElements("")) << "RPNConverter: empty string should not throw.";
    ASSERT_NO_THROW(spider::rpn::extractPostfixElements("(4)*3"))
                                << "RPNConverter: well-formed expression should not throw.";
    ASSERT_NO_THROW(spider::rpn::extractPostfixElements("4*(3)"))
                                << "RPNConverter: well-formed expression should not throw.";
}

TEST_F(rpnconverterTest, rpnconverter2StringTest) {
    ASSERT_EQ(spider::rpn::postfixString(spider::rpn::extractPostfixElements("exp(log(0.2))")), "0.2 log exp");
    ASSERT_EQ(spider::rpn::infixString(spider::rpn::extractPostfixElements("exp(log(0.2))")), "exp(log(0.2))");
    ASSERT_EQ(spider::rpn::infixString(spider::rpn::extractPostfixElements("exp( log ( 0.2) )")), "exp(log(0.2))");
    ASSERT_EQ(spider::rpn::infixString(spider::rpn::extractPostfixElements("4cos(PI/2)")), "(4*cos((3.1415926536/2)))");
    ASSERT_EQ(spider::rpn::infixString(spider::rpn::extractPostfixElements("4max(4,cos(PI))")),
              "(4*max(4,cos(3.1415926536)))");
    ASSERT_EQ(spider::rpn::infixString(spider::rpn::extractPostfixElements("e")),
              "2.7182818285");
    ASSERT_EQ(spider::rpn::infixString(spider::rpn::extractPostfixElements("4cos(E/2)")), "(4*cos((2.7182818285/2)))");
    ASSERT_EQ(spider::rpn::infixString(spider::rpn::extractPostfixElements("4max(4,cos(e))")),
              "(4*max(4,cos(2.7182818285)))");
}

TEST_F(rpnconverterTest, rpnconverterGetStringFunctionsTest) {
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("+"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("-"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("*"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("/"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("%"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("^"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("("));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString(")"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("cos"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("sin"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("tan"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("exp"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("log"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("log2"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("ceil"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("floor"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("sqrt"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("min"));
    EXPECT_NO_THROW(spider::rpn::getOperatorTypeFromString("max"));
    EXPECT_THROW(spider::rpn::getOperatorTypeFromString("foo"), spider::Exception);
}

TEST_F(rpnconverterTest, rpnconverterGetOperatorTest) {
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::ADD));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::SUB));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::MUL));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::DIV));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::MOD));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::POW));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::LEFT_PAR));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::RIGHT_PAR));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::COS));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::SIN));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::TAN));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::LOG));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::LOG2));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::EXP));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::CEIL));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::FLOOR));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::SQRT));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::MIN));
    EXPECT_NO_THROW(spider::rpn::getOperatorFromOperatorType(RPNOperatorType::MAX));
    for (uint32_t i = 0; i < (spider::rpn::OPERATOR_COUNT); ++i) {
        EXPECT_NO_THROW(spider::rpn::getOperator(i));
    }
    EXPECT_THROW(spider::rpn::getOperator(-1), std::out_of_range);
}

TEST_F(rpnconverterTest, rpnconverterReorderTest) {
    {
        auto stack = spider::rpn::extractPostfixElements("((2+w)+6)*(20)");
        ASSERT_EQ(spider::rpn::postfixString(stack), "2 w + 6 + 20 *");
        ASSERT_NO_THROW(spider::rpn::reorderPostfixStack(stack)) << "RPNConverter: stack reordering should not throw.";
        ASSERT_EQ(spider::rpn::postfixString(stack), "2 6 + w + 20 *");
    }
    {
        auto stack = spider::rpn::extractPostfixElements("((2+w)*(w+2))*(h+2)");
        ASSERT_EQ(spider::rpn::postfixString(stack), "2 w + w 2 + * h 2 + *");
        ASSERT_NO_THROW(spider::rpn::reorderPostfixStack(stack)) << "RPNConverter: stack reordering should not throw.";
        ASSERT_EQ(spider::rpn::postfixString(stack), "2 w + w 2 + * h 2 + *");
    }
    {
        auto stack = spider::rpn::extractPostfixElements("((2+w)+(w+2))*(h+2)");
        ASSERT_EQ(spider::rpn::postfixString(stack), "2 w + w 2 + + h 2 + *");
        ASSERT_NO_THROW(spider::rpn::reorderPostfixStack(stack)) << "RPNConverter: stack reordering should not throw.";
        ASSERT_EQ(spider::rpn::postfixString(stack), "2 2 + w w + + h 2 + *");
    }
    {
        auto stack = spider::rpn::extractPostfixElements("(2+w)+(w+2)*(h+2)");
        ASSERT_EQ(spider::rpn::postfixString(stack), "2 w + w 2 + h 2 + * +");
        ASSERT_NO_THROW(spider::rpn::reorderPostfixStack(stack)) << "RPNConverter: stack reordering should not throw.";
        ASSERT_EQ(spider::rpn::postfixString(stack), "2 w + w 2 + h 2 + * +");
    }
    {
        auto stack = spider::rpn::extractPostfixElements("(w*2)*(4*h)");
        ASSERT_EQ(spider::rpn::postfixString(stack), "w 2 * 4 h * *");
        ASSERT_NO_THROW(spider::rpn::reorderPostfixStack(stack)) << "RPNConverter: stack reordering should not throw.";
        ASSERT_EQ(spider::rpn::postfixString(stack), "4 2 * w h * *");
    }
    {
        auto stack = spider::rpn::extractPostfixElements("(4/w)/2");
        ASSERT_EQ(spider::rpn::postfixString(stack), "4 w / 2 /");
        ASSERT_NO_THROW(spider::rpn::reorderPostfixStack(stack)) << "RPNConverter: stack reordering should not throw.";
        ASSERT_EQ(spider::rpn::postfixString(stack), "4 2 / w /");
    }
}

