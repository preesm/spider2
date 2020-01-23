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
#include <graphs/pisdf/Vertex.h>

namespace spider {

    namespace pisdf {

        /* === Class definition === */

        class ExecVertex : public Vertex {
        public:
            explicit ExecVertex(std::string name = "unnamed-execvertex",
                                size_t edgeINCount = 0,
                                size_t edgeOUTCount = 0) : Vertex(std::move(name), edgeINCount, edgeOUTCount) {
                rtInformation_ = spider::make_shared<RTInfo>(StackID::RUNTIME);
            };

            ExecVertex(std::string name,
                       size_t edgeINCount,
                       size_t edgeOUTCount,
                       size_t paramINCount,
                       size_t paramOUTCount,
                       const ExecVertex *reference) : Vertex(std::move(name),
                                                             edgeINCount,
                                                             edgeOUTCount,
                                                             paramINCount,
                                                             paramOUTCount, reference) { };

            ExecVertex(const ExecVertex &) = default;

            ExecVertex(ExecVertex &&other) noexcept : Vertex(std::move(other)) { };

            ~ExecVertex() override = default;

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override { visitor->visit(this); };

            /* === Getter(s) === */

            /**
             * @brief Ensure that any vertex inheriting from ExecVertex will not be able to override this method.
             * @return false.
             */
            inline bool executable() const final { return true; };

            /**
             * @brief Ensure that any vertex inheriting from ExecVertex will not be able to override this method.
             * @return false.
             */
            inline bool hierarchical() const final { return false; };
        };
    }
}
#endif //SPIDER2_EXECVERTEX_H
