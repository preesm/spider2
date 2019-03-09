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
#ifndef SPIDER2_SPIDERALLOCATOR_H
#define SPIDER2_SPIDERALLOCATOR_H

#include <cstdlib>
#include <cstdint>
#include <cinttypes>
#include "common/Logger.h"
#include "common/SpiderException.h"

class SpiderAllocator {
public:
    explicit inline SpiderAllocator(const char *name, std::int32_t alignment);

    ~SpiderAllocator() = default;

    /**
     * @brief Allocate a memory buffer.
     * @param size Size of the buffer to allocate
     * @return pointer to allocated memory, nullptr on failure or if size is 0
     */
    virtual void *alloc(std::uint64_t size) = 0;

    /**
     * @brief Free a memory buffer.
     * @param ptr Memory address to be freed
     */
    virtual void free(void *ptr) = 0;

    /* Setters */

    /**
     * @brief Set memory allocation alignment.
     *        All new allocation made after this call result in allocation aligned to new value.
     * @param alignment  New allocation value.
     */
    inline void setAllocationAlignment(std::int32_t alignment);

    /* Getters */

    /**
     * @brief Fetch current memory allocation alignment
     * @return current allocation alignment
     */
    inline std::int32_t getAllocationAlignment() const;

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
    std::uint64_t used_;
    std::uint64_t peak_;
    std::uint64_t averageUse_;
    std::uint64_t numberAverage_;
    std::int32_t alignment_;

    static inline std::uint64_t computeAlignedSize(std::uint64_t &size, std::int32_t alignment);

    static inline std::uint64_t computeAlignedSize(std::uint64_t &size);

    static inline std::int32_t computePadding(std::uint64_t &base, std::int32_t alignment);

    static inline const char *getByteUnitString(std::uint64_t &size);

    static inline double getByteNormalizedSize(std::uint64_t &size);

private:
    const char *name_;
};

SpiderAllocator::SpiderAllocator(const char *name, std::int32_t alignment) : used_{0},
                                                                             peak_{0},
                                                                             averageUse_{0},
                                                                             numberAverage_{0},
                                                                             alignment_{alignment},
                                                                             name_{name} {

}

void SpiderAllocator::setAllocationAlignment(std::int32_t alignment) {
    alignment_ = alignment;
}

std::int32_t SpiderAllocator::getAllocationAlignment() const {
    return alignment_;
}

const char *SpiderAllocator::getName() const {
    return name_;
}

void SpiderAllocator::printStats() const {
    Logger::print(LOG_GENERAL, LOG_INFO, "Allocator: %s\n", getName());
    Logger::print(LOG_GENERAL, LOG_INFO, "       ==> max usage: %" PRIu64"\n", peak_);
    Logger::print(LOG_GENERAL, LOG_INFO, "       ==> avg usage: %" PRIu64"\n", averageUse_ / numberAverage_);
}

std::uint64_t SpiderAllocator::computeAlignedSize(std::uint64_t &size, std::int32_t alignment) {
    std::uint64_t alignFactor = size / alignment + (size % alignment != 0); // ceil(size / pageSize)
    return alignFactor * alignment;
}

std::uint64_t SpiderAllocator::computeAlignedSize(std::uint64_t &size) {
    std::int32_t alignment = 4096;
    return computeAlignedSize(size, alignment);
}

std::int32_t SpiderAllocator::computePadding(std::uint64_t &base, std::int32_t alignment) {
    return static_cast<int32_t>(computeAlignedSize(base, alignment) - base);
}

const char *SpiderAllocator::getByteUnitString(std::uint64_t &size) {
    const std::uint64_t sizeGB = 1024 * 1024 * 1024;
    const std::uint64_t sizeMB = 1024 * 1024;
    const std::uint64_t sizeKB = 1024;
    if (size / sizeGB) {
        return "GB";
    } else if (size / sizeMB) {
        return "MB";
    } else if (size / sizeKB) {
        return "KB";
    }
    return "B";
}

double SpiderAllocator::getByteNormalizedSize(std::uint64_t &size) {
    const double sizeGB = 1024 * 1024 * 1024;
    const double sizeMB = 1024 * 1024;
    const double sizeKB = 1024;
    const double dblSize = (double) size;
    if (dblSize / sizeGB >= 1.) {
        return dblSize / sizeGB;
    } else if (dblSize / sizeMB >= 1.) {
        return dblSize / sizeMB;
    } else if (dblSize / sizeKB >= 1.) {
        return dblSize / sizeKB;
    }
    return dblSize;
}

#endif //SPIDER2_SPIDERALLOCATOR_H
