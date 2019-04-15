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
#ifndef SPIDER2_MEMORY_TEST_GENERIC_ALLOCATOR_H
#define SPIDER2_MEMORY_TEST_GENERIC_ALLOCATOR_H

#include <gtest/gtest.h>
#include "common/memory/GenericAllocator.h"
#include "common/SpiderException.h"

#define ALLOCATOR_NAME "my-allocator"

TEST(GenericAllocatorTest, GetName) {
    auto *allocator = new GenericAllocator(ALLOCATOR_NAME);
    EXPECT_STREQ(allocator->getName(), ALLOCATOR_NAME);
    delete allocator;
}

TEST(GenericAllocatorTest, MinAlignmentSize) {
    EXPECT_THROW(GenericAllocator(ALLOCATOR_NAME, -1), SpiderException);
}

TEST(GenericAllocatorTest, MemoryAlloc) {
    auto *allocator = new GenericAllocator(ALLOCATOR_NAME);
    auto *array = (double *) allocator->alloc(2 * sizeof(double));
    ASSERT_NE(array, nullptr);
    array[0] = 1;
    array[1] = 2;
    ASSERT_EQ(array[0], 1);
    ASSERT_EQ(array[1], 2);
    ASSERT_EQ(nullptr, allocator->alloc(0));
    EXPECT_NO_THROW(allocator->dealloc(array));
    EXPECT_NO_THROW(allocator->reset());
    delete allocator;
}

TEST(GenericAllocatorTest, DestructorWithUnFreedMemory) {
    auto *allocator = new GenericAllocator(ALLOCATOR_NAME);
    auto *array = (double *) allocator->alloc(2 * sizeof(double));
    ASSERT_NE(array, nullptr);
    delete allocator;
    allocator = new GenericAllocator(ALLOCATOR_NAME);
    array = (double *) allocator->alloc(1024);
    ASSERT_NE(array, nullptr);
    delete allocator;
    allocator = new GenericAllocator(ALLOCATOR_NAME);
    array = (double *) allocator->alloc(1024 * 1024);
    ASSERT_NE(array, nullptr);
    delete allocator;
    allocator = new GenericAllocator(ALLOCATOR_NAME);
    array = (double *) allocator->alloc(1024 * 1024 * 1024);
    ASSERT_NE(array, nullptr);
    delete allocator;
}

TEST(GenericAllocatorTest, FreeNull) {
    auto *allocator = new GenericAllocator(ALLOCATOR_NAME);
    EXPECT_NO_THROW(allocator->dealloc(nullptr));
    delete allocator;
}

#endif //SPIDER2_MEMORY_TEST_GENERIC_ALLOCATOR_H
