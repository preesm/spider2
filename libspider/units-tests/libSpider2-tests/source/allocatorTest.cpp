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
#include <common/Exception.h>
#include <memory/allocator.h>
#include <memory/dynamic-allocators/FreeListAllocator.h>

class allocatorTest : public ::testing::Test {
protected:
};

TEST_F(allocatorTest, abstractAllocNameTest) {
    auto allocator = LinearStaticAllocator("alloc", 512);
    ASSERT_STREQ(allocator.name(), "alloc") << "LinearStaticAllocator: name() failed.";
}
TEST_F(allocatorTest, abstractAllocUsageTest) {
    spider::api::enableLogger(spider::log::GENERAL);
    {
        auto allocator = GenericAllocator("alloc", 8);
        auto *buffer = allocator.allocate(1024*1024*1024);
        ASSERT_NE(buffer, nullptr) << "Allocator: failed to allocated 1GB";
        ASSERT_NO_THROW(allocator.deallocate(buffer)) << "Allocator: deallocation failed";
        allocator.printStats();
    }
    {
        auto allocator = GenericAllocator("alloc", 8);
        auto *buffer = allocator.allocate(2*1024*1024);
        ASSERT_NE(buffer, nullptr) << "Allocator: failed to allocated 1MB";
        ASSERT_NO_THROW(allocator.deallocate(buffer)) << "Allocator: deallocation failed";
        allocator.printStats();
    }
    {
        auto allocator = GenericAllocator("alloc", 8);
        auto *buffer = allocator.allocate(1024);
        ASSERT_NE(buffer, nullptr) << "Allocator: failed to allocated 1KB";
        ASSERT_NO_THROW(allocator.deallocate(buffer)) << "Allocator: deallocation failed";
        allocator.printStats();
    }
    spider::api::disableLogger(spider::log::GENERAL);
}

TEST_F(allocatorTest, linearAllocCtorTest) {
    ASSERT_THROW(LinearStaticAllocator("", 0), spider::Exception) << "LinearStaticAllocator should throw with 0 size.";
    ASSERT_THROW(LinearStaticAllocator("", 1000, 4), spider::Exception) << "LinearStaticAllocator should throw with improper alignment size.";
    ASSERT_NO_THROW(LinearStaticAllocator("", 1000)) << "LinearStaticAllocator should not throw with default ctor.";
    ASSERT_THROW(LinearStaticAllocator("", 10, nullptr), spider::Exception) << "LinearStaticAllocator should throw with nullptr extern buffer.";
    char tmp[512];
    ASSERT_THROW(LinearStaticAllocator("", 0, &tmp), spider::Exception) << "LinearStaticAllocator should throw with 0 size and extern buffer.";
    ASSERT_THROW(LinearStaticAllocator("", 10, &tmp, 4), spider::Exception) << "LinearStaticAllocator should throw invalid alignment size.";
    ASSERT_NO_THROW(LinearStaticAllocator("", 10, &tmp)) << "LinearStaticAllocator should not throw with valid external buffer.";
}

TEST_F(allocatorTest, linearAllocTest) {
    auto allocator = LinearStaticAllocator("alloc", 512);
    ASSERT_NE(allocator.allocate(64), nullptr) << "LinearStaticAllocator: failed to allocate buffer.";
    ASSERT_EQ(allocator.allocate(0), nullptr) << "LinearStaticAllocator: 0 size buffer should return nullptr.";
    ASSERT_NO_THROW(allocator.allocate(64)) << "LinearStaticAllocator: failed to allocate buffer.";
    allocator.reset();
    ASSERT_NO_THROW(allocator.allocate(512)) << "LinearStaticAllocator: failed to allocate buffer.";
    allocator.reset();
    ASSERT_NO_THROW(allocator.allocate(512)) << "LinearStaticAllocator: reallocating after reset should not throw.";
    allocator.reset();
    ASSERT_THROW(allocator.allocate(513), spider::Exception) << "LinearStaticAllocator: should throw if size > available.";
    char tmp[50];
    ASSERT_THROW(allocator.deallocate(tmp), spider::Exception) << "LinearStaticAllocator: deallocating not allocated buffer should throw.";
    ASSERT_NO_THROW(allocator.deallocate(nullptr)) << "LinearStaticAllocator: deallocate on nullptr should not throw.";
    ASSERT_NO_THROW(allocator.deallocate(allocator.allocate(64))) << "LinearStaticAllocator: deallocate should not throw.";
}

