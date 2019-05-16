/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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

/* === Includes === */

#include <graphs/pisdf/PiSDFVertex.h>
#include <graphs/pisdf/PiSDFGraph.h>
#include <graphs/pisdf/PiSDFEdge.h>
#include <common/memory/Allocator.h>

/* === Methods implementation === */

PiSDFVertex::PiSDFVertex(PiSDFGraph *graph,
                         PiSDFType type,
                         PiSDFSubType subType,
                         std::uint32_t nEdgesIN,
                         std::uint32_t nEdgesOUT,
                         std::string name) : graph_{graph},
                                             name_{std::move(name)},
                                             type_{type},
                                             subType_{subType},
                                             nEdgesIN_{nEdgesIN},
                                             nEdgesOUT_{nEdgesOUT},
                                             inputEdgeArray_(StackID::PISDF, nEdgesIN),
                                             outputEdgeArray_(StackID::PISDF, nEdgesOUT) {
    for (std::uint32_t i = 0; i < nEdgesIN; ++i) {
        inputEdgeArray_[i] = nullptr;
    }
    for (std::uint32_t i = 0; i < nEdgesOUT; ++i) {
        outputEdgeArray_[i] = nullptr;
    }
}

void PiSDFVertex::setSubGraph(PiSDFGraph *subgraph) {
    if (subgraph_) {
        throwSpiderException("Vertex [%s] already has a subgraph.", name_.c_str());
    }
    if (!subgraph) {
        throwSpiderException("Trying to set nullptr subgraph to vertex [%s]", name_.c_str());
    }
    hierarchical_ = true;
    subgraph_ = subgraph;
    if (subgraph->parentVertex() != this) {
        subgraph->setParentVertex(this);
    }
    if (!graph_) {
        throwSpiderException("Vertex [%s] is not part of any graph.", name_.c_str());
    }
    graph_->addSubGraph(this);
}
