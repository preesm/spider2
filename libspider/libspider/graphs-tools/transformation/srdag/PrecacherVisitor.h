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
#ifndef SPIDER2_PRECACHERVISITOR_H
#define SPIDER2_PRECACHERVISITOR_H

/* === Include(s) === */

#include <graphs/pisdf/visitors/DefaultVisitor.h>
#include <graphs-tools/transformation/srdag/Precacher.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/ExecVertex.h>

namespace spider {
    namespace srdag {

        /* === Struct definition === */

        struct PrecacherVisitor final : pisdf::DefaultVisitor {
        public:
            explicit PrecacherVisitor(pisdf::Graph *graph, Precacher *precacher) : srdag_{ graph },
                                                                                   precacher_{ precacher } { }

            inline void visit(pisdf::Graph *graph) override {
                auto *clone = precacher_->make<pisdf::ExecVertex, pisdf::VertexType::NORMAL>(graph->name(),
                                                                                             graph->inputEdgeCount(),
                                                                                             graph->outputEdgeCount(),
                                                                                             StackID::TRANSFO);
                srdag_->addVertex(clone);
            }

            inline void visit(pisdf::ExecVertex *vertex) override {
                auto *clone = precacher_->make<pisdf::ExecVertex, pisdf::VertexType::NORMAL>(vertex->name(),
                                                                                             vertex->inputEdgeCount(),
                                                                                             vertex->outputEdgeCount(),
                                                                                             StackID::TRANSFO);
                setRef(vertex, clone);
            }

            inline void visit(pisdf::ForkVertex *vertex) override {
                auto *clone = precacher_->make<pisdf::ForkVertex, pisdf::VertexType::FORK>(vertex->name(),
                                                                                           vertex->outputEdgeCount(),
                                                                                           StackID::TRANSFO);
                setRef(vertex, clone);
            }

            inline void visit(pisdf::JoinVertex *vertex) override {
                auto *clone = precacher_->make<pisdf::JoinVertex, pisdf::VertexType::JOIN>(vertex->name(),
                                                                                           vertex->inputEdgeCount(),
                                                                                           StackID::TRANSFO);
                setRef(vertex, clone);
            }

            inline void visit(pisdf::HeadVertex *vertex) override {
                auto *clone = precacher_->make<pisdf::HeadVertex, pisdf::VertexType::HEAD>(vertex->name(),
                                                                                           vertex->inputEdgeCount(),
                                                                                           StackID::TRANSFO);
                setRef(vertex, clone);
            }

            inline void visit(pisdf::TailVertex *vertex) override {
                auto *clone = precacher_->make<pisdf::TailVertex, pisdf::VertexType::TAIL>(vertex->name(),
                                                                                           vertex->inputEdgeCount(),
                                                                                           StackID::TRANSFO);
                setRef(vertex, clone);
            }

            inline void visit(pisdf::DuplicateVertex *vertex) override {
                auto *clone = precacher_->make<pisdf::DuplicateVertex, pisdf::VertexType::DUPLICATE>(vertex->name(),
                                                                                                     vertex->outputEdgeCount(),
                                                                                                     StackID::TRANSFO);
                setRef(vertex, clone);
            }

            inline void visit(pisdf::RepeatVertex *vertex) override {
                auto *clone = precacher_->make<pisdf::RepeatVertex, pisdf::VertexType::REPEAT>(vertex->name(),
                                                                                               StackID::TRANSFO);
                setRef(vertex, clone);
            }

            inline void visit(pisdf::InitVertex *vertex) override {
                auto *clone = precacher_->make<pisdf::InitVertex, pisdf::VertexType::INIT>(vertex->name(),
                                                                                           StackID::TRANSFO);
                setRef(vertex, clone);
            }

            inline void visit(pisdf::EndVertex *vertex) override {
                auto *clone = precacher_->make<pisdf::EndVertex, pisdf::VertexType::END>(vertex->name(),
                                                                                         StackID::TRANSFO);
                setRef(vertex, clone);
            }

            inline void visit(pisdf::InputInterface *interface) override {
                auto *clone = precacher_->make<pisdf::RepeatVertex, pisdf::VertexType::REPEAT>(interface->name(),
                                                                                               StackID::TRANSFO);
                srdag_->addVertex(clone);
            }

            inline void visit(pisdf::OutputInterface *interface) override {
                auto *clone = precacher_->make<pisdf::TailVertex, pisdf::VertexType::TAIL>(interface->name(),
                                                                                           1,
                                                                                           StackID::TRANSFO);
                srdag_->addVertex(clone);
            }

            pisdf::Graph *srdag_ = nullptr;
            Precacher *precacher_ = nullptr;

        private:
            inline void setRef(pisdf::Vertex *vertex, pisdf::Vertex *clone) {
                srdag_->addVertex(clone);
                clone->reference_ = vertex;
                vertex->copyCount_ += 1;
            }
        };

        /* === Inline method(s) === */
    }
}
#endif //SPIDER2_PRECACHERVISITOR_H
