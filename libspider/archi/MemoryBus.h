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
#ifndef SPIDER2_MEMORYBUS_H
#define SPIDER2_MEMORYBUS_H

/* === Include(s) === */

#include <api/global-api.h>
#include <common/Types.h>

namespace spider {

    /* === Forward declaration(s) === */

    class MemoryInterface;

    class RTKernel;

    /* === Class definition === */

    class MemoryBus {
    public:
        MemoryBus();

        ~MemoryBus() = default;

        /* === Method(s) === */

        /**
         * @brief Get the cost of sending size bytes on the this MemoryBus.
         * @param size  Number of bytes to read.
         * @return cost associated to reading size bytes.
         */
        uint64_t sendCost(uint64_t size) const;

        /**
         * @brief Get the cost of receiving size bytes on the this MemoryBus.
         * @param size  Number of bytes to write.
         * @return cost associated to writing size bytes.
         */
        uint64_t receiveCost(uint64_t size) const;

        /**
         * @brief Send data over the bus.
         * @param size     Size in bytes to send.
         * @param packetIx Ix of the packet sent.
         * @param buffer   Buffer to send.
         */
        void dataSend(i64 size, i32 packetIx, void *buffer);

        /**
         * @brief Receive data over the bus.
         * @param size     Size in bytes to receive.
         * @param packetIx Ix of the packet to receive.
         * @param buffer   Buffer of the receive data.
         */
        void dataReceive(i64 size, i32 packetIx, void *buffer);

        /* === Getter(s) === */

        /**
         * @brief Get the runtime kernel associated with this memory bus send routine.
         * @return pointer to the kernel, nullptr else.
         */
        RTKernel *sendKernel() const;

        /**
         * @brief Get the runtime kernel associated with this memory bus receive routine.
         * @return pointer to the kernel, nullptr else.
         */
        RTKernel *receiveKernel() const;

        /**
         * @brief Get the write speed of this memory bus.
         * @return writing speed of the bus in bytes / s.
         */
        uint64_t writeSpeed() const;

        /**
         * @brief Get the write speed of this memory bus.
         * @return reading speed of the bus in bytes / s.
         */
        uint64_t readSpeed() const;

        /* === Setter(s) === */

        /**
         * @brief Set the routine for the sending cost on this bus.
         * @remark override current value.
         * @param routine  Routine to set.
         */
        void setSendCostRoutine(MemoryExchangeCostRoutine routine);

        /**
         * @brief Set the routine for the receive cost on this bus.
         * @remark override current value.
         * @param routine  Routine to set.
         */
        void setReceiveCostRoutine(MemoryExchangeCostRoutine routine);

        /**
         * @brief Set the routine for sending data on this bus.
         * @remark override current value.
         * @param routine  Routine to set.
         */
        void setSendRoutine(MemoryBusRoutine routine);

        /**
         * @brief Set the routine for receiving data on this bus.
         * @remark override current value.
         * @param routine  Routine to set.
         */
        void setReceiveRoutine(MemoryBusRoutine routine);

        /**
         * @brief Set the routine for sending data on this bus.
         * @remark override current value.
         * @param routine  Routine to set.
         */
        void setWriteSpeed(uint64_t value);

        /**
         * @brief Set the routine for receiving data on this bus.
         * @remark override current value.
         * @param routine  Routine to set.
         */
        void setReadSpeed(uint64_t value);

    private:
        MemoryExchangeCostRoutine sendCostRoutine_;    /* = Memory send exchange cost routine used for this MemoryBus = */
        MemoryExchangeCostRoutine receiveCostRoutine_; /* = Memory receive exchange cost routine used for this MemoryBus = */
        MemoryBusRoutine sendRoutine_;                 /* = Memory send routine used by this MemoryBus = */
        MemoryBusRoutine receiveRoutine_;              /* = Memory receive routine used by this MemoryBus = */
        uint64_t writeSpeed_ = 0;                      /* = Memory bus write speed in bytes / s = */
        uint64_t readSpeed_ = 0;                       /* = Memory bus read speed in bytes / s = */
        mutable size_t sendKernelIx_ = SIZE_MAX;       /* = Ix of the send kernel = */
        mutable size_t recvKernelIx_ = SIZE_MAX;       /* = Ix of the receive kernel = */

        /* === Private method(s) === */

        static void send(const int64_t *paramsIN, int64_t *, void *in[], void *[]);

        static void receive(const int64_t *paramsIN, int64_t *, void *[], void *out[]);
    };
}
#endif //SPIDER2_MEMORYBUS_H
