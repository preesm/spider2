/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <graphs-tools/transformation/srdag/visitors/SRDAGCopyVertexVisitor.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/srdag/SRDAGGraph.h>
#include <graphs/srdag/SRDAGEdge.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <api/pisdf-api.h>

/* === Function(s) definition === */

void spider::srdag::SRDAGCopyVertexVisitor::visit(pisdf::Vertex *vertex) {
    switch (vertex->subtype()) {
        case pisdf::VertexType::DELAY: {
            /* == This a trick to ensure proper coherence even with recursive delay init == */
            /* == For given scenario:   A -> | delay | -> B
             *                         setter --^ --> getter
             *    This will produce this:
             *                          setter -> | delay | -> getter
             *                               A -> |       | -> B
             *    But in reality the vertex does not make it after the SR-Transformation.
             */
            auto *clone = make<srdag::Vertex, StackID::TRANSFO>(vertex, 0, 2, 2);
            clone->setExecutable(false);
            /* == Add clone to the srdag == */
            srdag_->addVertex(clone);
            ix_ = srdag_->vertexCount() - 1;
        }
            break;
        default:
            makeClone(vertex);
            break;
    }
}

void spider::srdag::SRDAGCopyVertexVisitor::visit(pisdf::Graph *graph) {
    /* == Clone the vertex == */
    makeClone(graph);
}

/* === Private method(s) === */

void spider::srdag::SRDAGCopyVertexVisitor::makeClone(pisdf::Vertex *vertex) {
    for (uint32_t it = 0; it < vertex->repetitionValue(); ++it) {
        auto *clone = make<srdag::Vertex, StackID::TRANSFO>(vertex,
                                                            it,
                                                            vertex->inputEdgeCount(),
                                                            vertex->outputEdgeCount());
        clone->setExecutable(vertex->executable());
        /* == Add clone to the srdag == */
        srdag_->addVertex(clone);
        /* == Get the cloned parameters == */
        for (const auto ix : vertex->inputParamIxVector()) {
            clone->addInputParameter(job_.params_[ix]);
        }
        for (const auto ix : vertex->refinementParamIxVector()) {
            clone->addRefinementParameter(job_.params_[ix]);
        }
    }
    ix_ = (srdag_->vertexCount() - 1) - (vertex->repetitionValue() - 1);
}

#endif