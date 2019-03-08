/**
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
#include <gtest/gtest.h>
#include <common/SpiderException.h>
#include "LIFOAllocatorTest.h"


LIFOAllocatorTest::LIFOAllocatorTest() : allocator(ALLOCATOR_NAME, MAX_SIZE) {
}

LIFOAllocatorTest::~LIFOAllocatorTest() {

}

void LIFOAllocatorTest::SetUp() {

}

void LIFOAllocatorTest::TearDown() {

}

TEST_F(LIFOAllocatorTest, GetName) {
    EXPECT_STREQ(allocator.getName(), ALLOCATOR_NAME);
}

TEST_F(LIFOAllocatorTest, ThrowSizeException) {
    std::uint64_t size = MAX_SIZE + 1;
    EXPECT_THROW(allocator.alloc(size), SpiderException);
}

TEST_F(LIFOAllocatorTest, MemoryAlloc) {
    auto *array = (double *) allocator.alloc(2 * sizeof(double));
    ASSERT_NE(array, nullptr);
    array[0] = 1;
    array[1] = 2;
    ASSERT_EQ(array[0], 1);
    ASSERT_EQ(array[1], 2);
    ASSERT_EQ(nullptr, allocator.alloc(0));
    EXPECT_THROW(allocator.alloc(MAX_SIZE), SpiderException);
    EXPECT_NO_THROW(allocator.reset());
    EXPECT_NO_THROW(allocator.alloc(MAX_SIZE));
    EXPECT_NO_THROW(allocator.free(array));
}

TEST_F(LIFOAllocatorTest, MemoryAllocAlignment) {
    auto *charArray = (char *) allocator.alloc(9 * sizeof(char));
    ASSERT_NE(charArray, nullptr);
    auto *dblArray = (double *) allocator.alloc(2 * sizeof(double));
    ASSERT_NE(dblArray, nullptr);
    ASSERT_EQ(charArray + 2 * sizeof(std::uint64_t), (char*) dblArray);
}

TEST_F(LIFOAllocatorTest, FreeNull) {
    EXPECT_NO_THROW(allocator.free(nullptr));
}

TEST_F(LIFOAllocatorTest, FreeUnordered) {
    auto *charArray = (char *) allocator.alloc(9 * sizeof(char));
    ASSERT_NE(charArray, nullptr);
    auto *dblArray = (double *) allocator.alloc(2 * sizeof(double));
    ASSERT_NE(dblArray, nullptr);
    EXPECT_NO_THROW(allocator.free(charArray));
    EXPECT_THROW(allocator.free(dblArray), SpiderException);
}

TEST_F(LIFOAllocatorTest, FreeOrdered) {
    EXPECT_NO_THROW(allocator.free(nullptr));
    auto *charArray = (char *) allocator.alloc(9 * sizeof(char));
    ASSERT_NE(charArray, nullptr);
    auto *dblArray = (double *) allocator.alloc(2 * sizeof(double));
    ASSERT_NE(dblArray, nullptr);
    EXPECT_NO_THROW(allocator.free(dblArray));
    EXPECT_NO_THROW(allocator.free(charArray));
}

TEST_F(LIFOAllocatorTest, FreeOutOfScope) {
    char *charArray = new char[8];
    EXPECT_THROW(allocator.free(charArray), SpiderException);
    delete[] charArray;
    auto *dblArray = (double *) allocator.alloc(2 * sizeof(double));
    ASSERT_NE(dblArray, nullptr);
    EXPECT_THROW(allocator.free(dblArray + MAX_SIZE), SpiderException);
}