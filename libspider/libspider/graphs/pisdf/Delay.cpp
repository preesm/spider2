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

#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <spider-api/pisdf.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

Spider::PiSDF::Delay::Delay(Expression &&expression,
                            Edge *edge,
                            ExecVertex *setter,
                            std::uint32_t setterPortIx,
                            ExecVertex *getter,
                            std::uint32_t getterPortIx,
                            bool persistent,
                            StackID stack) : expression_{std::move(expression)},
                                             edge_{edge},
                                             setter_{setter},
                                             setterPortIx_{setterPortIx},
                                             getter_{getter},
                                             getterPortIx_{getterPortIx},
                                             persistent_{persistent} {
    if (!edge_) {
        throwSpiderException("Delay can not be created on nullptr edge.");
    }

    /* == If no setter is provided then an INIT is created == */
    if (!setter_) {
        setterPortIx_ = 0; /* = Ensure the proper value of the port ix = */
        setter_ = Spider::API::createInit(edge->containingGraph(),
                                          "init-" + edge->sink()->name() + "_" + std::to_string(edge->sinkPortIx()),
                                          0, stack);
    }

    /* == If no getter is provided then an END is created == */
    if (!getter_) {
        getterPortIx_ = 0; /* = Ensure the proper value of the port ix = */
        getter_ = Spider::API::createEnd(edge->containingGraph(),
                                         "end-" + edge->source()->name() + "_" + std::to_string(edge->sourcePortIx()),
                                         0, stack);
    }

    /* == Create virtual vertex and connect it to setter / getter == */
    vertex_ = Spider::allocate<ExecVertex>(stack);
    Spider::construct(vertex_, this->name(), VertexType::DELAY, 1, 1, edge->containingGraph(), stack);
    if (expression_.dynamic()) {
        const auto &rateExpression = expression_.string();
        Spider::API::createEdge(setter_, setterPortIx_, rateExpression, vertex_, 0, rateExpression);
        Spider::API::createEdge(vertex_, 0, rateExpression, getter_, getterPortIx_, rateExpression);
    } else {
        const auto &value = expression_.value();
        Spider::API::createEdge(setter_, setterPortIx_, value, vertex_, 0, value);
        Spider::API::createEdge(vertex_, 0, value, getter_, getterPortIx_, value);
    }
}

Spider::PiSDF::Delay::~Delay() {
    if (vertex_) {
        Spider::destroy(vertex_);
        Spider::deallocate(vertex_);
    }
}

std::string Spider::PiSDF::Delay::name() const {
    return "delay-" +
           edge_->source()->name() + "_" + std::to_string(edge_->sourcePortIx()) + "--" +
           edge_->sink()->name() + "_" + std::to_string(edge_->sinkPortIx());
}

/* === Private method(s) === */
