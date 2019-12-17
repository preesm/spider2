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
#ifndef SPIDER2_ABSTRACTALLOCATORPOLICY_H
#define SPIDER2_ABSTRACTALLOCATORPOLICY_H

/* === Includes === */

#include <cstdlib>
#include <cstdint>
#include <cinttypes>
#include <common/Logger.h>
#include <common/Exception.h>
#include <common/Math.h>

/* === Class definition === */

class AbstractAllocatorPolicy {
public:

    explicit AbstractAllocatorPolicy(size_t alignment = 0) : usage_{ 0 }, alignment_{ alignment } { }

    virtual ~AbstractAllocatorPolicy() noexcept = default;

    /**
     * @brief Allocate a memory buffer.
     * @param size Size of the buffer to allocate
     * @return pointer to allocated memory, nullptr on failure or if size is 0
     */
    virtual void *allocate(size_t &&size) = 0;

    /**
     * @brief Free a memory buffer.
     * @param ptr Memory address to be freed
     * @return size of the freed buffer
     */
    virtual size_t deallocate(void *ptr) = 0;

    /* === Setter(s) === */

    /**
     * @brief Set memory allocation alignment.
     *        All new allocation made after this call result in allocation aligned to new value.
     * @param alignment  New allocation value.
     */
    inline void setAllocationAlignment(size_t alignment) noexcept {
        alignment_ = alignment;
    }

    /* === Getter(s) === */

    /**
     * @brief Returns current memory allocation alignment
     * @return memory alignment value
     */
    inline size_t alignment() const noexcept {
        return alignment_;
    }

    /**
     * @brief Returns current memory usage.
     * @return current memory usage.
     */
    inline size_t usage() const noexcept {
        return usage_;
    }

protected:
    uint64_t usage_ = 0;
    size_t alignment_ = 0;

    static inline size_t computeAlignedSize(size_t size, size_t alignment) noexcept {
        return size + computePadding(size, alignment);
    }

    static inline size_t computePadding(size_t size, size_t alignment) noexcept {
        const auto &byteAlignment = (size % alignment);
        return byteAlignment ? alignment - byteAlignment : 0;
    }

    static inline size_t
    computePaddingWithHeader(size_t size, size_t alignment, size_t headerSize) noexcept {
        return computePadding(size + headerSize, alignment);
    }
};

#endif //SPIDER2_ABSTRACTALLOCATORPOLICY_H
