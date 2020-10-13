/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::pisdf::Edge::Edge(Vertex *source, size_t srcIx, Expression srcExpr,
                          Vertex *sink, size_t snkIx, Expression snkExpr) :
        srcExpression_{ spider::make_unique<Expression>(std::move(srcExpr)) },
        snkExpression_{ spider::make_unique<Expression>(std::move(snkExpr)) },
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
    source->connectOutputEdge(static_cast<Edge *>(this), srcIx);
    sink->connectInputEdge(static_cast<Edge *>(this), snkIx);
}

std::string spider::pisdf::Edge::name() const {
    auto srcName = std::string(src_->name()).append(":").append(std::to_string(srcPortIx_));
    auto snkName = std::string(snk_->name()).append(":").append(std::to_string(snkPortIx_));
    return std::string("edge_").append(srcName).append("-").append(snkName);
}

spider::pisdf::Graph *spider::pisdf::Edge::graph() const {
    if (src_) {
        return src_->graph();
    }
    return snk_ ? snk_->graph() : nullptr;
}

spider::pisdf::Delay *spider::pisdf::Edge::delay() const {
    return delay_.get();
}

int64_t spider::pisdf::Edge::sourceRateValue() const {
    return srcExpression_->value();
}

int64_t spider::pisdf::Edge::sinkRateValue() const {
    return snkExpression_->value();
}

void spider::pisdf::Edge::setDelay(Delay *delay) {
    if (!delay) {
        return;
    } else if (delay_) {
        throwSpiderException("Cannot set delay. Edge [%s] already has a delay.", name().c_str());
    } else if ((snk_ && snk_->subtype() == VertexType::DELAY) ||
               (src_ && src_->subtype() == VertexType::DELAY)) {
        throwSpiderException("Cannot set a delay on a edge connected to a delay.");
    }
    delay_ = unique_ptr<Delay>(delay);
}

void spider::pisdf::Edge::setSource(Vertex *vertex, size_t ix, Expression expr) {
    if (vertex) {
        if (snk_ && vertex->graph() != snk_->graph()) {
            throwSpiderException("Can not set edge between [%s] and [%s]: not in the same graph.",
                                 vertex->name().c_str(), snk_->name().c_str());
        }
        /* == Disconnect current output edge (if any) == */
        vertex->disconnectOutputEdge(ix);

        /* == Connect this edge == */
        vertex->connectOutputEdge(static_cast<Edge *>(this), ix);
    }

    /* == Disconnect current src (if any) == */
    if (src_) {
        src_->disconnectOutputEdge(srcPortIx_);
    }

    /* == Set source of this edge == */
    src_ = vertex;
    srcPortIx_ = ix;
    *(srcExpression_.get()) = std::move(expr);
}

void spider::pisdf::Edge::setSink(Vertex *vertex, size_t ix, Expression expr) {
    if (vertex) {
        if (src_ && vertex->graph() != src_->graph()) {
            throwSpiderException("Can not set edge between [%s] and [%s]: not in the same graph.",
                                 src_->name().c_str(), vertex->name().c_str());
        }
        /* == Disconnect current input edge (if any) == */
        vertex->disconnectInputEdge(ix);

        /* == Connect this edge == */
        vertex->connectInputEdge(static_cast<Edge *>(this), ix);
    }

    /* == Disconnect current snk_ (if any) == */
    if (snk_) {
        snk_->disconnectInputEdge(snkPortIx_);
    }

    /* == Set sink of this edge == */
    snk_ = vertex;
    snkPortIx_ = ix;
    *(snkExpression_.get()) = std::move(expr);
}
