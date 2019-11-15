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
#ifndef SPIDER2_CLONEVERTEXVISITOR_H
#define SPIDER2_CLONEVERTEXVISITOR_H

/* === Include(s) === */

#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/visitors/DefaultVisitor.h>

namespace Spider {
    namespace PiSDF {

        /* === Class definition === */

        /**
         * @brief Clone the vertex. In the case of Graph, the clone is shallow.
         * @param stack Stack on which to clone the vertex.
         * @param Graph Graph to which the clone is added (if nullptr, @refitem graph_ is used).
         * @return Clone instance of the vertex.
         */
        struct CloneVertexVisitor : public DefaultVisitor {
            explicit CloneVertexVisitor(Graph *graph, StackID stack = StackID::PISDF) : graph_{ graph },
                                                                                        stack_{ stack } { }

            /* === Method(s) === */

            inline void visit(Graph *graph) override {
                auto *clone = Spider::API::createSubraph(graph_,
                                                         graph->name_,
                                                         graph->vertexCount(),
                                                         graph->edgeCount(),
                                                         graph->paramCount(),
                                                         graph->edgesINCount(),
                                                         graph->edgesOUTCount(),
                                                         graph->configVertexCount(),
                                                         stack_);
                clone->dynamic_ = graph->dynamic_;
                setRef(graph, clone);
            }

            inline void visit(ExecVertex *vertex) override {
                auto *clone = Spider::API::createVertex(graph_,
                                                        vertex->name_,
                                                        vertex->edgesINCount(),
                                                        vertex->edgesOUTCount(),
                                                        stack_);
                clone->refinementIx_ = vertex->refinementIx_;
                setRef(vertex, clone);
            }

            inline void visit(ConfigVertex *vertex) override {
                auto *clone = Spider::API::createConfigActor(graph_,
                                                             vertex->name_,
                                                             vertex->edgesINCount(),
                                                             vertex->edgesOUTCount(),
                                                             stack_);
                setRef(vertex, clone);

            }

            inline void visit(ForkVertex *vertex) override {
                auto *clone = Spider::API::createFork(graph_,
                                                      vertex->name_,
                                                      vertex->edgesOUTCount(),
                                                      stack_);
                setRef(vertex, clone);
            }

            inline void visit(JoinVertex *vertex) override {
                auto *clone = Spider::API::createJoin(graph_,
                                                      vertex->name_,
                                                      vertex->edgesINCount(),
                                                      stack_);
                setRef(vertex, clone);
            }

            inline void visit(HeadVertex *vertex) override {
                auto *clone = Spider::API::createHead(graph_,
                                                      vertex->name_,
                                                      vertex->edgesINCount(),
                                                      stack_);
                setRef(vertex, clone);
            }

            inline void visit(TailVertex *vertex) override {
                auto *clone = Spider::API::createTail(graph_,
                                                      vertex->name_,
                                                      vertex->edgesINCount(),
                                                      stack_);
                setRef(vertex, clone);
            }

            inline void visit(DuplicateVertex *vertex) override {
                auto *clone = Spider::API::createDuplicate(graph_,
                                                           vertex->name_,
                                                           vertex->edgesOUTCount(),
                                                           stack_);
                setRef(vertex, clone);
            }

            inline void visit(RepeatVertex *vertex) override {
                auto *clone = Spider::API::createRepeat(graph_,
                                                        vertex->name_,
                                                        stack_);
                setRef(vertex, clone);
            }

            inline void visit(InitVertex *vertex) override {
                auto *clone = Spider::API::createInit(graph_,
                                                      vertex->name_,
                                                      stack_);
                setRef(vertex, clone);
            }

            inline void visit(EndVertex *vertex) override {
                auto *clone = Spider::API::createEnd(graph_,
                                                     vertex->name_,
                                                     stack_);
                setRef(vertex, clone);
            }

            /* == Graph to add vertex to == */
            Graph *graph_ = nullptr;
            StackID stack_;

        private:
            inline void setRef(Vertex *vertex, Vertex *clone) {
                clone->reference_ = vertex;
                vertex->copyCount_ += 1;
            }

        };
    }
}
#endif //SPIDER2_CLONEVERTEXVISITOR_H