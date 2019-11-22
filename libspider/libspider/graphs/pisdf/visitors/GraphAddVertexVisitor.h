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
#ifndef SPIDER2_GRAPHADDVERTEXVISITOR_H
#define SPIDER2_GRAPHADDVERTEXVISITOR_H

/* === Include(s) === */

#include <graphs/pisdf/visitors/DefaultVisitor.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/Graph.h>
#include <scenario/Scenario.h>
#include <spider-api/archi.h>
#include <archi/Platform.h>

namespace spider {
    namespace pisdf {
        struct GraphAddVertexVisitor final : public DefaultVisitor {

            explicit GraphAddVertexVisitor(Graph *graph) : graph_{ graph } { }

            /* === Method(s) === */

            inline void visit(ExecVertex *vertex) override {
                /* == Add vertex to vertexVector_ == */
                addVertex(vertex);

                /* == Create default constraints == */
                addScenarioConstraint();
            }

            inline void visit(ConfigVertex *vertex) override {
                /* == Add vertex to vertexVector_ == */
                addVertex(vertex);

                /* == Add config vertex to the "viewer" vector == */
                graph_->configVertexVector_.emplace_back(vertex);

                /* == Create default constraints == */
                addScenarioConstraint();
            }

            inline void visit(Graph *subgraph) override {
                /* == Add the subgraph as Vertex == */
                addVertex(subgraph);

                /* == Add the subgraph in the "viewer" vector == */
                subgraph->subIx_ = static_cast<uint32_t>(graph_->subgraphVector_.size());
                graph_->subgraphVector_.emplace_back(subgraph);
            }

            /* == Graph to add vertex to == */
            Graph *graph_ = nullptr;
        private:
            template<class T>
            void addVertex(T *vertex) {
                vertex->setIx(static_cast<uint32_t>(graph_->vertexVector_.size()));
                graph_->vertexVector_.emplace_back(vertex);
                vertex->setGraph(graph_);
            }

            void addScenarioConstraint() {
                if (graph_->scenario_) {
                    const auto &peCount = spider::platform()->PECount();
                    graph_->scenario_->mappingConstraintsVector_.emplace_back(std::vector<bool>(peCount, true));
                    graph_->scenario_->executionTimingsVector_.emplace_back(
                            spider::vector<Expression>(peCount, Expression(100)));
                }
            }
        };
    }
}


#endif //SPIDER2_GRAPHADDVERTEXVISITOR_H
