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

#include <memory/static-allocators/LinearStaticAllocator.h>

/* === Methods implementation === */

LinearStaticAllocator::LinearStaticAllocator(std::string name, std::uint64_t totalSize, std::int32_t alignment) :
        StaticAllocator(std::move(name), totalSize, alignment) {
    if (alignment < 8) {
        throwSpiderException("Memory alignment should be at least of size sizeof(std::int64_t) = 8 bytes.");
    }
}

LinearStaticAllocator::LinearStaticAllocator(std::string name,
                                             std::uint64_t totalSize,
                                             void *externalBase,
                                             int32_t alignment) : StaticAllocator(std::move(name), totalSize,
                                                                                  externalBase,
                                                                                  alignment) {
    if (alignment < 8) {
        throwSpiderException("Memory alignment should be at least of size sizeof(std::int64_t) = 8 bytes.");
    }
}

void *LinearStaticAllocator::allocate(std::uint64_t size) {
    if (!size) {
        return nullptr;
    }
    std::int32_t padding = 0;
    if (alignment_ && used_ % alignment_ != 0) {
        /*!< Compute next aligned address padding */
        padding = AbstractAllocator::computePadding(used_, alignment_);
    }

    std::uint64_t requestedSize = used_ + padding + size;
    if (requestedSize > totalSize_) {
        throwSpiderException("Memory request exceed memory available. Stack: %s -- Size: %"
                                     PRIu64
                                     " -- Requested: %"
                                     PRIu64, getName(), totalSize_, requestedSize);
    }
    const auto &alignedAllocatedAddress = reinterpret_cast<std::uintptr_t>(startPtr_) + used_ + padding;
    used_ += (size + padding);
    peak_ = std::max(peak_, used_);
    return reinterpret_cast<void *>(alignedAllocatedAddress);
}

void LinearStaticAllocator::deallocate(void *ptr) {
    StaticAllocator::checkPointerAddress(ptr);
    /*!< LinearStaticAllocator does not free memory per block */
}

void LinearStaticAllocator::reset() {
    averageUse_ += used_;
    numberAverage_++;
    used_ = 0;
}
