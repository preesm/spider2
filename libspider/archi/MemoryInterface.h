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
#ifndef SPIDER2_MEMORYINTERFACE_H
#define SPIDER2_MEMORYINTERFACE_H

/* === Include(s) === */

#include <containers/unordered_map.h>
#include <api/global-api.h>
#include <common/Exception.h>

namespace spider {

    /* === Class definition === */

    class MemoryInterface {
    public:
        explicit MemoryInterface(uint64_t size = 0);

        ~MemoryInterface();

        /* === Method(s) === */

        /**
         * @brief Read memory at the given memory virtual address.
         * @remark if count is 0, the value is discarded.
         * @param address  Virtual address to evaluate.
         * @param count           Number of use of the buffer to add.
         * @return physical address corresponding to the virtual address.
         */
        void *read(uint64_t address, i32 count = 0);

        /**
         * @brief Update lifetime of a given buffer.
         * @param address Virtual address of the buffer.
         * @param count   Counter update to apply.
         */
        void update(uint64_t address, i32 count = 1);

        /**
         * @brief Allocate memory to the given virtual address.
         * @param address  Virtual address to evaluate.
         * @param size     Size of the memory to allocate.
         * @param count    Number of use of the buffer to set.
         * @return physical memory addressed allocated.
         */
        void *allocate(uint64_t address, size_t size, i32 count = 1);

        /**
         * @brief Deallocate memory from the given virtual address.
         * @param virtualAddress  Virtual address to evaluate.
         * @param size            Size of the memory to deallocate.
         */
        void deallocate(uint64_t virtualAddress, size_t size);

        /**
         * @brief Free every existing buffer with non-zero counter.
         */
        void collect();

        /**
         * @brief Reset the memory interface.
         */
        void clear();


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

        /* === Setter(s) === */

        /**
         * @brief Set the routine for allocating memory.
         * @remark override current value.
         * @param routine  Routine to set.
         */
        inline void setAllocateRoutine(MemoryAllocateRoutine routine) {
            allocateRoutine_ = std::move(routine);
        }

        /**
         * @brief Set the routine for deallocating memory.
         * @remark override current value.
         * @param routine  Routine to set.
         */
        inline void setDeallocateRoutine(MemoryDeallocateRoutine routine) {
            deallocateRoutine_ = std::move(routine);
        }

    private:
        struct buffer_t {
            void *buffer_;
            size_t size_;
            i32 count_;
        };
        /* = Map associating virtual address to physical ones = */
        spider::unordered_map<uint64_t, buffer_t> virtual2Phys_;
        std::mutex lock_;
        /* = Total size of the MemoryUnit = */
        uint64_t size_ = 0;
        /* = Currently used memory (strictly less or equal to size_) = */
        uint64_t used_ = 0;

        /* === Allocation routines === */

        MemoryAllocateRoutine allocateRoutine_;     /* = Memory allocation routine used for this MemoryInterface = */
        MemoryDeallocateRoutine deallocateRoutine_; /* = Memory deallocation routine used for this MemoryInterface = */

        /* === Private method(s) === */

        /**
         * @brief Register a physical address associated with a given virtual address.
         * @param virtAddress Virtual address to evaluate.
         * @param phyAddress  Physical address to register.
         * @param size        Size of the memory to allocate.
         * @param count       Number of use of the buffer to set.
         */
        void registerPhysicalAddress(uint64_t virtAddress, void *phyAddress, size_t size, i32 count = 1);

        /**
         * @brief Retrieve the physical address corresponding to the given virtual address.
         * @param virtualAddress   Virtual address to evaluate.
         * @return corresponding physical address, nullptr if not found.
         */
        buffer_t *retrieveBuffer(uint64_t virtualAddress);
    };
}

#endif //SPIDER2_MEMORYINTERFACE_H
