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
#ifndef CONTAINERS_LINKEDLISTTEST_H
#define CONTAINERS_LINKEDLISTTEST_H

/* === Includes === */

#include <gtest/gtest.h>
#include <containers/LinkedList.h>

/* === Methods prototype === */

TEST(LinkedListTest, TestAdd) {
    AllocatorConfig cfg;
    cfg.allocatorType = AllocatorType::FREELIST;
    cfg.size = 512;
    Spider::initAllocator(StackID::GENERAL, cfg);
    auto testList = Spider::LinkedList<double>(StackID::GENERAL);
    EXPECT_NO_THROW(testList.addHead(10.2));
    EXPECT_EQ(testList.tail(), testList.head());
    EXPECT_EQ(testList.tail(), testList.current());
    EXPECT_NO_THROW(testList.remove(testList.head()));
    EXPECT_NO_THROW(testList.addTail(3.14159265358));
    EXPECT_NO_THROW(testList.remove(testList.head()));
    EXPECT_NO_THROW(testList.addCurrent(2.71));
    EXPECT_NO_THROW(testList.~LinkedList());
    Spider::finalizeAllocators();
}

TEST(LinkedListTest, TestEqValue) {
    AllocatorConfig cfg;
    cfg.allocatorType = AllocatorType::FREELIST;
    cfg.size = 512;
    Spider::initAllocator(StackID::GENERAL, cfg);
    auto testList = Spider::LinkedList<double>(StackID::GENERAL);
    EXPECT_NO_THROW(testList.addHead(10.2));
    EXPECT_NO_THROW(testList.addHead(3.14159265358));
    EXPECT_EQ(testList.tail()->value, 10.2);
    EXPECT_NO_THROW(testList.addCurrent(2.71));
    EXPECT_EQ(testList.tail()->value, 2.71);
    EXPECT_EQ(testList.current()->value, 2.71);
    EXPECT_EQ(testList.head()->value, 3.14159265358);
    testList.previous();
    EXPECT_EQ(testList.current()->value, 10.2);
    testList.previous();
    EXPECT_EQ(testList.current()->value, 3.14159265358);
    EXPECT_NO_THROW(testList.~LinkedList());
    Spider::finalizeAllocators();
}

TEST(LinkedListTest, TestRemove) {
    AllocatorConfig cfg;
    cfg.allocatorType = AllocatorType::FREELIST;
    cfg.size = 512;
    Spider::initAllocator(StackID::GENERAL, cfg);
    auto testList = Spider::LinkedList<double>(StackID::GENERAL);
    EXPECT_NO_THROW(testList.addHead(10.2));
    EXPECT_NO_THROW(testList.addHead(3.14159265358));
    EXPECT_NO_THROW(testList.addCurrent(2.71));
    EXPECT_NO_THROW(testList.remove(testList.current()));
    EXPECT_EQ(testList.tail()->value, 10.2);
    EXPECT_EQ(testList.current()->value, 10.2);
    EXPECT_EQ(testList.size(), 2);
    EXPECT_NO_THROW(testList.remove(testList.current()));
    EXPECT_EQ(testList.size(), 1);
    EXPECT_NO_THROW(testList.remove(testList.current()));
    EXPECT_NO_THROW(testList.remove(testList.current()));
    EXPECT_EQ(testList.size(), 0);
    EXPECT_NO_THROW(testList.~LinkedList());
    Spider::finalizeAllocators();
}

TEST(LinkedListTest, TestIterator) {
    AllocatorConfig cfg;
    cfg.allocatorType = AllocatorType::FREELIST;
    cfg.size = 512;
    Spider::initAllocator(StackID::GENERAL, cfg);
    auto testList = Spider::LinkedList<double>(StackID::GENERAL);
    EXPECT_NO_THROW(testList.addHead(10.2));
    EXPECT_NO_THROW(testList.addHead(3.14159265358));
    EXPECT_NO_THROW(testList.addCurrent(2.71));
    double count = 1.;
    for (auto &val : testList) {
        val = 3.1415926535 + count;
        count += 1;
    }
    count = 1;
    for (const auto &val : testList) {
        EXPECT_EQ(val, 3.1415926535 + count);
        count += 1;
    }
}

TEST(LinkedListTest, TestRandomAccessOperator) {
    AllocatorConfig cfg;
    cfg.allocatorType = AllocatorType::FREELIST;
    cfg.size = 512;
    Spider::initAllocator(StackID::GENERAL, cfg);
    auto testList = Spider::LinkedList<double>(StackID::GENERAL);
    EXPECT_NO_THROW(testList.addHead(10.2));
    EXPECT_NO_THROW(testList.addHead(3.14159265358));
    EXPECT_NO_THROW(testList.addCurrent(2.71));
    EXPECT_NO_THROW(testList[2]);
    EXPECT_THROW(testList[3], Spider::Exception);
    EXPECT_EQ(testList[2], 2.71);
    EXPECT_EQ(testList[1], 10.2);
    EXPECT_EQ(testList.current()->value, 2.71);
    EXPECT_NO_THROW(testList.~LinkedList());
    Spider::finalizeAllocators();
}

#endif //CONTAINERS_LINKEDLISTTEST_H
