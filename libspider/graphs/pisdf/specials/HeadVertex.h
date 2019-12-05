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
#ifndef SPIDER2_HEADVERTEX_H
#define SPIDER2_HEADVERTEX_H

/* === Include(s) === */

#include <graphs/pisdf/specials/VertexInterface.h>
#include <graphs/pisdf/Graph.h>

namespace spider {
    namespace pisdf {

        inline void head(const int64_t *paramsIn, int64_t *[], void *in[], void *out[]) {
            const auto &inputEnd = paramsIn[0]; /* = Number of input to consider = */
            size_t offset = 0;
            for (int64_t i = 0; i < inputEnd; ++i) {
                /* == Size to copy for current input == */
                const auto &inputSize = static_cast<size_t>(paramsIn[i + 1]);
                auto *output = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(out[i]) + offset);
                std::memcpy(output, in[i], inputSize);
                offset += inputSize;
            }
        }

        /* === Class definition === */

        class HeadVertex final : public VertexInterface<HeadVertex> {
        public:
            explicit HeadVertex(std::string name = "unnamed-headvertex",
                                uint32_t edgeINCount = 0,
                                StackID stack = StackID::PISDF) : VertexInterface<HeadVertex>(std::move(name),
                                                                                              edgeINCount,
                                                                                              1,
                                                                                              stack) { }

            /* === Method(s) === */

            /* === Getter(s) === */

            inline VertexType subtype() const override {
                return VertexType::HEAD;
            }
        };
    }
}
#endif //SPIDER2_HEADVERTEX_H