TEST_F(allocatorTest, linearExternAllocTest) {
    char buffer[512];
    auto allocator = LinearStaticAllocator("alloc", 512, &buffer);
    ASSERT_NE(allocator.allocate(64), nullptr) << "LinearStaticAllocator: failed to allocate buffer.";
    ASSERT_EQ(allocator.allocate(0), nullptr) << "LinearStaticAllocator: 0 size buffer should return nullptr.";
    ASSERT_NO_THROW(allocator.allocate(64)) << "LinearStaticAllocator: failed to allocate buffer.";
    allocator.reset();
    ASSERT_NO_THROW(allocator.allocate(512)) << "LinearStaticAllocator: failed to allocate buffer.";
    allocator.reset();
    ASSERT_NO_THROW(allocator.allocate(512)) << "LinearStaticAllocator: reallocating after reset should not throw.";
    allocator.reset();
    ASSERT_THROW(allocator.allocate(513), spider::Exception) << "LinearStaticAllocator: should throw if size > available.";
    char tmp[50];
    ASSERT_THROW(allocator.deallocate(tmp), spider::Exception) << "LinearStaticAllocator: deallocating not allocated buffer should throw.";
    ASSERT_NO_THROW(allocator.deallocate(nullptr)) << "LinearStaticAllocator: deallocate on nullptr should not throw.";
    ASSERT_NO_THROW(allocator.deallocate(allocator.allocate(64))) << "LinearStaticAllocator: deallocate should not throw.";
}

template<class ALLOC>
void staticAllocAlignTest(ALLOC &allocator) {
    {
        auto *buffer = reinterpret_cast<char *>(allocator.allocate(9 * sizeof(char)));
        ASSERT_NE(buffer, nullptr) << "Allocator: allocation failed.";
        auto *buffer2 = reinterpret_cast<double *>(allocator.allocate(2 * sizeof(double)));
        ASSERT_NE(buffer2, nullptr) << "Allocator: allocation failed.";
        ASSERT_EQ(reinterpret_cast<uintptr_t>(buffer) + 16, reinterpret_cast<uintptr_t>(buffer2)) << "Allocator: alignment with padding failed.";
    }
    {
        auto *buffer = reinterpret_cast<char *>(allocator.allocate(8 * sizeof(char)));
        ASSERT_NE(buffer, nullptr) << "Allocator: allocation failed.";
        auto *buffer2 = reinterpret_cast<double *>(allocator.allocate(2 * sizeof(double)));
        ASSERT_NE(buffer2, nullptr) << "Allocator: allocation failed.";
        ASSERT_EQ(reinterpret_cast<uintptr_t>(buffer) + 8, reinterpret_cast<uintptr_t>(buffer2)) << "Allocator: alignment without padding failed.";
    }
}

TEST_F(allocatorTest, linearAlignTest) {
    spider::api::enableLogger(spider::log::GENERAL);
    auto allocator = LinearStaticAllocator("alloc", 512, 8);
    staticAllocAlignTest(allocator);
    allocator.reset();
    allocator.printStats();
    spider::api::disableLogger(spider::log::GENERAL);
}

TEST_F(allocatorTest, LIFOAlignTest) {
    auto allocator = LIFOStaticAllocator("alloc", 512);
    staticAllocAlignTest(allocator);
}

