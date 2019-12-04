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

#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/specials/DelayVertex.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::pisdf::Edge::Edge(Vertex *source,
                          uint32_t srcIx,
                          Expression &&srcExpr,
                          Vertex *sink,
                          uint32_t snkIx,
                          Expression &&snkExpr) : srcExpression_{ srcExpr },
                                                  snkExpression_{ snkExpr },
                                                  src_{ source },
                                                  snk_{ sink },
                                                  srcPortIx_{ srcIx },
                                                  snkPortIx_{ snkIx } {
    if (!source || !sink) {
        throwSpiderException("nullptr vertex connected to Edge.");
    }
    if (source->graph() != sink->graph()) {
        throwSpiderException("Can not create edge between [%s] and [%s]: not in the same graph.",
                             source->name().c_str(), sink->name().c_str());
    }
    source->connectOutputEdge(this, srcIx);
    sink->connectInputEdge(this, snkIx);
    graph_ = source->graph();
}

spider::pisdf::Edge::~Edge() {
    spider::destroy(delay_);
    source()->disconnectOutputEdge(srcPortIx_);
    sink()->disconnectInputEdge(snkPortIx_);
}

std::string spider::pisdf::Edge::name() const {
    return "edge_" + src_->name() + "-" + snk_->name();
}

void spider::pisdf::Edge::removeDelay() {
    if (delay_) {
        graph_->removeEdge(delay_->vertex_->inputEdge(0));
        graph_->removeEdge(delay_->vertex_->outputEdge(0));
        graph_->removeVertex(delay_->vertex_);
        delay_->vertex_ = nullptr;
    }
    spider::destroy(delay_);
    delay_ = nullptr;
}

spider::pisdf::Vertex *spider::pisdf::Edge::sourceFw() const {
    return src_->forwardEdge(this);
}

spider::pisdf::Vertex *spider::pisdf::Edge::sinkFw() const {
    return snk_->forwardEdge(this);
}

spider::pisdf::Edge *spider::pisdf::Edge::setSource(Vertex *vertex, uint32_t ix, Expression &&expr) {
    if (!vertex) {
        throwSpiderException("Can not set nullptr vertex on edge [%s].", name().c_str());
    }
    auto *edge = vertex->disconnectOutputEdge(ix);
    vertex->connectOutputEdge(this, ix);
    src_->disconnectOutputEdge(srcPortIx_);
    src_ = vertex;
    srcPortIx_ = ix;
    srcExpression_ = std::move(expr);
    return edge;
}

spider::pisdf::Edge *spider::pisdf::Edge::setSink(Vertex *vertex, uint32_t ix, Expression &&expr) {
    if (!vertex) {
        throwSpiderException("Can not set nullptr vertex on edge [%s].", name().c_str());
    }
    auto *edge = vertex->disconnectInputEdge(ix);
    vertex->connectInputEdge(this, ix);
    snk_->disconnectInputEdge(snkPortIx_);
    snk_ = vertex;
    snkPortIx_ = ix;
    snkExpression_ = std::move(expr);
    return edge;
}