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
#ifndef SPIDER2_VERTEXPOOL_H
#define SPIDER2_VERTEXPOOL_H

/* === Include(s) === */

#include <cstdint>
#include <common/Exception.h>
#include <memory/Allocator.h>
#include <graphs-tools/transformation/srdag/AbstractVertexPool.h>
#include <graphs/pisdf/specials/Specials.h>

namespace spider {
    namespace srdag {

        /* === Class definition === */

        template<class T>
        class VertexPool final : public AbstractVertexPool {
        public:

            explicit VertexPool(std::uint64_t size) : size_{ size } {
                buffer_ = spider::allocate<T>(StackID::TRANSFO, size);
            }

            ~VertexPool() override = default;

            /* === Method(s) === */

            inline T *get() {
                if (count_ >= size_) {
                    throwSpiderException("pool is out of pre-allocated elements.");
                }
                return buffer_ + count_++;
            }

            /* === Getter(s) === */

            /* === Setter(s) === */

        private:
            T *buffer_ = nullptr;
            std::uint64_t size_ = 0;
            std::uint64_t count_ = 0;
        };
    }
}
#endif //SPIDER2_VERTEXPOOL_H
