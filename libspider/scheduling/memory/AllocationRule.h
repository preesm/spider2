/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_ALLOCATIONRULE_H
#define SPIDER2_ALLOCATIONRULE_H

/* === Include(s) === */

#include <common/Types.h>
#include <runtime/common/Fifo.h>

namespace spider {
    namespace sched {

        enum AllocType : u8 {
            NEW,      /*!< Specify that a new FIFO should be allocated */
            SAME_IN,  /*!< Specify that an existing input FIFO should be used */
            SAME_OUT, /*!< Specify that an existing output FIFO should be used */
            EXT,      /*!< Specify that allocation of FIFO is external */
            MERGE,    /*!< Specify that allocation of FIFO requires to merge multiple FIFOs */
            REPEAT,   /*!< Specify that allocation of FIFO requires to repeat input FIFO */
        };

        struct AllocationRule {
            AllocationRule *others_ = nullptr;
            u32 size_ = UINT32_MAX;
            u32 offset_ = 0u;
            u32 fifoIx_ = 0u;
            u32 count_ = 0u;
            AllocType type_ = AllocType::NEW;
            FifoAttribute attribute_ = FifoAttribute::RW_OWN;

            AllocationRule() = default;

            ~AllocationRule() noexcept = default;

            AllocationRule(const AllocationRule &) = default;

            AllocationRule(AllocationRule &&) = default;

            AllocationRule &operator=(const AllocationRule &) = default;

            AllocationRule &operator=(AllocationRule &&) = default;

            AllocationRule(u32 size, u32 offset, u32 ix, u32 count, AllocType type, FifoAttribute attribute) :
                    others_{ nullptr }, size_{ size }, offset_{ offset }, fifoIx_{ ix }, count_{ count }, type_{ type },
                    attribute_{ attribute } {
            }
        };
    }
}

#endif //SPIDER2_ALLOCATIONRULE_H
