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

#include "RPNConverterTest.h"
#include <memory/Allocator.h>
#include <graphs-tools/expression-parser/RPNConverter.h>
#include <spider-api/config.h>
#include <cmath>

/* === Methods implementation === */

RPNConverterTest::RPNConverterTest() = default;

RPNConverterTest::~RPNConverterTest() = default;

void RPNConverterTest::SetUp() {
    AllocatorConfig cfg = AllocatorConfig();
    cfg.allocatorType = AllocatorType::FREELIST;
    cfg.size = 512;
    spider::initAllocator(StackID::GENERAL, cfg);
}

void RPNConverterTest::TearDown() {
    spider::finalizeAllocators();
}


TEST_F(RPNConverterTest, TestCreation) {
    EXPECT_THROW(spider::rpn::extractPostfixElements("4*((4+3)"), spider::Exception);
    EXPECT_THROW(spider::rpn::extractPostfixElements("4**3"), spider::Exception);
    EXPECT_THROW(spider::rpn::extractPostfixElements("4*"), spider::Exception);
    EXPECT_THROW(spider::rpn::extractPostfixElements("*4"), spider::Exception);
    EXPECT_NO_THROW(spider::rpn::extractPostfixElements(""));
    EXPECT_NO_THROW(spider::rpn::extractPostfixElements("(4)*3"));
    EXPECT_NO_THROW(spider::rpn::extractPostfixElements("4*(3)"));
}

TEST_F(RPNConverterTest, TestString) {
    ASSERT_EQ(spider::rpn::postfixString(spider::rpn::extractPostfixElements("exp(log(0.2))")), "0.2 log exp ");
    ASSERT_EQ(spider::rpn::infixString(spider::rpn::extractPostfixElements("exp(log(0.2))")), "exp(log(0.2))");
    ASSERT_EQ(spider::rpn::infixString(spider::rpn::extractPostfixElements("exp( log ( 0.2) )")), "exp(log(0.2))");
    ASSERT_EQ(spider::rpn::infixString(spider::rpn::extractPostfixElements("4cos(PI/2)")), "(4*cos((3.1415926535/2)))");
    ASSERT_EQ(spider::rpn::infixString(spider::rpn::extractPostfixElements("4max(4,cos(PI))")), "(4*max(4,cos(3.1415926535)))");
}

TEST_F(RPNConverterTest, TestGetStringFunctions) {
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
    EXPECT_THROW(spider::rpn::getOperatorTypeFromString("dummy"), spider::Exception);
}

TEST_F(RPNConverterTest, TestGetOperator) {
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
    for (std::uint32_t i = 0; i < (spider::rpn::operator_count); ++i) {
        EXPECT_NO_THROW(spider::rpn::getOperator(i));
    }
    EXPECT_THROW(spider::rpn::getOperator(-1), std::out_of_range);
}