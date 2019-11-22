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

/* === Include(s) === */

#include <graphs-tools/brv/LCMBRVCompute.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>
#include <cstdint>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

void LCMBRVCompute::execute() {
    /* == Initializes the Rational array == */
    spider::array<spider::Rational> reps{ graph_->vertexCount(), spider::Rational(), StackID::TRANSFO };

    /* == Go through all connected components == */
    for (const auto &component : connectedComponents_) {
        /* == Extract the edges == */
        auto edgeArray = extractEdges(component);

        /* == Extract the rationals == */
        extractRationals(edgeArray, reps);

        /* == Compute the LCM factor for the current component == */
        auto lcmFactor = computeLCM(component, reps);

        /* == Compute the repetition vector values of the current component == */
        computeBRV(component, reps, lcmFactor);

        /* == Update the repetition vector values using the interfaces of the graph == */
        updateBRV(component);

        /* == Check validity of the obtained repetition vector == */
        checkValidity(edgeArray);
    }

    /* == Print BRV if VERBOSE == */
    BRVCompute::print();
}

/* === Private method(s) implementation === */

void LCMBRVCompute::extractRationals(spider::array<const PiSDFEdge *> &edgeArray,
                                     spider::array<spider::Rational> &reps) const {
    auto dummyRational = spider::Rational{ 1 };
    for (const auto &edge:edgeArray) {
        const auto *source = edge->source();
        const auto *sink = edge->sink();
        int64_t sourceRate = edge->sourceRateExpression().evaluate(params_);
        int64_t sinkRate = edge->sinkRateExpression().evaluate(params_);

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

        auto &sourceRational = source->subtype() == PiSDFVertexType::INPUT ? dummyRational : reps[source->ix()];
        auto &sinkRational = sink->subtype() == PiSDFVertexType::OUTPUT ? dummyRational : reps[sink->ix()];

        if (!sinkRational.nominator() && sinkRate) {
            sinkRational = spider::Rational{ sourceRate, sinkRate };
            if (sourceRational.nominator()) {
                sinkRational *= sourceRational;
            }
        }

        if (!sourceRational.nominator() && sourceRate) {
            sourceRational = spider::Rational{ sinkRate, sourceRate };
            if (sinkRational.nominator()) {
                sourceRational *= sinkRational;
            }
        }
    }
}

int64_t LCMBRVCompute::computeLCM(const BRVComponent &component, spider::array<spider::Rational> &reps) {
    int64_t lcmFactor = 1;
    for (const auto &v : component.vertices) {
        lcmFactor = spider::math::lcm(lcmFactor, reps[v->ix()].denominator());
    }
    return lcmFactor;
}

void LCMBRVCompute::computeBRV(const BRVComponent &component,
                               spider::array<spider::Rational> &reps,
                               int64_t lcmFactor) {
    for (const auto &v : component.vertices) {
        v->setRepetitionValue(spider::Rational{ reps[v->ix()] * lcmFactor }.toInt32());
    }
}

void LCMBRVCompute::checkValidity(spider::array<const PiSDFEdge *> &edgeArray) const {
    for (const auto &edge : edgeArray) {
        if (edge->source()->subtype() == PiSDFVertexType::INPUT ||
            edge->sink()->subtype() == PiSDFVertexType::OUTPUT) {
            continue;
        }
        const auto &sourceRate = edge->sourceRateExpression().evaluate(params_);
        const auto &sinkRate = edge->sinkRateExpression().evaluate(params_);
        const auto *source = edge->source();
        const auto *sink = edge->sink();

        if ((sourceRate * source->repetitionValue()) != (sinkRate * sink->repetitionValue())) {
            throwSpiderException("Edge [%s]: prod(%"
                                         PRId64
                                         ") * sourceRV(%"
                                         PRIu32
                                         ") != cons(%"
                                         PRId64
                                         ") * sinkRV(%"
                                         PRIu32
                                         ").",
                                 edge->name().c_str(),
                                 sourceRate,
                                 source->repetitionValue(),
                                 sinkRate,
                                 sink->repetitionValue());
        }
    }
}