void freeListAlignTest(FreeListAllocator &allocator) {
    {
        /* == Should allocate 32 bytes (17 of char + sizeof(size_t) of header + 7 of padding) == */
        auto *buffer = reinterpret_cast<char *>(allocator.allocate(17 * sizeof(char)));
        ASSERT_NE(buffer, nullptr) << "Allocator: allocation failed.";

        /* == Should allocate 24 bytes (16 of dbl + sizeof(size_t) of header) == */
        auto *buffer2 = reinterpret_cast<double *>(allocator.allocate(2 * sizeof(double)));
        ASSERT_NE(buffer2, nullptr) << "Allocator: allocation failed.";
        ASSERT_EQ(reinterpret_cast<uintptr_t>(buffer) + (32 - sizeof(size_t)) + sizeof(size_t), reinterpret_cast<uintptr_t>(buffer2)) << "Allocator: align with padding failed.";
        ASSERT_NO_THROW(allocator.deallocate(buffer)) << "Allocator: deallocation should not fail.";
        ASSERT_NO_THROW(allocator.deallocate(buffer2)) << "Allocator: deallocation should not fail.";
    }
    {
        /* == Should allocate 24 bytes (16 of char + sizeof(size_t) of header) == */
        auto *buffer = reinterpret_cast<char *>(allocator.allocate(16 * sizeof(char)));
        ASSERT_NE(buffer, nullptr) << "Allocator: allocation failed.";

        /* == Should allocate 24 bytes (16 of dbl + sizeof(size_t) of header) == */
        auto *buffer2 = reinterpret_cast<double *>(allocator.allocate(2 * sizeof(double)));
        ASSERT_NE(buffer2, nullptr) << "Allocator: allocation failed.";
        ASSERT_EQ(reinterpret_cast<uintptr_t>(buffer) + (24 - sizeof(size_t)) + sizeof(size_t), reinterpret_cast<uintptr_t>(buffer2)) << "Allocator: align with padding failed.";
        ASSERT_NO_THROW(allocator.deallocate(buffer)) << "Allocator: deallocation should not fail.";
        ASSERT_NO_THROW(allocator.deallocate(buffer2)) << "Allocator: deallocation should not fail.";
    }
    {
        /* == Should allocate 520 bytes (512 of char + sizeof(size_t) of header) == */
        auto *buffer = reinterpret_cast<char *>(allocator.allocate(512));
        ASSERT_NE(buffer, nullptr) << "Allocator: allocation failed.";

        /* == Should allocate 520 bytes (512 of dbl + sizeof(size_t) of header) == */
        auto *buffer2 = reinterpret_cast<double *>(allocator.allocate(512));
        ASSERT_NE(buffer2, nullptr) << "Allocator: allocation failed.";
        ASSERT_EQ(reinterpret_cast<uintptr_t>(buffer) + 512 + sizeof(size_t), reinterpret_cast<uintptr_t>(buffer2)) << "Allocator: align with padding failed.";
        ASSERT_NO_THROW(allocator.deallocate(buffer)) << "Allocator: deallocation should not fail.";
        ASSERT_NO_THROW(allocator.deallocate(buffer2)) << "Allocator: deallocation should not fail.";
    }
}

TEST_F(allocatorTest, freeListAlignTest) {
    auto allocator = FreeListAllocator("alloc", 512, FreeListPolicy::FIND_FIRST);
    freeListAlignTest(allocator);
    auto allocator2 = FreeListAllocator("alloc", 512, FreeListPolicy::FIND_BEST);
    freeListAlignTest(allocator2);
}

