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
#ifndef SPIDER2_TAILVERTEX_H
#define SPIDER2_TAILVERTEX_H

/* === Include(s) === */

#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/Graph.h>

namespace Spider {
    namespace PiSDF {

        inline void tail(const std::int64_t *paramsIn, std::int64_t *[], void *in[], void *out[]) {
            const auto &inputCount = paramsIn[0];  /* = Number of input = */
            const auto &inputStart = paramsIn[1];  /* = First input to be considered = */
            const auto &inputOffset = paramsIn[2]; /* = Offset in the first buffer if any = */

            /* == Copy the first input with the offset == */
            std::memcpy(reinterpret_cast<char *>(out[0]),
                        reinterpret_cast<char *>(in[inputStart]) + inputOffset,
                        paramsIn[3]);

            /* == Do the general case == */
            std::int64_t offset = inputOffset + paramsIn[3];
            for (auto i = inputStart + 1; i < inputCount; ++i) {
                const auto &inputSize = paramsIn[i + 4];
                std::memcpy(reinterpret_cast<char *>(out[0]) + offset, in[i], inputSize);
                offset += inputSize;
            }
        }

        /* === Class definition === */

        class TailVertex final : public ExecVertex {
        public:
            explicit TailVertex(std::string name = "unnamed-tailvertex",
                                std::uint32_t edgeINCount = 0,
                                Graph *graph = nullptr, //TODO: change to Spider::pisdfgraph() when this API replace old one
                                StackID stack = StackID::PISDF) : ExecVertex(std::move(name),
                                                                             VertexType::SPECIAL,
                                                                             edgeINCount,
                                                                             1,
                                                                             graph,
                                                                             stack) {
//                refinement_ = tail;
            }

            /* === Method(s) === */

            inline Vertex *clone(StackID stack, Graph *graph) const override;

            /* === Getter(s) === */

            inline VertexType subtype() const override;

            /* === Setter(s) === */

        private:

            //TODO add function call

            /* === Private method(s) === */
        };

        VertexType TailVertex::subtype() const {
            return VertexType::TAIL;
        }

        Vertex *TailVertex::clone(StackID stack, Graph *graph) const {
            graph = graph ? graph : this->graph_;
            auto *result = Spider::API::createTail(graph,
                                                   "cpy-" + graph->name() + "-" + this->name_,
                                                   this->edgesINCount(),
                                                   stack);
            result->reference_ = this;
            this->copyCount_ += 1;
            return result;
        }

        /* === Inline method(s) === */
    }
}
#endif //SPIDER2_TAILVERTEX_H
