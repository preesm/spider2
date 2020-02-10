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
#include <graphs/pisdf/Graph.h>
#include <api/pisdf-api.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::pisdf::Delay::Delay(Expression expression, Edge *edge,
                            Vertex *setter, size_t setterPortIx, Expression setterRateExpression,
                            Vertex *getter, size_t getterPortIx, Expression getterRateExpression,
                            bool persistent) : expression_{ std::move(expression) },
                                               edge_{ edge },
                                               setter_{ setter },
                                               setterPortIx_{ setterPortIx },
                                               getter_{ getter },
                                               getterPortIx_{ getterPortIx },
                                               persistent_{ persistent } {
    if (!edge_) {
        throwSpiderException("Delay can not be created on nullptr edge.");
    } else if (edge_->delay()) {
        throwSpiderException("Edge can only have one delay.");
    }
    if (persistent && (setter || getter)) {
        throwSpiderException("Persistent delay on edge [%s] can not have setter nor getter.", edge->name().c_str());
    }

    /* == If no setter is provided then an INIT is created == */
    if (!setter_) {
        setterPortIx_ = 0; /* = Ensure the proper value of the port ix = */
        setter_ = api::createInit(edge->graph(),
                                  "init-" + edge->sink()->name() + "_" + std::to_string(edge->sinkPortIx()));
    }

    /* == If no getter is provided then an END is created == */
    if (!getter_) {
        getterPortIx_ = 0; /* = Ensure the proper value of the port ix = */
        getter_ = api::createEnd(edge->graph(),
                                 "end-" + edge->source()->name() + "_" + std::to_string(edge->sourcePortIx()));
    }

    /* == Create virtual vertex and connect it to setter / getter == */
    vertex_ = make<ExecVertex, StackID::PISDF>(VertexType::DELAY, this->name(), 1u, 1u);
    edge->graph()->addVertex(vertex_);

    auto *setterEdge = make<Edge, StackID::PISDF>(setter_, setterPortIx_, std::move(setterRateExpression),
                                                  vertex_, 0u, expression_);
    auto *getterEdge = make<Edge, StackID::PISDF>(vertex_, 0u, expression_,
                                                  getter_, getterPortIx_, std::move(getterRateExpression));

    /* == Add things to the graph == */
    edge->graph()->addEdge(setterEdge);
    edge->graph()->addEdge(getterEdge);
    edge_->setDelay(this);
}

std::string spider::pisdf::Delay::name() const {
    return "delay-" +
           edge_->source()->name() + "_" + std::to_string(edge_->sourcePortIx()) + "--" +
           edge_->sink()->name() + "_" + std::to_string(edge_->sinkPortIx());
}


int64_t spider::pisdf::Delay::value() const {
    return expression_.evaluate();
}

int64_t spider::pisdf::Delay::value(const spider::vector<std::shared_ptr<Param>> &params) const {
    return expression_.evaluate(params);
}
