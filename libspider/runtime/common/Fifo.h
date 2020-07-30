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
#ifndef SPIDER2_RTFIFO_H
#define SPIDER2_RTFIFO_H

/* === Include(s) === */

#include <common/Types.h>

namespace spider {

    enum class FifoAttribute {
        RW_ONLY = 0, /*!< Owner of the FIFO does not own the associated memory:
                        *   --> no dealloc after read, no alloc before write */
        RW_OWN,      /*!< Owner of the FIFO own the associated memory:
                        *   --> dealloc after read, alloc before write */
        RW_EXT,      /*!< Owner of the FIFO reads (writes) from (to) external memory */
    };

    /* === Class definition === */

    struct RTFifo {
        size_t virtualAddress_ = SIZE_MAX;                /* = Virtual address of the Fifo = */
        u32 size_ = 0;                                    /* = Size of the Fifo = */
        u32 offset_ = 0;                                  /* = Offset in the address = */
        u32 count_ = 1;                                   /* = Number of use of this FIFO = */
        FifoAttribute attribute_ = FifoAttribute::RW_OWN; /* = Attribute of the Fifo = */
    };
}

#endif //SPIDER2_RTFIFO_H
