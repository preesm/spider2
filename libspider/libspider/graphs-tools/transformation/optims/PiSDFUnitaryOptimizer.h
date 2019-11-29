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
#ifndef SPIDER2_PISDFUNITARYOPTIMIZER_H
#define SPIDER2_PISDFUNITARYOPTIMIZER_H

/* === Includes === */

#include <graphs-tools/transformation/optims/PiSDFOptimizer.h>
#include <graphs/pisdf/visitors/DefaultVisitor.h>

/* === Class definition === */

/**
 * @brief Optimize a PiSDFGraph by removing useless special actors.
 *        detail:    --> Fork      --> : removes fork with 1 output edge
 *                   --> Duplicate --> : removes duplicate with 1 input edge if rate_in == rate_out
 *                   --> Join      --> : removes join with 1 input edge
 *                   --> Tail      --> : removes tail with 1 input edge if rate_in == rate_out
 *                   --> Head      --> : removes head with 1 input edge if rate_in == rate_out
 *                   --> Repeat    --> : removes repeat if rate_in == rate_out
 */
class PiSDFUnitaryOptimizer final : public PiSDFOptimizer {
public:
    inline bool operator()(PiSDFGraph *graph) const override;

private:

    struct OptimizerVisitor final : public spider::pisdf::DefaultVisitor {

        explicit OptimizerVisitor(PiSDFGraph *graph) : graph_{ graph }, params_{ graph->params() } { }

        inline void visit(spider::pisdf::ExecVertex *) override { removed_ = false; }

        inline void visit(spider::pisdf::ForkVertex *vertex) override {
            removed_ = false;
            if (vertex->outputEdgeCount() == 1) {
                tryRemoveOutputEdge(vertex);
            }
        }

        inline void visit(spider::pisdf::JoinVertex *vertex) override {
            removed_ = false;
            if (vertex->inputEdgeCount() == 1) {
                tryRemoveOutputEdge(vertex);
            }
        }

        inline void visit(spider::pisdf::HeadVertex *vertex) override {
            removed_ = false;
            if (vertex->inputEdgeCount() == 1) {
                tryRemoveOutputEdge(vertex);
            }
        }

        inline void visit(spider::pisdf::TailVertex *vertex) override {
            removed_ = false;
            if (vertex->inputEdgeCount() == 1) {
                tryRemoveOutputEdge(vertex);
            }
        }

        inline void visit(spider::pisdf::DuplicateVertex *vertex) override {
            removed_ = false;
            if (vertex->outputEdgeCount() == 1) {
                tryRemoveOutputEdge(vertex);
            }
        }

        inline void visit(spider::pisdf::RepeatVertex *vertex) override {
            removed_ = false;
            tryRemoveOutputEdge(vertex);
        }

        PiSDFGraph *graph_ = nullptr;
        const spider::vector<spider::pisdf::Param *> &params_;
        bool removed_ = false;
    private:
        void tryRemoveOutputEdge(spider::pisdf::Vertex *vertex) {
            auto *inputEdge = vertex->inputEdge(0);
            auto *outputEdge = vertex->outputEdge(0);
            if (inputEdge->sinkRateExpression().evaluate(params_) ==
                outputEdge->sourceRateExpression().evaluate(params_)) {
                inputEdge->setSink(outputEdge->sink(),
                                   outputEdge->sinkPortIx(),
                                   spider::Expression(outputEdge->sinkRateExpression()));
                graph_->removeEdge(outputEdge);
                graph_->removeVertex(vertex);
                removed_ = true;
            }
        }
    };
};

bool PiSDFUnitaryOptimizer::operator()(PiSDFGraph *graph) const {
    bool optimized = true;

    OptimizerVisitor optimizer{ graph };
    auto it = graph->vertices().begin();
    while (it != graph->vertices().end()) {
        auto &vertex = (*it);
        vertex->visit(&optimizer);
        if (!optimizer.removed_) {
            it++;
        }
        optimized &= (!optimizer.removed_);
    }

    return optimized;
}

#endif //SPIDER2_PISDFUNITARYOPTIMIZER_H
