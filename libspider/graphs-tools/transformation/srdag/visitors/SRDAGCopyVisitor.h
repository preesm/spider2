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
#ifndef SPIDER2_SRDAGCOPYVISITOR_H
#define SPIDER2_SRDAGCOPYVISITOR_H

/* === Include(s) === */

#include <graphs/pisdf/visitors/PiSDFDefaultVisitor.h>
#include <graphs/pisdf/visitors/PiSDFCloneVisitor.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <api/pisdf-api.h>

namespace spider {
    namespace srdag {

        /* === Struct definition === */

        struct SRDAGCopyVisitor final : public pisdf::DefaultVisitor {
        public:
            SRDAGCopyVisitor(const TransfoJob &job, pisdf::Graph *srdag) : job_{ job }, srdag_{ srdag } { };

            ~SRDAGCopyVisitor() override = default;

            inline void visit(pisdf::DelayVertex *vertex) override {
                /* == This a trick to ensure proper coherence even with recursive delay init == */
                /* == For given scenario:   A -> | delay | -> B
                 *                         setter --^ --> getter
                 *    This will produce this:
                 *                          setter -> | delay | -> getter
                 *                               A -> |       | -> B
                 *    But in reality the vertex does not make it after the SR-Transformation.
                 */
                api::createVertex(srdag_, buildCloneName(vertex, 0), 2, 2);
                ix_ = srdag_->vertexCount() - 1;
            }

            inline void visit(pisdf::ExecVertex *vertex) override { clone(vertex); }

            inline void visit(pisdf::ConfigVertex *vertex) override { clone(vertex); }

            inline void visit(pisdf::ForkVertex *vertex) override { clone(vertex); }

            inline void visit(pisdf::JoinVertex *vertex) override { clone(vertex); }

            inline void visit(pisdf::HeadVertex *vertex) override { clone(vertex); }

            inline void visit(pisdf::TailVertex *vertex) override { clone(vertex); }

            inline void visit(pisdf::DuplicateVertex *vertex) override { clone(vertex); }

            inline void visit(pisdf::RepeatVertex *vertex) override { clone(vertex); }

            inline void visit(pisdf::InitVertex *vertex) override { clone(vertex); }

            inline void visit(pisdf::EndVertex *vertex) override { clone(vertex); }

            inline void visit(pisdf::Graph *graph) override {
                /* == Clone the vertex == */
                ix_ = 0;
                for (uint32_t it = 0; it < graph->repetitionValue(); ++it) {
                    auto *clone = api::createNonExecVertex(srdag_,
                                                           buildCloneName(graph, it),
                                                           static_cast<uint32_t>(graph->inputEdgeCount()),
                                                           static_cast<uint32_t>(graph->outputEdgeCount()));
                    ix_ = clone->ix();
                }
                ix_ = ix_ - (graph->repetitionValue() - 1);
            }

            const TransfoJob &job_;
            pisdf::Graph *srdag_ = nullptr;
            size_t ix_ = SIZE_MAX;
        private:
            std::string buildCloneName(const pisdf::Vertex *vertex, uint32_t firing) {
                const auto *graphRef = job_.firingValue_ == UINT32_MAX ?
                                       job_.reference_ : srdag_->vertex(*(job_.srdagIx_));
                return graphRef->name() + ":" + vertex->name() + "-" + std::to_string(firing);
            }

            template<class T>
            inline void clone(const T *vertex) {
                for (uint32_t it = 0; it < vertex->repetitionValue(); ++it) {
                    auto *clone = make<T>(StackID::PISDF, (*vertex));
                    srdag_->addVertex(clone);
                    /* == Change the name of the clone == */
                    clone->setName(buildCloneName(vertex, it));
                    clone->setInstanceValue(it);

                    /* == Get the cloned parameters == */
                    for (const auto &param : vertex->inputParamVector()) {
                        clone->addInputParameter(job_.params_[param->ix()]);
                    }
                    for (const auto &param : vertex->refinementParamVector()) {
                        clone->addRefinementParameter(job_.params_[param->ix()]);
                    }
                    for (const auto &param : vertex->outputParamVector()) {
                        clone->addOutputParameter(job_.params_[param->ix()]);
                    }
                }
                ix_ = (srdag_->vertexCount() - 1) - (vertex->repetitionValue() - 1);
            }
        };
    }
}

#endif //SPIDER2_SRDAGCOPYVISITOR_H
