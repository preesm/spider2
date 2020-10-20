/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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
    allocateRoutine_ = [](u64 sizeAlloc) -> void * { return std::malloc(static_cast<size_t>(sizeAlloc)); };
    deallocateRoutine_ = [](void *addr) -> void { std::free(addr); };
}

spider::MemoryInterface::~MemoryInterface() {
#ifndef NDEBUG
    if (log::enabled<log::MEMORY>()) {
        for (const auto &buff : virtual2Phys_) {
            if (buff.second.count_) {
                log::print<log::MEMORY>(log::yellow, "INFO",
                                        "PHYSICAL: [%p] remaining: %zu bytes at address %zu with count: %u.\n", this,
                                        buff.second.size_,
                                        buff.first,
                                        buff.second.count_);
            }
        }
    }
#endif
}

/* === Private method(s) implementation === */

void *spider::MemoryInterface::read(uint64_t virtualAddress, i32 count) {
    std::lock_guard<std::mutex> lockGuard{ lock_ };
    auto *buffer = retrieveBuffer(virtualAddress);
    buffer->count_ += count;
    return buffer->buffer_;
}

void *spider::MemoryInterface::allocate(uint64_t virtualAddress, size_t size, i32 count) {
    if (!size) {
        return nullptr;
    }
    std::lock_guard<std::mutex> lockGuard{ lock_ };
    if (log::enabled<log::MEMORY>()) {
        log::print<log::MEMORY>(log::yellow, "INFO", "PHYSICAL: [%p] allocating: %zu bytes at address %zu.\n", this,
                                size,
                                virtualAddress);
    }
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
    registerPhysicalAddress(virtualAddress, physicalAddress, size, count);
    return physicalAddress;
}

void spider::MemoryInterface::deallocate(uint64_t virtualAddress, size_t size) {
    if (!size) {
        return;
    }
    std::lock_guard<std::mutex> lockGuard{ lock_ };
    auto *buffer = retrieveBuffer(virtualAddress);
    buffer->count_ -= 1;
#ifndef NDEBUG
    if (buffer->size_ > used_) {
        throwSpiderException("Deallocating more memory than used.");
    }
    if (buffer->count_ < 0) {
        throwSpiderException("Double free of a buffer.");
    }
#endif
    if (!buffer->count_) {
        if (log::enabled<log::MEMORY>()) {
            log::print<log::MEMORY>(log::green, "INFO", "PHYSICAL: [%p] deallocating: %zu bytes at address %zu.\n",
                                    this, buffer->size_, virtualAddress);
        }
        used_ -= buffer->size_;
        deallocateRoutine_(buffer->buffer_);
    }
}

void spider::MemoryInterface::clear() {
    std::lock_guard<std::mutex> lockGuard{ lock_ };
    virtual2Phys_.clear();
}

void spider::MemoryInterface::garbageCollect() {
    std::lock_guard<std::mutex> lockGuard{ lock_ };
    for (auto &elt : virtual2Phys_) {
        auto &buffer = elt.second;
        if (buffer.count_ < 0) {
            used_ -= buffer.size_;
            deallocateRoutine_(buffer.buffer_);
            buffer.count_ = 0;
            buffer.buffer_ = nullptr;
        }
    }
}

/* === Private method(s) === */

void spider::MemoryInterface::registerPhysicalAddress(uint64_t virtAddress, void *phyAddress, size_t size, i32 count) {
    /* == Apply offset to virtual address to get the corresponding address associated to attached MemoryUnit == */
    virtual2Phys_[virtAddress] = buffer_t{ phyAddress, size, count };
}

spider::MemoryInterface::buffer_t *spider::MemoryInterface::retrieveBuffer(uint64_t virtualAddress) {
    if (log::enabled<log::MEMORY>()) {
        log::print<log::MEMORY>(log::red, "INFO", "PHYSICAL: [%p] fetching address: %zu.\n", this, virtualAddress);
    }
#ifndef NDEBUG
    try {
        return &virtual2Phys_.at(virtualAddress);
    } catch (const std::out_of_range &) {
        log::print<log::MEMORY>(log::red, "ERROR", " [%p] accessing bad memory address.\n",
                                reinterpret_cast<void *>(this));
        throw;
    }
#else
    return &virtual2Phys_[virtualAddress];
#endif
}
