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
#ifndef SPIDER2_MEMORY_TEST_FREELIST_ALLOCATOR_H
#define SPIDER2_MEMORY_TEST_FREELIST_ALLOCATOR_H

#include <gtest/gtest.h>
#include <memory/static-allocators/FreeListStaticAllocator.h>

#define ALLOCATOR_NAME "my-allocator"

#define MAX_SIZE 512

TEST(FreeListStaticAllocatorTest, GetName) {
    auto *allocator = new FreeListStaticAllocator(ALLOCATOR_NAME, MAX_SIZE);
    EXPECT_STREQ(allocator->getName(), ALLOCATOR_NAME);
    delete allocator;
}

TEST(FreeListStaticAllocatorTest, ThrowSizeException) {
    auto *allocator = new FreeListStaticAllocator(ALLOCATOR_NAME, MAX_SIZE);
    std::uint64_t size = MAX_SIZE + 1;
    EXPECT_THROW(allocator->allocate(size), SpiderException);
    delete allocator;
}

TEST(FreeListStaticAllocatorTest, MemoryAllocFindFirst) {
    auto *allocator = new FreeListStaticAllocator(ALLOCATOR_NAME, MAX_SIZE);
    auto *array = (double *) allocator->allocate(2 * sizeof(double));
    ASSERT_NE(array, nullptr);
    array[0] = 1;
    array[1] = 2;
    ASSERT_EQ(array[0], 1);
    ASSERT_EQ(array[1], 2);
    ASSERT_EQ(nullptr, allocator->allocate(0));
    EXPECT_THROW(allocator->allocate(MAX_SIZE), SpiderException);
    EXPECT_NO_THROW(allocator->allocate(sizeof(std::int32_t)));
    EXPECT_NO_THROW(allocator->reset());
    EXPECT_NO_THROW(allocator->allocate(MAX_SIZE));

    /* == Undefined behavior == */
    EXPECT_NO_THROW(allocator->deallocate(array));
    EXPECT_NO_THROW(allocator->allocate(MAX_SIZE));
    delete allocator;
}

TEST(FreeListStaticAllocatorTest, MemoryAllocAlignmentFindFirst) {
    auto *allocator = new FreeListStaticAllocator(ALLOCATOR_NAME, MAX_SIZE);
    auto *charArray = (char *) allocator->allocate(17 * sizeof(char));
    ASSERT_NE(charArray, nullptr);
    auto *dblArray = (double *) allocator->allocate(2 * sizeof(double));
    ASSERT_NE(dblArray, nullptr);
    std::int32_t headerSize = 2 * sizeof(std::uint64_t);
    ASSERT_EQ(charArray + 17 + headerSize, (char *) dblArray);
    delete allocator;
}

TEST(FreeListStaticAllocatorTest, FreeNull) {
    auto *allocator = new FreeListStaticAllocator(ALLOCATOR_NAME, MAX_SIZE);
    EXPECT_NO_THROW(allocator->deallocate(nullptr));
    delete allocator;
}

TEST(FreeListStaticAllocatorTest, FreeOutOfScope) {
    auto *allocator = new FreeListStaticAllocator(ALLOCATOR_NAME, MAX_SIZE);
    char *charArray = new char[8];
    EXPECT_THROW(allocator->deallocate(charArray), SpiderException);
    delete[] charArray;
    auto *dblArray = (double *) allocator->allocate(2 * sizeof(double));
    ASSERT_NE(dblArray, nullptr);
    EXPECT_THROW(allocator->deallocate(dblArray + MAX_SIZE), SpiderException);
    delete allocator;
}

TEST(FreeListStaticAllocatorTest, MemoryAllocFindBest) {
    auto *allocator = new FreeListStaticAllocator(ALLOCATOR_NAME, MAX_SIZE, FreeListPolicy::FIND_BEST);
    auto *array = (double *) allocator->allocate(2 * sizeof(double));
    ASSERT_NE(array, nullptr);
    array[0] = 1;
    array[1] = 2;
    ASSERT_EQ(array[0], 1);
    ASSERT_EQ(array[1], 2);
    ASSERT_EQ(nullptr, allocator->allocate(0));
    EXPECT_THROW(allocator->allocate(MAX_SIZE), SpiderException);
    EXPECT_NO_THROW(allocator->allocate(sizeof(std::int32_t)));
    EXPECT_NO_THROW(allocator->reset());
    EXPECT_NO_THROW(allocator->allocate(MAX_SIZE));
    EXPECT_NO_THROW(allocator->deallocate(array));
    EXPECT_NO_THROW(allocator->allocate(MAX_SIZE));

    delete allocator;
}

TEST(FreeListStaticAllocatorTest, MemoryAllocAlignmentFindBest) {
    auto *allocator = new FreeListStaticAllocator(ALLOCATOR_NAME, MAX_SIZE, FreeListPolicy::FIND_BEST);
    auto *charArray = (char *) allocator->allocate(17 * sizeof(char));
    ASSERT_NE(charArray, nullptr);
    auto *dblArray = (double *) allocator->allocate(2 * sizeof(double));
    ASSERT_NE(dblArray, nullptr);
    std::int32_t headerSize = 2 * sizeof(std::uint64_t);
    ASSERT_EQ(charArray + 17 + headerSize, (char *) dblArray);
    delete allocator;
}

TEST(FreeListStaticAllocatorTest, Free) {
    auto *allocator = new FreeListStaticAllocator(ALLOCATOR_NAME, MAX_SIZE);
    EXPECT_NO_THROW(allocator->deallocate(nullptr));
    auto *charArray = (char *) allocator->allocate(16 * sizeof(char));
    ASSERT_NE(charArray, nullptr);
    auto *dblArray = (double *) allocator->allocate(2 * sizeof(double));
    ASSERT_NE(dblArray, nullptr);
    auto *ptr = allocator->allocate(6 * sizeof(double));
    ASSERT_NE(ptr, nullptr);
    EXPECT_NO_THROW(allocator->deallocate(dblArray));
    EXPECT_NO_THROW(allocator->deallocate(ptr));
    EXPECT_NO_THROW(allocator->deallocate(charArray));
    EXPECT_NO_THROW(allocator->allocate(MAX_SIZE));
    delete allocator;
}

TEST(FreeListStaticAllocatorTest, MinAlignmentSize) {
    EXPECT_THROW(FreeListStaticAllocator(ALLOCATOR_NAME, MAX_SIZE, FreeListPolicy::FIND_FIRST, 0),
                 SpiderException);
}

#endif //SPIDER2_MEMORY_TEST_FREELIST_ALLOCATOR_H
