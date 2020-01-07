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

#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Edge.h>

/* === Function(s) definition === */

spider::pisdf::Vertex::Vertex(const Vertex &other,
                              StackID stack) : name_{ other.name_ },
                                               reference_{ &other },
                                               graph_{ other.graph_ },
                                               ix_{ other.ix_ },
                                               repetitionValue_{ other.repetitionValue_ },
                                               copyCount_{ 0 } {
    inputEdgeVector_ = containers::vector<Edge *>(other.inputEdgeVector_.size(), nullptr, stack);
    outputEdgeVector_ = containers::vector<Edge *>(other.outputEdgeVector_.size(), nullptr, stack);
}


spider::pisdf::Vertex::~Vertex() noexcept {
    if (copyCount_ && log::enabled()) {
        log::error("Removing vertex [%s] with copies out there.\n", name().c_str());
    }
    this->reference_->copyCount_ -= 1;

    /* == If got any Edges left disconnect them == */
    for (size_t ix = 0; ix < inputEdgeVector_.size(); ++ix) {
        disconnectInputEdge(ix);
    }
    for (size_t ix = 0; ix < outputEdgeVector_.size(); ++ix) {
        disconnectOutputEdge(ix);
    }
}

spider::pisdf::Edge *spider::pisdf::Vertex::disconnectInputEdge(size_t ix) {
    auto *edge = disconnectEdge(inputEdgeVector_, ix);
    if (edge) {
        /* == Reset the Edge == */
        edge->setSink(nullptr, SIZE_MAX, Expression());
    }
    return edge;
}

spider::pisdf::Edge *spider::pisdf::Vertex::disconnectOutputEdge(size_t ix) {
    auto *edge = disconnectEdge(outputEdgeVector_, ix);
    if (edge) {
        /* == Reset the Edge == */
        edge->setSource(nullptr, SIZE_MAX, Expression());
    }
    return edge;
}

spider::pisdf::Edge *spider::pisdf::Vertex::disconnectEdge(spider::vector<Edge *> &edges, size_t ix) {
    auto *&edge = edges.at(ix);
    Edge *ret = edge;
    if (edge) {
        edge = nullptr;
    }
    return ret;
}

void spider::pisdf::Vertex::connectEdge(spider::vector<Edge *> &edges, Edge *edge, size_t ix) {
    auto *&current = edges.at(ix);
    if (!current) {
        current = edge;
        return;
    }
    throwSpiderException("Edge already exists at position: %zu", ix);
}
