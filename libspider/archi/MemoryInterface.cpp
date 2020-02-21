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

#include <archi/MemoryInterface.h>
#include <common/Types.h>

/* === Method(s) implementation === */

spider::MemoryInterface::MemoryInterface(uint64_t size) : size_{ size }, used_{ 0 } {
    /* == Default routines == */
    allocateRoutine_ = [](u64 size) -> void * { return std::malloc(static_cast<size_t>(size)); };
    deallocateRoutine_ = [](void *addr) -> void { std::free(addr); };
}

/* === Private method(s) implementation === */

void *spider::MemoryInterface::read(uint64_t virtualAddress) {
    std::lock_guard<std::mutex> lockGuard{ lock_ };
    return retrievePhysicalAddress(virtualAddress);
}

void *spider::MemoryInterface::allocate(uint64_t virtualAddress, size_t size) {
    std::lock_guard<std::mutex> lockGuard{ lock_ };
    uint64_t res = UINT64_MAX;
    if (size <= available()) {
        used_ += size;
        res = size;
    }
    if (res != size) {
        throwSpiderException("failed to allocate %zu bytes.", size);
    }
    auto *physicalAddress = allocateRoutine_(size);
    if (!physicalAddress) {
        return nullptr;
    }
    registerPhysicalAddress(virtualAddress, physicalAddress);
    return physicalAddress;
}

void spider::MemoryInterface::deallocate(uint64_t virtualAddress, size_t size) {
    std::lock_guard<std::mutex> lockGuard{ lock_ };
    if (size > used_) {
        throwSpiderException("Deallocating more memory than used.");
    }
    used_ -= size;
    deallocateRoutine_(retrievePhysicalAddress(virtualAddress));
}

void spider::MemoryInterface::reset() {
    virtual2Phys_.clear();
}

/* === Private method(s) === */

void spider::MemoryInterface::registerPhysicalAddress(uint64_t virtualAddress, void *physicalAddress) {
    /* == Apply offset to virtual address to get the corresponding address associated to attached MemoryUnit == */
    virtual2Phys_[virtualAddress] = physicalAddress;
}

void *spider::MemoryInterface::retrievePhysicalAddress(uint64_t virtualAddress) {
    return virtual2Phys_.at(virtualAddress);
}
