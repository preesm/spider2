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
#ifndef SPIDER2_EDGE_H
#define SPIDER2_EDGE_H

/* === Include(s) === */

#include <cstdint>
#include <string>
#include <functional>
#include <memory/memory.h>
#include <memory/unique_ptr.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/abstract/AbstractEdge.h>
#include <graphs-tools/expression-parser/Expression.h>

namespace spider {
    namespace pisdf {

        /* === Forward declaration(s) === */

        class Vertex;

        /* === Class definition === */

        class Edge : public AbstractEdge<pisdf::Vertex, pisdf::Edge> {
        public:

            Edge(Vertex *source, size_t srcIx, Expression srcExpr,
                 Vertex *sink, size_t snkIx, Expression snkExpr);

            ~Edge() noexcept override = default;

            /* === Method(s) === */

            /* === Getter(s) === */

            /**
             * @brief Get the containing @refitem spider::pisdf::Graph of the edge.
             * @return containing @refitem spider::pisdf::Graph
             */
            Graph *graph() const;

            /**
             * @brief Get the delay (if any) associated to the edge.
             * @return @refitem PiSDFDelay of the edge.
             */
            Delay *delay() const;

            /* === Setter(s) === */

            /**
             * @brief
             * @param delay
             */
            void setDelay(Delay *delay);

        private:
            unique_ptr<Delay> delay_; /* = Pointer to Delay associated to the Edge (if any) = */
        };
    }
}
#endif //SPIDER2_GRAPH_H
