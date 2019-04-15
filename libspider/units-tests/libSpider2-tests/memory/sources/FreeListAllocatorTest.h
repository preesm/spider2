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
#ifndef SPIDER2_MEMORY_TEST_FREELISTDYNAMIC_ALLOCATOR_H
#define SPIDER2_MEMORY_TEST_FREELISTDYNAMIC_ALLOCATOR_H

#include <gtest/gtest.h>
#include "common/memory/FreeListAllocator.h"

#define ALLOCATOR_NAME "my-allocator"

#define MAX_SIZE 512

TEST(FreeListAllocatorTest, GetName) {
    auto *allocator = new FreeListAllocator(ALLOCATOR_NAME, MAX_SIZE);
    EXPECT_STREQ(allocator->getName(), ALLOCATOR_NAME);
    delete allocator;
}

TEST(FreeListAllocatorTest, ThrowSizeException) {
    auto *allocator = new FreeListAllocator(ALLOCATOR_NAME, MAX_SIZE);
    std::uint64_t size = MAX_SIZE + 1;
    EXPECT_NO_THROW(allocator->alloc(size));
    delete allocator;
}

TEST(FreeListAllocatorTest, MemoryAllocFindFirst) {
    auto *allocator = new FreeListAllocator(ALLOCATOR_NAME, MAX_SIZE);
    auto *array = (double *) allocator->alloc(2 * sizeof(double));
    ASSERT_NE(array, nullptr);
    array[0] = 1;
    array[1] = 2;
    ASSERT_EQ(array[0], 1);
    ASSERT_EQ(array[1], 2);
    ASSERT_EQ(nullptr, allocator->alloc(0));
    EXPECT_NO_THROW(allocator->alloc(MAX_SIZE));
    EXPECT_THROW(allocator->alloc(sizeof(std::int32_t)), SpiderException);
    EXPECT_NO_THROW(allocator->dealloc(array));
    EXPECT_NO_THROW(allocator->reset());
    EXPECT_NO_THROW(allocator->alloc(MAX_SIZE));
    EXPECT_THROW(allocator->dealloc(array), SpiderException);
    EXPECT_NO_THROW(allocator->alloc(MAX_SIZE));
    delete allocator;
}

TEST(FreeListAllocatorTest, MemoryAllocAlignmentFindFirst) {
    auto *allocator = new FreeListAllocator(ALLOCATOR_NAME, MAX_SIZE);
    auto *charArray = (char *) allocator->alloc(17 * sizeof(char));
    ASSERT_NE(charArray, nullptr);
    auto *dblArray = (double *) allocator->alloc(2 * sizeof(double));
    ASSERT_NE(dblArray, nullptr);
    std::int32_t paddingSize = 1 * sizeof(std::uint64_t);
    std::int32_t headerSize = 2 * sizeof(std::uint64_t);
    ASSERT_EQ(charArray + 16 + paddingSize + headerSize, (char *) dblArray);
    delete allocator;
}

TEST(FreeListAllocatorTest, MemoryAllocAlignmentChunks) {
    auto *allocator = new FreeListAllocator(ALLOCATOR_NAME, MAX_SIZE);
    auto *charArray = (char *) allocator->alloc(MAX_SIZE);
    ASSERT_NE(charArray, nullptr);
    auto *dblArray = (double *) allocator->alloc(MAX_SIZE);
    ASSERT_NE(dblArray, nullptr);
    std::int32_t paddingSize = 0;
    std::int32_t headerSize = 2 * sizeof(std::uint64_t);
    ASSERT_EQ(charArray + MAX_SIZE + paddingSize + headerSize, (char *) dblArray);
    auto *ptr = (char *) allocator->alloc(17 * sizeof(char));
    ASSERT_NE(ptr + 2 * MAX_SIZE + 2 * headerSize, (char *) dblArray);
    delete allocator;
}

