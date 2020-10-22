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
#ifndef SPIDER2_SPECIALSCHEDVERTEX_H
#define SPIDER2_SPECIALSCHEDVERTEX_H

/* === Include(s) === */

#include <graphs/sched/SchedVertex.h>

namespace spider {

    class MemoryBus;

    namespace sched {

        /* === Class definition === */

        class SpecialVertex final : public sched::Vertex {
        public:

            explicit SpecialVertex(Type type, size_t edgeINCount, size_t edgeOUTCount);

            ~SpecialVertex() final = default;

            /* === Method(s) === */

            u64 timingOnPE(const PE *pe) const final;

            bool reduce(sched::Graph *) final;

            inline u32 color() const final {
                if (type_ == Type::MERGE) {
                    /* == Studio Purple == */
                    return 0x8e44ad;
                } else if (type_ == Type::FORK) {
                    /* == Buttercup orange == */
                    return 0xf39c12;
                }
                /* == Shakespeare blue for duplicate == */
                return 0x52b3d9;
            }

            /* === Getter(s) === */

            inline sched::Type type() const final { return type_; }

            inline std::string name() const final {
                if (type_ == Type::MERGE) {
                    return "merge";
                } else if (type_ == Type::FORK) {
                    return "fork";
                }
                return "duplicate";
            }

            /* === Setter(s) === */

        private:
            Type type_;

            /* === Private method(s) === */

            void reduceForkDuplicate();

            u32 getKernelIx() const final;

            spider::unique_ptr<i64> buildInputParams() const final;

            spider::unique_ptr<i64> buildMergeParams() const;

            spider::unique_ptr<i64> buildForkParams() const;

            spider::unique_ptr<i64> buildDuplicateParams() const;

        };
    }
}
#endif //SPIDER2_SPECIALSCHEDVERTEX_H
