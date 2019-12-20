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
#ifndef SPIDER2_MEMORYINTERFACE_H
#define SPIDER2_MEMORYINTERFACE_H

/* === Include(s) === */

#include <api/global-api.h>
#include <common/Exception.h>
#include <containers/containers.h>

namespace spider {

    /* === Forward declaration(s) === */

    class MemoryUnit;

    /* === Class definition === */

    class MemoryInterface {
    public:
        MemoryInterface();

        ~MemoryInterface() = default;

        /* === Method(s) === */

        /**
         * @brief Read memory at the given memory virtual address.
         * @param virtualAddress  Virtual address to evaluate.
         * @return physical address corresponding to the virtual address.
         */
        void *read(uint64_t virtualAddress);

        /**
         * @brief Write memory at the given memory virtual address.
         * @param virtualAddress  Virtual address to evaluate.
         * @return true on success, false else.
         */
        bool write(uint64_t virtualAddress);

        /**
         * @brief Allocate memory to the given virtual address.
         * @param virtualAddress  Virtual address to evaluate.
         * @param size            Size of the memory to allocate.
         * @return physical memory addressed allocated.
         */
        void *allocate(uint64_t virtualAddress, size_t size);

        /**
         * @brief Deallocate memory from the given virtual address.
         * @param virtualAddress  Virtual address to evaluate.
         * @param size            Size of the memory to deallocate.
         */
        void deallocate(uint64_t virtualAddress, size_t size);

        /**
         * @brief Reset the memory interface and the attached MemoryUnit.
         */
        void reset();

        /**
         * @brief Get the cost of reading size bytes on the this MemoryInterface.
         * @param size  Number of bytes to read.
         * @return cost associated to reading size bytes.
         */
        inline uint64_t readCost(uint64_t size) const {
            return readCostRoutine_(size);
        }

        /**
         * @brief Get the cost of writing size bytes on the this MemoryInterface.
         * @param size  Number of bytes to write.
         * @return cost associated to writing size bytes.
         */
        inline uint64_t writeCost(uint64_t size) const {
            return writeCostRoutine_(size);
        }

        /* === Getter(s) === */

        inline MemoryUnit *memoryUnit() const {
            return memoryUnit_;
        }

        /* === Setter(s) === */

        /**
         * @brief Set the @refitem MemoryUnit for this interface.
         * @remark if nullptr, nothing happens.
         * @param memoryUnit Memory unit to set.
         * @throws spider::Exception if this interface already has a MemoryUnit.
         */
        inline void setMemoryUnit(MemoryUnit *memoryUnit) {
            if (memoryUnit_) {
                throwSpiderException("MemoryInterface already has a memory unit.");
            }
            if (memoryUnit) {
                memoryUnit_ = memoryUnit;
            }
        }

        /**
         * @brief Set the routine for writing memory.
         * @remark override current value.
         * @param routine  Routine to set.
         */
        inline void setWriteRoutine(MemoryWriteRoutine routine) {
            writeRoutine_ = routine;
        }

        /**
         * @brief Set the routine for allocating memory.
         * @remark override current value.
         * @param routine  Routine to set.
         */
        inline void setAllocateRoutine(MemoryAllocateRoutine routine) {
            allocateRoutine_ = routine;
        }

        /**
         * @brief Set the routine for deallocating memory.
         * @remark override current value.
         * @param routine  Routine to set.
         */
        inline void setDeallocateRoutine(MemoryDeallocateRoutine routine) {
            deallocateRoutine_ = routine;
        }

        /**
         * @brief Set the routine for the read cost.
         * @remark override current value.
         * @param routine  Routine to set.
         */
        inline void setReadCostRoutine(MemoryExchangeCostRoutine routine) {
            readCostRoutine_ = routine;
        }

        /**
         * @brief Set the routine for the write cost.
         * @remark override current value.
         * @param routine  Routine to set.
         */
        inline void setWriteCostRoutine(MemoryExchangeCostRoutine routine) {
            writeCostRoutine_ = routine;
        }

    private:
        MemoryUnit *memoryUnit_ = nullptr;                     /* = Pointer to the MemoryUnit attached to this MemoryInterface = */
        spider::unordered_map<uint64_t, void *> virtual2Phys_; /* = Map associating virtual address to physical ones = */

        /* === Memory write routine === */

        MemoryWriteRoutine writeRoutine_; /* = Memory write routine used by this MemoryInterface = */

        /* === Allocation routines === */

        MemoryAllocateRoutine allocateRoutine_;     /* = Memory allocation routine used for this MemoryInterface = */
        MemoryDeallocateRoutine deallocateRoutine_; /* = Memory deallocation routine used for this MemoryInterface = */

        /* === Memory exchange cost routines === */

        MemoryExchangeCostRoutine readCostRoutine_;  /* = Memory read exchange cost routine used for this MemoryInterface = */
        MemoryExchangeCostRoutine writeCostRoutine_; /* = Memory write exchange cost routine used for this MemoryInterface = */

        /* === Private method(s) === */

        /**
         * @brief Register a physical address associated with a given virtual address.
         * @param virtualAddress   Virtual address to evaluate.
         * @param physicalAddress  Physical address to register.
         */
        void registerPhysicalAddress(uint64_t virtualAddress, void *physicalAddress);

        /**
         * @brief Retrieve the physical address corresponding to the given virtual address.
         * @param virtualAddress   Virtual address to evaluate.
         * @return corresponding physical address, nullptr if not found.
         */
        void *retrievePhysicalAddress(uint64_t virtualAddress);
    };
}

#endif //SPIDER2_MEMORYINTERFACE_H
