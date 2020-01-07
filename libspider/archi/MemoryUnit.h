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

namespace spider {

    /* === Class definition === */

    class MemoryUnit {
    public:

        explicit MemoryUnit(uint64_t size = 0) : size_{ size }, used_{ 0 } { };

        ~MemoryUnit() = default;

        /* === Method(s) === */

        /**
         * @brief Allocate memory on the MemoryUnit (virtual allocation)
         * @param size  Size (in bytes) to allocate.
         * @return size value on success, UINT64_MAX on failure.
         */
        inline uint64_t allocate(uint64_t size) {
            if (size <= available()) {
                used_ += size;
                return size;
            }
            return UINT64_MAX;
        }

        /**
         * @brief Deallocate memory on the MemoryUnit (virtual deallocation)
         * @param size Size (in bytes) to deallocate.
         * @return new available size.
         * @throws spider::Exception if deallocating too much memory.
         */
        inline uint64_t deallocate(uint64_t size) {
            if (size > used_) {
                throwSpiderException("Deallocating more memory than used.");
            }
            used_ -= size;
            return available();
        }

        /* === Getter(s) === */

        /**
         * @brief Get the total available size (in bytes) of the MemoryUnit.
         * @return total size in bytes.
         */
        inline uint64_t size() const {
            return size_;
        }

        /**
         * @brief Get the total current memory usage (in bytes) of the MemoryUnit.
         * @return total current memory usage.
         */
        inline uint64_t used() const {
            return used_;
        }

        /**
         * @brief Get the current available memory (in bytes) of the MemoryUnit.
         * @return size() - used().
         */
        inline uint64_t available() const {
            return size_ - used_;
        }

    private:
        uint64_t size_ = 0; /* = Total size of the MemoryUnit = */
        uint64_t used_ = 0; /* = Currently used memory (strictly less or equal to size_) = */
    };
}
#endif //SPIDER2_MEMORYUNIT_H
