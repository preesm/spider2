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
#ifndef SPIDER2_SINKTRANSFOVECTOR_H
#define SPIDER2_SINKTRANSFOVECTOR_H

/* === Include(s) === */

#include <graphs/pisdf/visitors/DefaultVisitor.h>
#include <graphs-tools/transformation/srdag/Helper.h>
#include <graphs/pisdf/Delay.h>

/* === Structure definition === */

struct SnkTransfoVectorVisitor final : public spider::pisdf::DefaultVisitor {
    explicit SnkTransfoVectorVisitor(spider::srdag::TransfoData &transfoData) : transfoData_{ transfoData } {
        /* == Reserve size of the vector == */
        auto *sink = transfoData_.edge_->sink();
        auto *delay = transfoData_.edge_->delay();
        sinkVector_.reserve(sink->repetitionValue() + (delay != nullptr));

        /* == First, if delay, populate the getter clones in reverse order == */
        handleDelay(transfoData_.edge_->delay());

    }

    void visit(spider::pisdf::DelayVertex *sink) override {
        auto *clone = transfoData_.srdag_->vertex(transfoData_.tracker_[sink->ix()]);
        auto *outputEdge = clone->outputEdge(1);
        if (outputEdge) {
            /* == We already connected sink of original edge containing the delay, we'll use it directly == */
            populateFromDelayVertex(outputEdge, clone);
        } else {
            /* == Populate the sink clones in reverse order == */
            populateSinkVector(transfoData_.edge_, sink);
        }
    }

    void visit(spider::pisdf::ExecVertex *sink) override {
        /* == Populate the sink clones in reverse order == */
        populateSinkVector(transfoData_.edge_, sink);
    }

    stack_vector(sinkVector_, spider::srdag::TransfoVertex, StackID::TRANSFO);
private:
    spider::srdag::TransfoData &transfoData_;

    /**
     * @brief Check if there is a delay and if so, get the corresponding delay vertex or if possible call
     *        populateFromDelayVertex.
     * @param delay  Delay of the edge.
     */
    inline void handleDelay(const spider::pisdf::Delay *delay) {
        const auto &params = transfoData_.job_.params_;
        auto *sink = transfoData_.edge_->sink();
        if (delay) {
            if ((sink == transfoData_.edge_->source()) &&
                delay->value(params) < transfoData_.edge_->sinkRateExpression().evaluate(params)) {
                throwSpiderException("Insufficient delay [%"
                                             PRIu32
                                             "] on edge [%s].",
                                     delay->value(params),
                                     transfoData_.edge_->name().c_str());
            }
            const auto *delayVertex = delay->vertex();
            auto *clone = transfoData_.srdag_->vertex(transfoData_.tracker_[delayVertex->ix()]);
            auto *outputEdge = clone->outputEdge(0);
            if (outputEdge) {
                /* == We already connected getter, we'll use it directly == */
                populateFromDelayVertex(outputEdge, clone);
            } else {
                populateTransfoStack(sinkVector_, delay->vertex(), delay->value(params), 1, transfoData_);
            }
        }
    }

    /**
     * @brief Populate sink vector from delay vertex.
     *        If corresponding edge exist, removes it and connect directly to the corresponding target, otherwise
     *        connect to clone.
     *        If last of the four, removes the clone.
     * @param edge   Corresponding edge in the four possible (in_0: setter, in_1: producer, out_0:getter, out_1:consumer)
     * @param clone  Clone of the delay vertex (see the trick in Helper.cpp::CopyVisitor)
     */
    inline void populateFromDelayVertex(PiSDFEdge *edge, PiSDFAbstractVertex *clone) {
        const auto &sinkDelay = edge->sink();
        const auto &outputRate = edge->sourceRateExpression().evaluate(transfoData_.job_.params_);
        const auto &sinkPortIxDelay = edge->sinkPortIx();
        sinkVector_.emplace_back(outputRate, sinkPortIxDelay, sinkDelay);

        /* == Remove the Edge == */
        transfoData_.srdag_->removeEdge(edge);

        /* == Check if we are the last of the four (setter / getter, producer / consumer) == */
        if (!clone->outputEdge(0) && !clone->outputEdge(1) &&
            !clone->inputEdge(0) && !clone->inputEdge(1)) {
            transfoData_.srdag_->removeVertex(clone);
        }
    }

    /**
     * @brief Populate sink vector with corresponding instances of sink vertex.
     * @param edge  Evaluated edge.
     * @param sink  Sink of the edge.
     */
    inline void populateSinkVector(const PiSDFEdge *edge, PiSDFAbstractVertex *sink) {
        const auto &params = transfoData_.job_.params_;
        const auto &rate =
                sink->subtype() == PiSDFVertexType::OUTPUT ? edge->sourceRateExpression().evaluate(params) *
                                                             edge->source()->repetitionValue()
                                                           : edge->sinkRateExpression().evaluate(params);
        populateTransfoStack(sinkVector_, sink, rate, edge->sinkPortIx(), transfoData_);
    }
};

#endif //SPIDER2_SINKTRANSFOVECTOR_H
