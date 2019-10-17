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

#include <graphs/tmp/Vertex.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

Spider::PiSDF::Vertex::Vertex(std::string name,
                              Spider::PiSDF::VertexType type,
                              std::uint32_t edgeINCount,
                              std::uint32_t edgeOUTCount,
                              Graph *graph,
                              StackID stack) : graph_{graph},
                                               name_{std::move(name)},
                                               type_{type},
                                               inputEdgeArray_{edgeINCount, nullptr, stack},
                                               outputEdgeArray_{edgeOUTCount, nullptr, stack},
                                               reference_{this} {

}

void Spider::PiSDF::Vertex::connectInputEdge(Edge *edge, std::uint32_t ix) {
    connectEdge(inputEdgeArray_, edge, ix);
}

void Spider::PiSDF::Vertex::connectOutputEdge(Edge *edge, std::uint32_t ix) {
    connectEdge(outputEdgeArray_, edge, ix);
}

void Spider::PiSDF::Vertex::disconnectInputEdge(std::uint32_t ix) {
    disconnectEdge(inputEdgeArray_, ix);
}

void Spider::PiSDF::Vertex::disconnectOutputEdge(std::uint32_t ix) {
    disconnectEdge(outputEdgeArray_, ix);
}

/* === Private method(s) === */

void Spider::PiSDF::Vertex::disconnectEdge(Spider::Array<Edge *> &edges, std::uint32_t ix) {
    auto *&edge = edges.at(ix);
    if (!edge) {
        return;
    }
    edge = nullptr;
}

void Spider::PiSDF::Vertex::connectEdge(Spider::Array<Edge *> &edges, Edge *edge, std::uint32_t ix) {
    auto *&current = edges.at(ix);
    if (!current) {
        current = edge;
    }
    throwSpiderException("Edge already exists at position: %"
                                 PRIu32
                                 "", ix);
}
