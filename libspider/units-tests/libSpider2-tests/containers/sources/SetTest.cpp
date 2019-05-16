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

#include "SetTest.h"
#include <common/memory/Allocator.h>
#include <common/containers/Set.h>

/* === Methods implementation === */


SetTest::SetTest() {
}

SetTest::~SetTest() {
}

void SetTest::SetUp() {
    AllocatorConfig cfg;
    cfg.allocatorType = AllocatorType::FREELIST;
    cfg.size = 512;
    Allocator::init(StackID::GENERAL, cfg);
}

void SetTest::TearDown() {
    Allocator::finalize();
}

class MySetElement : public SetElement {
public:
    double value;
};

TEST_F(SetTest, TestCreation) {
    EXPECT_THROW(Set<MySetElement>(StackID::GENERAL, 10), SpiderException);
    EXPECT_NO_THROW(Set<MySetElement *>(StackID::GENERAL, 10));
}


TEST_F(SetTest, TestAssignation) {
    auto testArray = Set<MySetElement *>(StackID::GENERAL, 10);
    MySetElement elt;
    EXPECT_NO_THROW(testArray.add(&elt));
    EXPECT_NO_THROW(testArray[0] = &elt);
    EXPECT_EQ(testArray[0], &elt);
    EXPECT_THROW(testArray[1], SpiderException);
    EXPECT_THROW(testArray[-1], SpiderException);
}

TEST_F(SetTest, TestRemove) {
    auto testArray = Set<MySetElement *>(StackID::GENERAL, 10);
    MySetElement *elt = new MySetElement;
    MySetElement *elt2 = new MySetElement;
    EXPECT_NO_THROW(testArray.add(elt));
    EXPECT_NO_THROW(testArray.add(elt2));
    EXPECT_NO_THROW(testArray.remove(elt));
    EXPECT_THROW(testArray.remove(elt), SpiderException);
    EXPECT_NO_THROW(testArray.remove(elt2));
    EXPECT_NO_THROW(testArray.remove(elt2));
    delete elt;
    delete elt2;
}


TEST_F(SetTest, TestIteration) {
    auto testSet = Set<MySetElement *>(StackID::GENERAL, 10);
    MySetElement count;
    count.value = 0.;
    EXPECT_EQ(testSet.size(), 10);
    EXPECT_EQ(testSet.occupied(), 0);
    int it = 0;
    for (const auto &val : testSet) {
        EXPECT_EQ(val, &count);
        it++;
    }
    EXPECT_EQ(it, 0);
    testSet.add(&count);
    EXPECT_EQ(testSet.occupied(), 1);
    for (const auto &val : testSet) {
        EXPECT_EQ(val, &count);
        it++;
    }
    EXPECT_EQ(it, 1);
}