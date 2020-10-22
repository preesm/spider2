/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_SYNCSCHEDVERTEX_H
#define SPIDER2_SYNCSCHEDVERTEX_H

/* === Include(s) === */

#include <graphs/sched/SchedVertex.h>

namespace spider {

    class MemoryBus;

    namespace sched {

        enum class SyncType : u8 {
            SEND,
            RECEIVE
        };

        /* === Class definition === */

        class SyncVertex final : public sched::Vertex {
        public:

            explicit SyncVertex(SyncType type, MemoryBus *bus) : sched::Vertex(), bus_{ bus }, type_{ type } { }

            ~SyncVertex() final = default;

            /* === Method(s) === */

            u64 timingOnPE(const PE *pe) const final;

            inline u32 color() const final {
                /* ==  SEND    -> vivid tangerine color == */
                /* ==  RECEIVE -> Studio purple color == */
                return type_ == SyncType::SEND ? 0xff9478 : 0x8e44ad;
            }

            /* === Getter(s) === */

            inline sched::Type type() const final { return type_ == SyncType::SEND ? Type::SEND : Type::RECEIVE; }

            inline std::string name() const final { return type_ == SyncType::SEND ? "send" : "receive"; }

            /* === Setter(s) === */

        private:
            const MemoryBus *bus_{ nullptr }; /*!< Memory bus used by the task */
            SyncType type_;

            /* === Private method(s) === */

            u32 getKernelIx() const final;

            spider::unique_ptr<i64> buildInputParams() const final;

        };
    }
}
#endif //SPIDER2_SYNCSCHEDVERTEX_H