void freeListAllocTest(FreeListAllocator &allocator) {
    {
        auto *buffer = reinterpret_cast<double*>(allocator.allocate(2 * sizeof(double)));
        ASSERT_NE(buffer, nullptr) << "Allocator: allocation failed.";
        ASSERT_EQ(nullptr, allocator.allocate(0)) << "Allocator: 0 size allocation should result in nullptr.";
        ASSERT_NO_THROW(allocator.allocate(FreeListAllocator::MIN_CHUNK_SIZE)) << "Allocator: allocation should not throw";
        void *test = allocator.allocate(1);
        ASSERT_NO_THROW(allocator.deallocate(test)) << "Allocator: deallocation of valid ptr should not throw";
        ASSERT_NO_THROW(allocator.deallocate(buffer)) << "Allocator: deallocation of valid ptr should not throw";
        allocator.reset();
        ASSERT_NO_THROW(allocator.allocate(FreeListAllocator::MIN_CHUNK_SIZE)) << "Allocator: allocation should not throw";
        /* == undefined behavior == */
        ASSERT_NO_THROW(allocator.deallocate(buffer));
        ASSERT_NO_THROW(allocator.allocate(FreeListAllocator::MIN_CHUNK_SIZE));
    }
    {
        auto *buffer = allocator.allocate(FreeListAllocator::MIN_CHUNK_SIZE);
        void *buffer2 = nullptr;
        ASSERT_NO_THROW(buffer2 = allocator.allocate(8192)) << "Allocator: extra buffer should not throw at allocation.";
        ASSERT_NO_THROW(allocator.deallocate(buffer)) << "Allocator: extra buffer should not throw at deallocation.";
        ASSERT_NO_THROW(allocator.deallocate(buffer2)) << "Allocator: extra buffer should not throw at deallocation.";
    }
    {
        allocator.reset();
        auto *buffer = allocator.allocate(FreeListAllocator::MIN_CHUNK_SIZE - (512 + 2*sizeof(size_t)));
        void *buffer2 = allocator.allocate(512 - (256 + sizeof(size_t)));
        void *buffer3 = nullptr;
        ASSERT_NO_THROW(buffer3 = allocator.allocate(256)) << "Allocator: perfect fit should not throw.";
        char extern_buffer[512];
        ASSERT_THROW(allocator.deallocate(extern_buffer), spider::Exception) << "Allocator: extern buffer deallocation should throw";
        allocator.deallocate(buffer);
        allocator.deallocate(buffer2);
        allocator.deallocate(buffer3);
    }
}

TEST_F(allocatorTest, freeListAlloc) {
    auto allocator = FreeListAllocator("alloc", 512, FreeListPolicy::FIND_FIRST);
    freeListAllocTest(allocator);
    auto allocator2 = FreeListAllocator("alloc", 512, FreeListPolicy::FIND_BEST);
    freeListAllocTest(allocator2);
    ASSERT_NO_THROW(allocator.deallocate(nullptr)) << "Allocator: deallocate for nullptr should not throw";
    char buffer[512];
    allocator.reset();
    ASSERT_THROW(allocator.deallocate(&buffer), spider::Exception) << "Allocator: deallocate when no allocation ongoing should throw";
}

TEST_F(allocatorTest, freeListCtorTest) {
    ASSERT_THROW(FreeListAllocator("", 0, FreeListPolicy::FIND_BEST, 2), spider::Exception) << "FreeListAllocator should throw with alignement < 8.";
    ASSERT_NO_THROW(FreeListAllocator("", 1000)) << "FreeListAllocator should not throw with default ctor.";
}

TEST_F(allocatorTest, genericCtorTest) {
    ASSERT_NO_THROW(GenericAllocator("", 0)) << "GenericAllocator should not throw with default ctor.";
}

TEST_F(allocatorTest, genericAllocTest) {
    auto allocator = GenericAllocator("alloc", 8);
    ASSERT_NO_THROW(allocator.allocate(0)) << "GenericAllocator should not throw with 0 size allocation.";
    ASSERT_NO_THROW(allocator.deallocate(nullptr)) << "GenericAllocator should not throw with nullptr deallocation.";
    allocator.reset();
    
}