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
#ifndef SPIDER2_MEMORYUNIT_H
#define SPIDER2_MEMORYUNIT_H

/* === Include(s) === */

#include <cstdint>
#include <common/Exception.h>

/* === Class definition === */

class MemoryUnit {
public:

    MemoryUnit(void *base, std::uint64_t size);

    ~MemoryUnit() = default;

    /* === Method(s) === */

    inline void reset();

    inline void *physicalAddress(std::uintptr_t virtualAddress) const;

    inline std::uint64_t allocate(std::uint64_t size);

    /* === Getter(s) === */

    inline std::uint64_t size() const;

    inline std::uint64_t used() const;

    inline std::uint64_t available() const;

    inline std::uint32_t ix() const;

    /* === Setter(s) === */

    inline void setIx(std::uint32_t ix);

private:

    /* === Core properties === */

    void *base_ = nullptr;
    std::uint64_t size_ = 0;
    std::uint64_t used_ = 0;
    std::uint32_t ix_ = 0;

    /* === Routines === */

    /* === Private method(s) === */
};

/* === Inline method(s) === */

void MemoryUnit::reset() {
    used_ = 0;
}

void *MemoryUnit::physicalAddress(std::uint64_t virtualAddress) const {
    if (virtualAddress > size_) {
        throwSpiderException("Invalid memory address!");
    }
    const auto &physical = reinterpret_cast<std::uintptr_t>(base_) + virtualAddress;
    return reinterpret_cast<void *>(physical);
}

std::uint64_t MemoryUnit::allocate(std::uint64_t size) {
    // TODO: handle different scheme of allocation
    auto address = used_;
    used_ += size;
    return address;
}

std::uint64_t MemoryUnit::size() const {
    return size_;
}

std::uint64_t MemoryUnit::used() const {
    return used_;
}

std::uint64_t MemoryUnit::available() const {
    return size_ - used_;
}

std::uint32_t MemoryUnit::ix() const {
    return ix_;
}

void MemoryUnit::setIx(std::uint32_t ix) {
    ix_ = ix;
}

#endif //SPIDER2_MEMORYUNIT_H
