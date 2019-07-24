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
#ifndef MEMORY_ALLOCATORTEST_H
#define MEMORY_ALLOCATORTEST_H


#include <gtest/gtest.h>
#include <memory/Allocator.h>


TEST(AllocatorTest, TryAlloc) {
    AllocatorConfig cfg;
    cfg.allocatorType = AllocatorType::FREELIST;
    cfg.size = 512;
    Spider::initAllocator(StackID::PISDF, cfg);
    EXPECT_THROW(Spider::allocate<double>(StackID::SCHEDULE, 10), SpiderException);
    cfg.allocatorType = AllocatorType::FREELIST_STATIC;
    cfg.size = 0;
    Spider::initAllocator(StackID::SRDAG, cfg);
    EXPECT_THROW(Spider::allocate<double>(StackID::SRDAG, 10), SpiderException);
    EXPECT_NO_THROW(Spider::allocate<double>(StackID::PISDF, 10));
    Spider::finalizeAllocators();
}

TEST(AllocatorTest, TryDealloc) {
    AllocatorConfig cfg;
    cfg.allocatorType = AllocatorType::FREELIST;
    cfg.size = 512;
    Spider::initAllocator(StackID::PISDF, cfg);
    auto *array = Spider::allocate<double>(StackID::PISDF, 10);
    EXPECT_NO_THROW(Spider::deallocate(array));
    Spider::finalizeAllocators();
}


#endif //MEMORY_ALLOCATORTEST_H
