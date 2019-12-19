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
#ifndef SPIDER2_UPDATEBRVVISITOR_H
#define SPIDER2_UPDATEBRVVISITOR_H

/* === Include(s) === */

#include <graphs/pisdf/visitors/PiSDFDefaultVisitor.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>

namespace spider {

    namespace brv {

        /* === Class definition === */

        struct UpdateBRVVisitor final : public pisdf::DefaultVisitor {

            explicit UpdateBRVVisitor(uint32_t &scaleFactor,
                                      const spider::vector<pisdf::Param *> &paramVector) : scaleFactor_{ scaleFactor },
                                                                                           paramVector_{
                                                                                                   paramVector } { }

            inline void visit(pisdf::Graph *) override { }

            /**
             * @brief Update the repetition vector based on the production rates of a given configuration actor.
             * @param vertex  Config vertex evaluated.
             */
            inline void visit(pisdf::ConfigVertex *vertex) override {
                for (const auto &edge : vertex->outputEdgeArray()) {
                    updateFromInputIf(edge);
                }
            }

            /**
             * @brief Update the repetition vector based on the production of a given input interface.
             * @param interface Interface to evaluate.
             */
            inline void visit(pisdf::InputInterface *interface) override {
                updateFromInputIf(interface->outputEdge());
            }

            /**
             * @brief Update the repetition vector based on the production of a given output interface.
             * @param interface Interface to evaluate.
             */
            inline void visit(pisdf::OutputInterface *interface) override {
                const auto &edge = interface->inputEdge();
                const auto &sourceRate = edge->sourceRateExpression().evaluate(paramVector_);
                const auto &sinkRate = edge->sinkRateExpression().evaluate(paramVector_);
                const auto &totalProd = sourceRate * edge->source()->repetitionValue() * scaleFactor_;
                if (totalProd && totalProd < sinkRate) {
                    /* == Return ceil(interfaceCons / vertexProd) == */
                    scaleFactor_ *= static_cast<uint32_t>(math::ceilDiv(sinkRate, totalProd));
                }
            }

            uint32_t &scaleFactor_;
            const spider::vector<pisdf::Param *> &paramVector_;
        private:
            inline void updateFromInputIf(const pisdf::Edge *edge) {
                const auto &sourceRate = edge->sourceRateExpression().evaluate(paramVector_);
                const auto &sinkRate = edge->sinkRateExpression().evaluate(paramVector_);
                const auto &totalCons = sinkRate * edge->sink()->repetitionValue() * scaleFactor_;
                if (totalCons && totalCons < sourceRate) {
                    /* == Return ceil( prod / vertexCons) == */
                    scaleFactor_ *= static_cast<uint32_t>(math::ceilDiv(sourceRate, totalCons));
                }
            }
        };
    }
}
#endif //SPIDER2_UPDATEBRVVISITOR_H
