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
#ifndef SPIDER2_INTERFACE_H
#define SPIDER2_INTERFACE_H

/* === Include(s) === */

#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Edge.h>

namespace Spider {
    namespace PiSDF {

        /* === Class definition === */

        class Interface : public Vertex {
        public:

            explicit Interface(std::string name = "unnamed-interface",
                               std::uint32_t edgeINCount = 0,
                               std::uint32_t edgeOUTCount = 0,
                               Graph *graph = nullptr,
                               StackID stack = StackID::PISDF) : Vertex(std::move(name),
                                                                        VertexType::INTERFACE,
                                                                        edgeINCount,
                                                                        edgeOUTCount,
                                                                        graph,
                                                                        stack) {
                if (!graph_) {
                    throwSpiderException("Interface [%s] need to belong to a graph.", this->name().c_str());
                }
            }

            /* === Method(s) === */

            inline Vertex *forwardEdge(const Edge *) override;

            inline Vertex *clone(StackID, Graph *) const override;

            inline void visit(Visitor *visitor) override;

            /* === Getter(s) === */

            virtual Edge *inputEdge() const = 0;

            virtual Edge *outputEdge() const = 0;

            /**
             * @brief Return vertex connected to interface.
             * @remark return source vertex for output IF, sink vertex for input IF
             * @warning no check is performed on validity of connected edge.
             * @return opposite vertex
             */
            virtual Vertex *opposite() const = 0;

            /* === Setter(s) === */

        };

        /* === Inline method(s) === */

        Vertex *Interface::forwardEdge(const Edge *) {
            return this->opposite()->forwardEdge(nullptr);
        }

        Vertex *Interface::clone(StackID, Graph *) const {
            throwSpiderException("can not clone interfaces");
        }

        void Interface::visit(Visitor *visitor) {
            visitor->visit(this);
        }
    }
}
#endif //SPIDER2_GRAPH_H
