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
#ifndef SPIDER2_FIFO_H
#define SPIDER2_FIFO_H

/* === Include(s) === */

#include <common/Types.h>
#include <containers/array.h>

namespace spider {

    enum class FifoAttribute : u8 {
        RW_ONLY = 0, /*!< Owner of the FIFO does not own the associated memory:
                        *   --> no dealloc after read, no alloc before write */
        RW_OWN,      /*!< Owner of the FIFO own the associated memory:
                        *   --> dealloc after read, alloc before write */
        RW_EXT,      /*!< Owner of the FIFO reads (writes) from (to) external memory */
        R_MERGE,     /*!< Owner of the FIFO needs to merge multiple FIFOs together */
        R_REPEAT,    /*!< Owner of the FIFO needs to repeat the input FIFO a given number of times */
        W_SINK,      /*!< Owner of the FIFO writes to a sink, i.e FIFO is useless */
        RW_AUTO,     /*!< Owner of the FIFO allocates / reads a FIFO that will be automatically managed */
        DUMMY,       /*!< Sentry for synchronization */
        First = RW_ONLY, /*!< Sentry for EnumIterator::begin */
        Last = DUMMY     /*!< Sentry for EnumIterator::end */
    };

    constexpr auto FIFO_ATTR_COUNT = static_cast<u8>(FifoAttribute::Last) - static_cast<u8>(FifoAttribute::First) + 1u;

    /* === Class definition === */

    struct FifoAlloc {
        size_t address_; /* = Virtual address of the Fifo = */
        u32 offset_;     /* = Offset in the address = */
    };

    struct Fifo {
        size_t address_;          /* = Virtual address of the Fifo = */
        u32 size_;                /* = Size of the Fifo = */
        u32 offset_;              /* = Offset in the address = */
        i32 count_;               /* = Number of use of this FIFO = */
        FifoAttribute attribute_; /* = Attribute of the Fifo = */

        Fifo() : address_{ SIZE_MAX },
                 size_{ 0u },
                 offset_{ 0u },
                 count_{ 0u },
                 attribute_{ FifoAttribute::RW_OWN } { }

        Fifo(size_t address, u32 size, u32 offset, i32 count, spider::FifoAttribute attribute) :
                address_{ address }, size_{ size }, offset_{ offset }, count_{ count }, attribute_{ attribute } {
        }
    };

    /* === Function(s) declaration === */

    class MemoryInterface;

    spider::array<void *> getInputBuffers(const array_view<Fifo> &fifos, MemoryInterface *memoryInterface);

    spider::array<void *> getOutputBuffers(const array_view<Fifo> &fifos, MemoryInterface *memoryInterface);
}

#endif //SPIDER2_FIFO_H
