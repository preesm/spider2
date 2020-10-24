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
#include <graphs/sched/SchedGraph.h>

/* === Static function === */

/* === Method(s) implementation === */

void spider::sched::Graph::clear() {
    edgeVector_.clear();
    vertexVector_.clear();
}

void spider::sched::Graph::addVertex(sched::Vertex *vertex) {
    if (!vertex) {
        return;
    }
    vertex->setIx(static_cast<u32>(vertexVector_.size()));
    vertexVector_.emplace_back(vertex);
}

void spider::sched::Graph::removeVertex(sched::Vertex *vertex) {
    if (!vertex) {
        return;
    }
    /* == Assert that vertex is part of the edgeVector_ == */
    auto ix = vertex->ix();
    if (ix >= vertexVector_.size()) {
        throwSpiderException("Trying to remove an element not from this graph.");
    } else if (vertexVector_[ix].get() != vertex) {
        throwSpiderException("Different element in ix position. Expected: %s -- Got: %s", vertex->name().c_str(),
                             vertexVector_[ix]->name().c_str());
    }
    /* == Reset vertex input edges == */
    for (auto &edge : vertex->inputEdges()) {
        if (edge) {
            edge->setSink(nullptr, UINT32_MAX);
        }
    }
    /* == Reset vertex output edges == */
    for (auto &edge : vertex->outputEdges()) {
        if (edge) {
            edge->setSource(nullptr, UINT32_MAX);
        }
    }
    /* == swap and destroy the element == */
    if (vertexVector_.back()) {
        vertexVector_.back()->setIx(ix);
    }
    out_of_order_erase(vertexVector_, ix);
}

void spider::sched::Graph::addEdge(sched::Edge *edge) {
    if (!edge) {
        return;
    }
    edgeVector_.emplace_back(edge);
}

void spider::sched::Graph::removeEdge(sched::Edge *edge) {
    if (!edge) {
        return;
    }
    /* == Reset edge source and sink == */
    edge->setSource(nullptr, UINT32_MAX);
    edge->setSink(nullptr, UINT32_MAX);
    size_t ix = 0;
    for (const auto &e : edges()) {
        if (e.get() == edge) {
            break;
        }
        ix++;
    }
    out_of_order_erase(edgeVector_, ix);
}

spider::sched::Edge *
spider::sched::Graph::createEdge(sched::Vertex *source, u32 srcIx, sched::Vertex *sink, u32 snkIx, Fifo alloc) {
    auto *edge = make<sched::Edge, StackID::SCHEDULE>(source, srcIx, sink, snkIx, alloc);
    addEdge(edge);
    return edge;
}

void spider::sched::Graph::reduce(size_t vertexIx) {
    auto it = std::next(std::begin(vertexVector_), static_cast<long>(vertexIx));
    while (it != std::end(vertexVector_)) {
        if (it->get()->reduce(this)) {
            auto dist = std::distance(std::begin(vertexVector_), it);
            removeVertex(it->get());
            it = std::next(std::begin(vertexVector_), dist);
        } else {
            it++;
        }
    }
}
