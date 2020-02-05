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
#ifndef SPIDER2_MEMORYBUS_H
#define SPIDER2_MEMORYBUS_H

/* === Include(s) === */

#include <cstdint>
#include <api/global-api.h>

namespace spider {

    /* === Forward declaration(s) === */

    class MemoryInterface;

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

        /* === Getter(s) === */

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

    private:
        MemoryExchangeCostRoutine sendCostRoutine_;    /* = Memory send exchange cost routine used for this MemoryBus = */
        MemoryExchangeCostRoutine receiveCostRoutine_; /* = Memory receive exchange cost routine used for this MemoryBus = */
        MemoryBusRoutine sendRoutine_;                 /* = Memory send routine used by this MemoryBus = */
        MemoryBusRoutine receiveRoutine_;              /* = Memory receive routine used by this MemoryBus = */

    };
}
#endif //SPIDER2_MEMORYBUS_H