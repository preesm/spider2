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

class allocatorTest : public ::testing::Test {
protected:
};

TEST_F(allocatorTest, linearAllocCtorTest) {
    ASSERT_THROW(LinearStaticAllocator("", 0), spider::Exception) << "LinearStaticAllocator should throw with 0 size.";
    ASSERT_THROW(LinearStaticAllocator("", 1000, 4), spider::Exception) << "LinearStaticAllocator should throw with improper alignment size.";
    ASSERT_NO_THROW(LinearStaticAllocator("", 1000)) << "LinearStaticAllocator should not throw with default ctor.";
    ASSERT_THROW(LinearStaticAllocator("", 10, nullptr), spider::Exception) << "LinearStaticAllocator should throw with nullptr extern buffer.";
    char tmp[500];
    ASSERT_THROW(LinearStaticAllocator("", 0, &tmp), spider::Exception) << "LinearStaticAllocator should throw with 0 size and extern buffer.";
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

TEST_F(allocatorTest, abstractAllocNameTest) {
    auto allocator = LinearStaticAllocator("alloc", 512);
    ASSERT_STREQ(allocator.name(), "alloc") << "LinearStaticAllocator: name() failed.";
}

TEST_F(allocatorTest, stdContainersCtorTest) {
}