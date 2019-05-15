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

#include <common/memory/Allocator.h>
#include <common/containers/Array.h>
#include "ArrayTest.h"

/* === Methods implementation === */


ArrayTest::ArrayTest() {
    AllocatorConfig cfg;
    cfg.allocatorType = AllocatorType::FREELIST;
    cfg.size = 512;
    Allocator::init(StackID::GENERAL_STACK, cfg);
}

ArrayTest::~ArrayTest() {
    AllocatorConfig cfg;
    cfg.allocatorType = AllocatorType::FREELIST;
    cfg.size = 512;
    Allocator::init(StackID::GENERAL_STACK, cfg);
}

TEST_F(ArrayTest, TestCreation) {
    auto testArray = Array<double>(StackID::GENERAL_STACK, 10);
}


TEST_F(ArrayTest, TestAssignation) {
    auto testArray = Array<double>(StackID::GENERAL_STACK, 10);
    EXPECT_NO_THROW(testArray[0] =  3.1415926535);
    EXPECT_EQ(testArray[0], 3.1415926535);
    EXPECT_THROW(testArray[10] =  3.141592, SpiderException);
    EXPECT_THROW(testArray[-1] =  3.141592, SpiderException);
}

TEST_F(ArrayTest, TestIteration) {
    auto testArray = Array<double>(StackID::GENERAL_STACK, 10);
    for (auto &val : testArray) {
        val = 3.1415926535;
    }
}
