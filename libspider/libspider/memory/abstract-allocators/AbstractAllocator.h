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
#ifndef SPIDER2_ABSTRACTALLOCATOR_H
#define SPIDER2_ABSTRACTALLOCATOR_H

/* === Includes === */

#include <cstdlib>
#include <cstdint>
#include <cinttypes>
#include <common/Logger.h>
#include <common/Exception.h>
#include <common/Math.h>

/* === Enumeration(s) === */

enum class FreeListPolicy {
    FIND_FIRST = 0,
    FIND_BEST = 1
};

/* === Class definition === */

class AbstractAllocator {
public:
    explicit AbstractAllocator(std::string name, std::size_t alignment = 0) : used_{ 0 },
                                                                              peak_{ 0 },
                                                                              averageUse_{ 0 },
                                                                              numberAverage_{ 0 },
                                                                              alignment_{ alignment },
                                                                              name_{ std::move(name) } { }

    virtual ~AbstractAllocator() {
        if (used_ > 0 && log_enabled()) {
            spider::log::error("Allocator: %s -- Still has %lf %s in use.\n",
                               getName(),
                               AbstractAllocator::getByteNormalizedSize(used_),
                               AbstractAllocator::getByteUnitString(used_));
        }
        printStats();
    }

    /**
     * @brief Allocate a memory buffer.
     * @param size Size of the buffer to allocate
     * @return pointer to allocated memory, nullptr on failure or if size is 0
     */
    virtual void *allocate(std::size_t size) = 0;

    /**
     * @brief Free a memory buffer.
     * @param ptr Memory address to be freed
     */
    virtual void deallocate(void *ptr) = 0;

    /* Setters */

    /**
     * @brief Set memory allocation alignment.
     *        All new allocation made after this call result in allocation aligned to new value.
     * @param alignment  New allocation value.
     */
    inline void setAllocationAlignment(std::size_t alignment);

    /* Getters */

    /**
     * @brief Fetch current memory allocation alignment
     * @return current allocation alignment
     */
    inline std::size_t getAllocationAlignment() const;

    /**
     * @brief Return name of the allocator.
     * @return name of the allocator
     */
    inline const char *getName() const;

    /* Methods */

    /**
     * @brief Print allocator usage statistics (peak usage, average usage)
     */
    inline void printStats() const;

protected:
    std::uint64_t used_ = 0;
    std::uint64_t peak_ = 0;
    std::uint64_t averageUse_ = 0;
    std::uint64_t numberAverage_ = 0;
    std::size_t alignment_ = 0;

    static inline std::size_t computeAlignedSize(std::size_t size, std::size_t alignment = 4096);

    static inline std::size_t computePadding(std::size_t base, std::size_t alignment);

    static inline std::size_t
    computePaddingWithHeader(std::size_t base, std::size_t alignment, std::size_t headerSize);

    static inline const char *getByteUnitString(std::uint64_t size);

    static inline double getByteNormalizedSize(std::uint64_t size);

private:
    std::string name_;
};

/* === Inline methods === */

void AbstractAllocator::setAllocationAlignment(std::size_t alignment) {
    alignment_ = alignment;
}

std::size_t AbstractAllocator::getAllocationAlignment() const {
    return alignment_;
}

const char *AbstractAllocator::getName() const {
    return name_.c_str();
}

void AbstractAllocator::printStats() const {
    if (log_enabled()) {
        spider::log::info("Allocator: %s\n", getName());
        spider::log::info("       ==> max usage:    %" PRIu64" B (%.6lf %s)\n",
                          peak_,
                          getByteNormalizedSize(peak_),
                          getByteUnitString(peak_));
        if (averageUse_) {
            spider::log::info("       ==> avg usage:    %" PRIu64" B (%.6lf %s)\n",
                              averageUse_ / numberAverage_,
                              getByteNormalizedSize(averageUse_ / numberAverage_),
                              getByteUnitString(averageUse_ / numberAverage_));
        }
        spider::log::info("       ==> still in use: %" PRIu64" B (%.6lf %s)\n",
                          used_,
                          getByteNormalizedSize(used_),
                          getByteUnitString(used_));
    }
}

std::size_t AbstractAllocator::computeAlignedSize(std::size_t size, std::size_t alignment /* = 4096 */) {
    const auto &alignFactor = spider::math::ceilDiv(size, alignment);
    return alignFactor * alignment;
}

std::size_t AbstractAllocator::computePadding(std::size_t base, std::size_t alignment) {
    return computeAlignedSize(base, alignment) - base;
}

std::size_t
AbstractAllocator::computePaddingWithHeader(std::size_t base, std::size_t alignment, std::size_t headerSize) {
    auto &&padding = computePadding(base, alignment);
    if (padding < headerSize) {
        const auto &neededSpace = headerSize - padding;
        padding += (alignment * (neededSpace / alignment));
        if (neededSpace % alignment > 0) {
            padding += alignment;
        }
    }
    return padding;
}

const char *AbstractAllocator::getByteUnitString(std::uint64_t size) {
    constexpr std::uint64_t SIZE_GB = 1024 * 1024 * 1024;
    constexpr std::uint64_t SIZE_MB = 1024 * 1024;
    constexpr std::uint64_t SIZE_KB = 1024;
    if (size / SIZE_GB) {
        return "GB";
    } else if (size / SIZE_MB) {
        return "MB";
    } else if (size / SIZE_KB) {
        return "KB";
    }
    return "B";
}

double AbstractAllocator::getByteNormalizedSize(std::uint64_t size) {
    constexpr double SIZE_GB = 1024 * 1024 * 1024;
    constexpr double SIZE_MB = 1024 * 1024;
    constexpr double SIZE_KB = 1024;
    const auto dblSize = (double) size;
    if (dblSize / SIZE_GB >= 1.) {
        return dblSize / SIZE_GB;
    } else if (dblSize / SIZE_MB >= 1.) {
        return dblSize / SIZE_MB;
    } else if (dblSize / SIZE_KB >= 1.) {
        return dblSize / SIZE_KB;
    }
    return dblSize;
}


#endif //SPIDER2_ABSTRACTALLOCATOR_H
