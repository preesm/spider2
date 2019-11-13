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
#ifndef SPIDER2_EXECVERTEX_H
#define SPIDER2_EXECVERTEX_H

/* === Include(s) === */

#include <cstdint>
#include <string>
#include <containers/Array.h>
#include <spider-api/config.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Refinement.h>

namespace Spider {
    namespace PiSDF {

        /* === Class definition === */

        class ExecVertex : public Vertex {
        public:
            explicit ExecVertex(std::string name = "unnamed-execvertex",
                                VertexType type = VertexType::NORMAL,
                                std::uint32_t edgeINCount = 0,
                                std::uint32_t edgeOUTCount = 0,
                                Graph *graph = nullptr, //TODO: change to Spider::pisdfgraph() when this API replace old one
                                StackID stack = StackID::PISDF);

            ExecVertex(const ExecVertex &) = delete;

            /* === Getter(s) === */

            Vertex *clone(StackID stack, Graph *graph) const override;

            inline bool executable() const override;

            inline std::uint32_t refinementIx() const;

            /* === Setter(s) === */

            inline void setRefinementIx(std::uint32_t ix);

        protected:
            std::uint32_t refinementIx_ = UINT32_MAX;
        };

        bool ExecVertex::executable() const {
            return true;
        }

        std::uint32_t ExecVertex::refinementIx() const {
            return refinementIx_;
        }

        void Spider::PiSDF::ExecVertex::setRefinementIx(std::uint32_t ix) {
            refinementIx_ = ix;
        }
    }
}
#endif //SPIDER2_EXECVERTEX_H
