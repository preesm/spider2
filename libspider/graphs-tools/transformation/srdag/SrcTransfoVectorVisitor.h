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
#ifndef SPIDER2_SRCTRANSFOVECTORVISITOR_H
#define SPIDER2_SRCTRANSFOVECTORVISITOR_H

/* === Include(s) === */

#include <graphs/pisdf/visitors/DefaultVisitor.h>
#include <graphs-tools/transformation/srdag/Helper.h>
#include <graphs/pisdf/Delay.h>

/* === Structure definition === */

struct SrcTransfoVectorVisitor final : public spider::pisdf::DefaultVisitor {
    explicit SrcTransfoVectorVisitor(spider::srdag::TransfoData &transfoData) : transfoData_{ transfoData } {
        /* == Reserve size of the vector == */
        auto *delay = transfoData_.edge_->delay();
        auto *source = transfoData_.edge_->source();
        sourceVector_.reserve(source->repetitionValue() + (delay != nullptr));
    }

    inline void visit(spider::pisdf::DelayVertex *source) override {
        auto *clone = transfoData_.srdag_->vertex(transfoData_.tracker_[source->ix()]);
        auto *inputEdge = clone->inputEdge(1);
        if (inputEdge) {
            /* == We already connected source of original edge containing the delay, we'll use it directly == */
            populateFromDelayVertex(inputEdge, clone);
        } else {
            /* == Populate the sink clones in reverse order == */
            populateSourceVector(transfoData_.edge_, source);
        }

        /* == Handle delay (if any) == */
        handleDelay(transfoData_.edge_->delay());
    }

    inline void visit(spider::pisdf::ExecVertex *sink) override {
        /* == Populate the source clones in reverse order == */
        populateSourceVector(transfoData_.edge_, sink);

        /* == Handle delay (if any) == */
        handleDelay(transfoData_.edge_->delay());
    }

    stack_vector(sourceVector_, spider::srdag::TransfoVertex, StackID::TRANSFO);
private:
    spider::srdag::TransfoData &transfoData_;

    /**
     * @brief Check if there is a delay and if so, get the corresponding delay vertex or if possible call
     *        populateFromDelayVertex.
     * @param delay  Delay of the edge.
     */
    inline void handleDelay(const spider::pisdf::Delay *delay) {
        /* == If delay, populate the setter clones in reverse order == */
        const auto &params = transfoData_.job_.params_;
        if (delay) {
            const auto *delayVertex = delay->vertex();
            auto *clone = transfoData_.srdag_->vertex(transfoData_.tracker_[delayVertex->ix()]);
            auto *inputEdge = clone->inputEdge(0);
            if (inputEdge) {
                /* == We already connected setter, we'll use it directly == */
                populateFromDelayVertex(inputEdge, clone);
            } else {
                populateTransfoStack(sourceVector_, delay->vertex(), delay->value(params), 1, transfoData_);
            }
        }
    }

    /**
     * @brief Populate source vector from delay vertex.
     *        If corresponding edge exist, removes it and connect directly to the corresponding target, otherwise
     *        connect to clone.
     *        If last of the four, removes the clone.
     * @param edge   Corresponding edge in the four possible (in_0: setter, in_1: producer, out_0:getter, out_1:consumer)
     * @param clone  Clone of the delay vertex (see the trick in Helper.cpp::CopyVisitor)
     */
    inline void populateFromDelayVertex(PiSDFEdge *edge, PiSDFAbstractVertex *clone) {
        const auto &sourceDelay = edge->source();
        const auto &inputRate = edge->sinkRateExpression().evaluate(transfoData_.job_.params_);
        const auto &sourcePortIxDelay = edge->sourcePortIx();
        sourceVector_.emplace_back(inputRate, sourcePortIxDelay, sourceDelay);
        /* == Remove the Edge == */
        transfoData_.srdag_->removeEdge(edge);
        /* == Check if we are the last of the four (setter / getter, producer / consumer) == */
        if (!clone->outputEdge(0) && !clone->outputEdge(1) &&
            !clone->inputEdge(0) && !clone->inputEdge(1)) {
            transfoData_.srdag_->removeVertex(clone);
        }
    }

    /**
     * @brief Populate source vector with corresponding instances of sink vertex.
     * @param edge   Evaluated edge.
     * @param source Source of the edge.
     */
    inline void populateSourceVector(const PiSDFEdge *edge, PiSDFAbstractVertex *source) {
        const auto &params = transfoData_.job_.params_;
        const auto &rate = source->subtype() == PiSDFVertexType::INPUT ? edge->sinkRateExpression().evaluate(params) *
                                                                         edge->sink()->repetitionValue()
                                                                       : edge->sourceRateExpression().evaluate(params);
        populateTransfoStack(sourceVector_, source, rate, edge->sourcePortIx(), transfoData_);
    }
};

#endif //SPIDER2_SRCTRANSFOVECTORVISITOR_H
