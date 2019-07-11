/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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
#include <common/memory/Allocator.h>
#include <common/expression-parser/RPNConverter.h>
#include <spider-api/general.h>
#include <cmath>

/* === Methods implementation === */

RPNConverterTest::RPNConverterTest() {
}

RPNConverterTest::~RPNConverterTest() {
}

void RPNConverterTest::SetUp() {
    AllocatorConfig cfg = AllocatorConfig();
    cfg.allocatorType = AllocatorType::FREELIST;
    cfg.size = 512;
    Spider::initAllocator(StackID::GENERAL, cfg);
}

void RPNConverterTest::TearDown() {
    Spider::finalizeAllocators();
}


TEST_F(RPNConverterTest, TestCreation) {
    EXPECT_THROW(RPNConverter("4*((4+3)", nullptr), SpiderException);
    EXPECT_THROW(RPNConverter("4**3", nullptr), SpiderException);
    EXPECT_THROW(RPNConverter("", nullptr), SpiderException);
    EXPECT_NO_THROW(RPNConverter("(4)*3", nullptr));
    EXPECT_NO_THROW(RPNConverter("4*(3)", nullptr));
}

TEST_F(RPNConverterTest, TestEvaluationOperators) {
    ASSERT_EQ(RPNConverter("4*3", nullptr).evaluate(), 12.);
    ASSERT_EQ(RPNConverter("4+ 3", nullptr).evaluate(), 7.);
    ASSERT_EQ(RPNConverter("4/3", nullptr).evaluate(), 4./3);
    ASSERT_EQ(RPNConverter("4^3", nullptr).evaluate(), std::pow(4,3));
    ASSERT_EQ(RPNConverter("4+4^3", nullptr).evaluate(), 68);
    ASSERT_EQ(RPNConverter("4*4^3", nullptr).evaluate(), 256);
    ASSERT_EQ(RPNConverter("5%3", nullptr).evaluate(), 2);
    ASSERT_EQ(RPNConverter("(4*5)%3", nullptr).evaluate(), 2);
    ASSERT_EQ(RPNConverter("4*5%3", nullptr).evaluate(), 8);
    ASSERT_EQ(RPNConverter("4*(5%3)", nullptr).evaluate(), 8);
    ASSERT_EQ(RPNConverter("4*(3 + 5)", nullptr).evaluate(), 32.);
    ASSERT_EQ(RPNConverter("4*3 + 5", nullptr).evaluate(), 17.);
    ASSERT_EQ(RPNConverter("(2+2)(2 + 2)", nullptr).evaluate(), 16.);
    ASSERT_EQ(RPNConverter("max(0.2, 0.21)", nullptr).evaluate(), 0.21);
    ASSERT_EQ(RPNConverter("max(max(0.2,0.3*2), 0.21)", nullptr).evaluate(), 0.3*2.);
    ASSERT_EQ(RPNConverter("min(min(0.2,0.1), 0.21)", nullptr).evaluate(), 0.1);
    ASSERT_EQ(RPNConverter("min(0.2, 0.21)", nullptr).evaluate(), 0.2);
    ASSERT_EQ(RPNConverter("min((0.2 + 0.1), 0.21)", nullptr).evaluate(), 0.21);
    ASSERT_EQ(RPNConverter("min((0.2 * 0.1), 0.21)", nullptr).evaluate(), 0.2 * 0.1);
    ASSERT_EQ(RPNConverter("min(0.2 * 0.1, 0.21)", nullptr).evaluate(), 0.2 * 0.1);
}

TEST_F(RPNConverterTest, TestEvaluationFunctions) {
    ASSERT_NEAR(RPNConverter("cos(pi)", nullptr).evaluate(), -1., 0.000001);
    ASSERT_NEAR(RPNConverter("cos(0)", nullptr).evaluate(), 1., 0.000001);
    ASSERT_NEAR(RPNConverter("sin(Pi)", nullptr).evaluate(), 0., 0.000001);
    ASSERT_NEAR(RPNConverter("sin(PI/2)", nullptr).evaluate(), 1., 0.000001);
    ASSERT_NEAR(RPNConverter("tan(4)", nullptr).evaluate(), RPNConverter("sin(4) / cos(4)", nullptr).evaluate(),
                0.000001);
    ASSERT_NEAR(RPNConverter("tan((8/2))", nullptr).evaluate(), RPNConverter("sin((8/2)) / cos((2^2))", nullptr).evaluate(),
                0.000001);
    ASSERT_NEAR(RPNConverter("floor(1.2)", nullptr).evaluate(), 1., 0.000001);
    ASSERT_NEAR(RPNConverter("ceil(0.2)", nullptr).evaluate(), 1., 0.000001);
    ASSERT_NEAR(RPNConverter("log(0.2)", nullptr).evaluate(), std::log(0.2), 0.000001);
    ASSERT_NEAR(RPNConverter("log2(0.2)", nullptr).evaluate(), std::log2(0.2), 0.000001);
    ASSERT_NEAR(RPNConverter("4log2(0.2)", nullptr).evaluate(), 4*std::log2(0.2), 0.000001);
    ASSERT_NEAR(RPNConverter("4cos(0.2)4", nullptr).evaluate(), 16*std::cos(0.2), 0.000001);
    ASSERT_NEAR(RPNConverter("exp(0.2)", nullptr).evaluate(), std::exp(0.2), 0.000001);
    ASSERT_NEAR(RPNConverter("exp(log(0.2))", nullptr).evaluate(), 0.2, 0.000001);
    ASSERT_NEAR(RPNConverter("log(exp(0.2))", nullptr).evaluate(), 0.2, 0.000001);
    ASSERT_NEAR(RPNConverter("sqrt(4)", nullptr).evaluate(), 2., 0.000001);
}

TEST_F(RPNConverterTest, TestString) {
    ASSERT_EQ(RPNConverter("exp(log(0.2))", nullptr).toString(), "0.200000 log exp ");
    ASSERT_EQ(RPNConverter("exp(log(0.2))", nullptr).infixString(), "exp(log(0.2))");
    ASSERT_EQ(RPNConverter("exp( log ( 0.2) )", nullptr).infixString(), "exp(log(0.2))");
    ASSERT_EQ(RPNConverter("4cos(PI/2)", nullptr).infixString(), "4*cos(3.1415926535/2)");
}

