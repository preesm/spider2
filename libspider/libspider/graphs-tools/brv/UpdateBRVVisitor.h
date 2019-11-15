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

#include <graphs/pisdf/visitors/DefaultVisitor.h>
#include <graphs/pisdf/specials/ConfigVertex.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>

namespace Spider {

    /* === Class definition === */

    struct UpdateBRVVisitor : public Spider::PiSDF::DefaultVisitor {

        explicit UpdateBRVVisitor(std::uint64_t &scaleFactor,
                                  const Spider::vector<PiSDFParam *> &paramVector) : scaleFactor_{ scaleFactor },
                                                                                     paramVector_{ paramVector } { }

        inline void visit(Spider::PiSDF::ExecVertex *) override { }

        inline void visit(Spider::PiSDF::ConfigVertex *vertex) override {
            for (const auto &edge : vertex->outputEdgeArray()) {
                updateFromInputIf(edge);
            }
        }

        inline void visit(Spider::PiSDF::InputInterface *interface) override {
            updateFromInputIf(interface->outputEdge());
        }

        inline void visit(Spider::PiSDF::OutputInterface *interface) override {
            const auto &edge = interface->inputEdge();
            std::uint64_t sourceRate = edge->sourceRateExpression().evaluate(paramVector_);
            std::uint64_t sinkRate = edge->sinkRateExpression().evaluate(paramVector_);
            auto totalProd = sourceRate * edge->source()->repetitionValue() * scaleFactor_;
            if (totalProd && totalProd < sinkRate) {
                /* == Return ceil(interfaceCons / vertexProd) == */
                scaleFactor_ *= Spider::Math::ceilDiv(sinkRate, totalProd);
            }
        }

        std::uint64_t &scaleFactor_;
        const Spider::vector<PiSDFParam *> &paramVector_;
    private:
        inline void updateFromInputIf(const PiSDFEdge *edge) {
            std::uint64_t sourceRate = edge->sourceRateExpression().evaluate(paramVector_);
            std::uint64_t sinkRate = edge->sinkRateExpression().evaluate(paramVector_);
            const auto &totalCons = sinkRate * edge->sink()->repetitionValue() * scaleFactor_;
            if (totalCons && totalCons < sourceRate) {
                /* == Return ceil( prod / vertexCons) == */
                scaleFactor_ *= Spider::Math::ceilDiv(sourceRate, totalCons);
            }
        }
    };

}
#endif //SPIDER2_UPDATEBRVVISITOR_H
