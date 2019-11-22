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

/* === Includes === */

#include <memory/static-allocators/LIFOStaticAllocator.h>

/* === Methods implementation === */

LIFOStaticAllocator::LIFOStaticAllocator(std::string name, size_t totalSize) :
        StaticAllocator(std::move(name), totalSize, sizeof(uint64_t)) {

}

LIFOStaticAllocator::LIFOStaticAllocator(std::string name, size_t totalSize, void *externalBase) :
        StaticAllocator(std::move(name), totalSize, externalBase, sizeof(uint64_t)) {

}

void *LIFOStaticAllocator::allocate(size_t size) {
    if (!size) {
        return nullptr;
    }
    used_ += size;
    /*! We assume alignment on 64 bits */
    const auto &alignedSize = AbstractAllocator::computeAlignedSize(static_cast<size_t>(used_), alignment_);
    if (alignedSize > totalSize_) {
        throwSpiderException("Memory request exceed memory available. Stack: %s -- Size: %"
                                     PRIu64
                                     " -- Requested: %"
                                     PRIu64
                                     "", getName(), totalSize_, alignedSize);
    }
    const auto &alignedAllocatedAddress = reinterpret_cast<uintptr_t>(startPtr_) + used_ - size;
    used_ = alignedSize;
    peak_ = std::max(peak_, used_);
    return reinterpret_cast<void *>(alignedAllocatedAddress);
}

void LIFOStaticAllocator::deallocate(void *ptr) {
    if (!ptr) {
        return;
    }
    StaticAllocator::checkPointerAddress(ptr);
    const auto &currentAddress = reinterpret_cast<uintptr_t >(ptr);
    if (currentAddress > (used_ + reinterpret_cast<uintptr_t>(startPtr_))) {
        throwSpiderException(
                "Allocator: %s -- LIFO allocator should deallocate element in reverse order of allocation.",
                getName());
    }
    used_ = currentAddress - reinterpret_cast<uintptr_t>(startPtr_);
}

void LIFOStaticAllocator::reset() {
    averageUse_ += used_;
    numberAverage_++;
    used_ = 0;
}