TEST(FreeListAllocatorTest, FreeNull) {
    auto *allocator = new FreeListAllocator(ALLOCATOR_NAME, MAX_SIZE);
    EXPECT_NO_THROW(allocator->dealloc(nullptr));
    delete allocator;
}

TEST(FreeListAllocatorTest, FreeOutOfScope) {
    auto *allocator = new FreeListAllocator(ALLOCATOR_NAME, MAX_SIZE);
    char *charArray = new char[8];
    EXPECT_THROW(allocator->dealloc(charArray), SpiderException);
    delete[] charArray;
    auto *dblArray = (double *) allocator->alloc(2 * sizeof(double));
    ASSERT_NE(dblArray, nullptr);
    EXPECT_THROW(allocator->dealloc(dblArray + MAX_SIZE), SpiderException);
    delete allocator;
}

TEST(FreeListAllocatorTest, MemoryAllocFindBest) {
    auto *allocator = new FreeListAllocator(ALLOCATOR_NAME, MAX_SIZE, FreeListAllocator::FIND_BEST);
    auto *array = (double *) allocator->alloc(2 * sizeof(double));
    ASSERT_NE(array, nullptr);
    array[0] = 1;
    array[1] = 2;
    ASSERT_EQ(array[0], 1);
    ASSERT_EQ(array[1], 2);
    ASSERT_EQ(nullptr, allocator->alloc(0));
    EXPECT_NO_THROW(allocator->alloc(MAX_SIZE));
    EXPECT_THROW(allocator->alloc(sizeof(std::int32_t)), SpiderException);
    EXPECT_NO_THROW(allocator->reset());
    EXPECT_NO_THROW(allocator->alloc(MAX_SIZE));
    EXPECT_THROW(allocator->dealloc(array), SpiderException);
    EXPECT_NO_THROW(allocator->alloc(MAX_SIZE));

    delete allocator;
}

TEST(FreeListAllocatorTest, MemoryAllocAlignmentFindBest) {
    auto *allocator = new FreeListAllocator(ALLOCATOR_NAME, MAX_SIZE, FreeListAllocator::FIND_BEST);
    auto *charArray = (char *) allocator->alloc(17 * sizeof(char));
    ASSERT_NE(charArray, nullptr);
    auto *dblArray = (double *) allocator->alloc(2 * sizeof(double));
    ASSERT_NE(dblArray, nullptr);
    std::int32_t paddingSize = 1 * sizeof(std::uint64_t);
    std::int32_t headerSize = 2 * sizeof(std::uint64_t);
    ASSERT_EQ(charArray + 16 + paddingSize + headerSize, (char *) dblArray);
    delete allocator;
}

TEST(FreeListAllocatorTest, Free) {
    auto *allocator = new FreeListAllocator(ALLOCATOR_NAME, MAX_SIZE);
    EXPECT_NO_THROW(allocator->dealloc(nullptr));
    auto *charArray = (char *) allocator->alloc(16 * sizeof(char));
    ASSERT_NE(charArray, nullptr);
    auto *dblArray = (double *) allocator->alloc(2 * sizeof(double));
    ASSERT_NE(dblArray, nullptr);
    auto *ptr = allocator->alloc(6 * sizeof(double));
    ASSERT_NE(ptr, nullptr);
    EXPECT_NO_THROW(allocator->dealloc(dblArray));
    EXPECT_NO_THROW(allocator->dealloc(ptr));
    EXPECT_NO_THROW(allocator->dealloc(charArray));
    ptr = allocator->alloc(MAX_SIZE);
    EXPECT_NO_THROW(allocator->dealloc(ptr));
    delete allocator;
}

TEST(FreeListAllocatorTest, MinAlignmentSize) {
    EXPECT_THROW(FreeListAllocator(ALLOCATOR_NAME, MAX_SIZE, FreeListAllocator::FIND_FIRST, 0),
                 SpiderException);
}

#endif //SPIDER2_MEMORY_TEST_FREELISTDYNAMIC_ALLOCATOR_H
