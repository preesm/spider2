/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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

/* === Include(s) === */

#include "LCMBRVCompute.h"
#include <graphs/pisdf/PiSDFGraph.h>
#include <graphs/pisdf/PiSDFVertex.h>
#include <graphs/pisdf/PiSDFEdge.h>
#include <graphs/pisdf/PiSDFDelay.h>
#include <cstdint>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

void LCMBRVCompute::execute() {
    /* == Initializes the Rational array == */
    Spider::Array<Spider::Rational> reps{StackID::TRANSFO, graph_->nVertices(), Spider::Rational()};

    /* == Go through all connected components == */
    for (const auto &component : connectedComponents_) {
        /* == Extract the edges == */
        Spider::Array<const PiSDFEdge *> edgeArray{StackID::TRANSFO, component.nEdges};
        BRVCompute::extractEdges(edgeArray, component);

        /* == Extract the rationals == */
        extractRationals(edgeArray, reps);

        /* == Compute the LCM factor for the current component == */
        auto lcmFactor = computeLCM(component, reps);

        /* == Compute the repetition vector values of the current component == */
        computeBRV(component, reps, lcmFactor);

        /* == Update the repetition vector values using the interfaces of the graph == */
        updateBRV(edgeArray, component);

        /* == Check validity of the obtained repetition vector == */
        checkValidity(edgeArray);
    }
}

/* === Private method(s) implementation === */

void LCMBRVCompute::extractRationals(Spider::Array<const PiSDFEdge *> &edgeArray,
                                     Spider::Array<Spider::Rational> &reps) const {
    auto dummyRational = Spider::Rational{1};
    for (const auto &edge:edgeArray) {
        const auto *source = edge->source();
        const auto *sink = edge->sink();
        std::int64_t sourceRate = edge->sourceRate();
        std::int64_t sinkRate = edge->sinkRate();

        /* == Check rates validity == */
        if ((!sinkRate && sourceRate) || (sinkRate && !sourceRate)) {
            throwSpiderException("Invalid rates on edge. Source [%s]: %"
                                         PRIu64
                                         " -- Sink [%s]: %"
                                         PRIu64
                                         ".",
                                 source->name().c_str(),
                                 sourceRate,
                                 sink->name().c_str(),
                                 sinkRate);
        }

        auto &sourceRational = source->type() == PiSDFVertexType::INTERFACE ? dummyRational : reps[source->getIx()];
        auto &sinkRational = sink->type() == PiSDFVertexType::INTERFACE ? dummyRational : reps[sink->getIx()];

        if (!sinkRational.nominator() && sinkRate) {
            sinkRational = Spider::Rational{sourceRate, sinkRate};
            if (sourceRational.nominator()) {
                sinkRational *= sourceRational;
            }
        }

        if (!sourceRational.nominator() && sourceRate) {
            sourceRational = Spider::Rational{sinkRate, sourceRate};
            if (sinkRational.nominator()) {
                sourceRational *= sinkRational;
            }
        }
    }
}

std::int64_t LCMBRVCompute::computeLCM(const BRVComponent &component, Spider::Array<Spider::Rational> &reps) {
    std::int64_t lcmFactor = 1;
    for (const auto &v : component.vertices) {
        lcmFactor = Spider::Math::lcm(lcmFactor, reps[v->getIx()].denominator());
    }
    return lcmFactor;
}

void LCMBRVCompute::computeBRV(const BRVComponent &component,
                               Spider::Array<Spider::Rational> &reps,
                               std::int64_t lcmFactor) const {
    for (const auto &v : component.vertices) {
        v->setRepetitionValue(Spider::Rational{reps[v->getIx()] * lcmFactor}.toInt32());
    }
}

void LCMBRVCompute::checkValidity(Spider::Array<const PiSDFEdge *> &edgeArray) const {
    for (const auto &edge : edgeArray) {
        if (edge->source()->type() == PiSDFVertexType::INTERFACE ||
            edge->sink()->type() == PiSDFVertexType::INTERFACE) {
            continue;
        }
        auto sourceRate = edge->sourceRate();
        auto sinkRate = edge->sinkRate();
        const auto *source = edge->source();
        const auto *sink = edge->sink();

        if (edge->sink()->type() == PiSDFVertexType::DELAY) {
            if (sink->repetitionValue() != 1) {
                throwSpiderException("Delay [%s] has repetition vector value of %"
                                             PRIu32
                                             " instead of 1.", edge->delay()->name().c_str(),
                                     sink->repetitionValue());
            }
        }

        if ((sourceRate * source->repetitionValue()) != (sinkRate * sink->repetitionValue())) {
            throwSpiderException("Edge [%s] -> [%s]. prod(%"
                                         PRIu64
                                         ") * sourceRV(%"
                                         PRIu32
                                         ") != cons(%"
                                         PRIu64
                                         ") * sinkRV(%"
                                         PRIu32
                                         ").",
                                 source->name().c_str(),
                                 sink->name().c_str(),
                                 sourceRate,
                                 source->repetitionValue(),
                                 sinkRate,
                                 sink->repetitionValue());
        }
    }
}
