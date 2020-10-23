/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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

#include <graphs/sched/SchedEdge.h>
#include <graphs/sched/SchedVertex.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::Edge::Edge(sched::Vertex *source, u32 srcIx, sched::Vertex *sink, u32 snkIx, Fifo alloc) :
        alloc_{ alloc },
        source_{ source }, sink_{ sink },
        srcPortIx_{ srcIx }, snkPortIx_{ snkIx } {
    source->connectOutputEdge(this, srcIx);
    sink->connectInputEdge(this, snkIx);
}

std::string spider::sched::Edge::name() const {
    auto srcName = std::string(source_->name()).append(":").append(std::to_string(srcPortIx_));
    auto snkName = std::string(sink_->name()).append(":").append(std::to_string(snkPortIx_));
    return std::string("edge_").append(srcName).append("-").append(snkName);
}

void spider::sched::Edge::setSource(sched::Vertex *vertex, u32 ix) {
    if (vertex) {
        /* == Disconnect current input edge (if any) == */
        vertex->disconnectOutputEdge(ix);
        /* == Connect this edge == */
        vertex->connectOutputEdge(this, ix);
    }
    /* == Disconnect current snk_ (if any) == */
    if (source_) {
        source_->disconnectOutputEdge(srcPortIx_);
    }
    /* == Set sink of this edge == */
    source_ = vertex;
    srcPortIx_ = ix;
}

void spider::sched::Edge::setSink(sched::Vertex *vertex, u32 ix) {
    if (vertex) {
        /* == Disconnect current input edge (if any) == */
        vertex->disconnectInputEdge(ix);
        /* == Connect this edge == */
        vertex->connectInputEdge(this, ix);
    }
    /* == Disconnect current snk_ (if any) == */
    if (sink_) {
        sink_->disconnectInputEdge(snkPortIx_);
    }
    /* == Set sink of this edge == */
    sink_ = vertex;
    snkPortIx_ = ix;
}
