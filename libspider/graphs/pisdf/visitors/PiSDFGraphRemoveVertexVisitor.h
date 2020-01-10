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
#ifndef SPIDER2_GRAPHVISITORS_H
#define SPIDER2_GRAPHVISITORS_H

/* === Include(s) === */

#include <graphs/pisdf/visitors/PiSDFDefaultVisitor.h>
#include <graphs/pisdf/Graph.h>

namespace spider {
    namespace pisdf {

        struct GraphRemoveVertexVisitor final : public DefaultVisitor {

            explicit GraphRemoveVertexVisitor(Graph *graph) : graph_{ graph } { }

            /* === Method(s) === */

            inline void visit(Graph *subgraph) override {
                /* == Remove the vertex and destroy it == */
                auto ix = subgraph->subIx_; /* = Save the index in the subgraphVector_ = */

                /* == Remove the subgraph from the subgraph vector == */
                graph_->subgraphVector_[ix] = graph_->subgraphVector_.back();
                graph_->subgraphVector_[ix]->subIx_ = ix;
                graph_->subgraphVector_.pop_back();

                destroyVertex(subgraph);
            }

            inline void visit(ExecVertex *vertex) override {
                /* == Remove the vertex and destroy it == */
                destroyVertex(vertex);
            }

            inline void visit(NonExecVertex *vertex) override {
                /* == Remove the vertex and destroy it == */
                destroyVertex(vertex);
            }

            inline void visit(ConfigVertex *vertex) override {
                /* == configVertexVector_ is just a "viewer" for config vertices so we need to find manually == */
                for (auto &cfg : graph_->configVertexVector_) {
                    if (cfg == vertex) {
                        cfg = graph_->configVertexVector_.back();
                        graph_->configVertexVector_.pop_back();
                        break;
                    }
                }

                /* == Remove the vertex and destroy it == */
                destroyVertex(vertex);
            }

            inline void visit(DelayVertex *vertex) override {
                /* == Remove the vertex and destroy it == */
                destroyVertex(vertex);
            }

            inline void visit(ForkVertex *vertex) override {
                /* == Remove the vertex and destroy it == */
                destroyVertex(vertex);
            }

            inline void visit(JoinVertex *vertex) override {
                /* == Remove the vertex and destroy it == */
                destroyVertex(vertex);
            }

            inline void visit(HeadVertex *vertex) override {
                /* == Remove the vertex and destroy it == */
                destroyVertex(vertex);
            }

            inline void visit(TailVertex *vertex) override {
                /* == Remove the vertex and destroy it == */
                destroyVertex(vertex);
            }

            inline void visit(DuplicateVertex *vertex) override {
                /* == Remove the vertex and destroy it == */
                destroyVertex(vertex);
            }

            inline void visit(RepeatVertex *vertex) override {
                /* == Remove the vertex and destroy it == */
                destroyVertex(vertex);
            }

            inline void visit(InitVertex *vertex) override {
                /* == Remove the vertex and destroy it == */
                destroyVertex(vertex);
            }

            inline void visit(EndVertex *vertex) override {
                /* == Remove the vertex and destroy it == */
                destroyVertex(vertex);
            }

            /* == Graph to add vertex to == */
            Graph *graph_ = nullptr;

        private:
            template<class T>
            inline void destroyVertex(T *vertex) {
                graph_->removeElement(graph_->vertexVector_, static_cast<Vertex *>(vertex));
                destroy(vertex);
            }
        };

    }
}

#endif //SPIDER2_GRAPHVISITORS_H
