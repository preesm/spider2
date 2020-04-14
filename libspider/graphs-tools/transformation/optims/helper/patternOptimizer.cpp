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
/* === Include(s) === */

#include <graphs-tools/transformation/optims/helper/patternOptimizer.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Graph.h>

/* === Function(s) definition === */

bool spider::optims::reduceFFJJWorker(pisdf::VertexType type,
                                      pisdf::Graph *graph,
                                      VertexMaker makeNewVertex,
                                      NextVertexGetter getNextVertex,
                                      EdgeRemover removeEdge,
                                      EdgeConnecter reconnect) {
    auto verticesToOptimize = factory::vector<pisdf::Vertex *>(StackID::TRANSFO);

    /* == Search for the pair of fork to optimize == */
    for (auto &vertex : graph->vertices()) {
        auto *vertexA = vertex.get();
        if (vertex->subtype() == type && vertex->scheduleTaskIx() == SIZE_MAX) {
            auto *vertexB = getNextVertex(vertexA);
            if (vertexB->subtype() == type && vertexB->scheduleTaskIx() == SIZE_MAX) {
                verticesToOptimize.emplace_back(vertexA);
            }
        }
    }

    /* == Do the optimization == */
    for (auto it = std::begin(verticesToOptimize); it != std::end(verticesToOptimize); ++it) {
        auto *vertexA = (*it);                  /* = Second fork or first join = */
        auto *vertexB = getNextVertex(vertexA); /* = First fork or second join = */

        /* == Remove edge == */
        /* == If type is JOIN, then it gets the output edge of the first join
         *    else if type if FORK, it gets the input edge of the second fork == */
        const auto offset = removeEdge(vertexA, vertexB);

        /* == Creates new vertex == */
        auto *newVertex = makeNewVertex(vertexA, vertexB);

        /* == Avoid passing two other parameters == */
        const auto vertexAEdgeCount = std::max(vertexA->inputEdgeCount(), vertexA->outputEdgeCount());
        const auto vertexBEdgeCount = std::max(vertexB->inputEdgeCount(), vertexB->outputEdgeCount());

        /* == Link edges of vertexB into newVertex == */
        /* == If type is JOIN, connects every input edges of the first join into the new join
         *    else if type if FORK, connects every output edges of the second fork into the new fork == */
        for (size_t i = 0; i < offset; ++i) {
            reconnect(vertexB, i, newVertex, i);
        }
        for (size_t i = offset + 1; i < vertexBEdgeCount; ++i) {
            reconnect(vertexB, i, newVertex, i + vertexAEdgeCount - 1);
        }

        /* == Link edges of vertexA into newVertex == */
        /* == If type is JOIN, connects every input edges of the second join into the new join
         *    else if type if FORK, connects every output edges of the first fork into the new fork == */
        for (size_t i = 0; i < vertexAEdgeCount; ++i) {
            reconnect(vertexA, i, newVertex, i + offset);
        }

        /* == Search for the pair to modify (if any) == */
        for (auto it2 = std::next(it); it2 != std::end(verticesToOptimize); ++it2) {
            auto *secVertexA = (*it2);
            if (secVertexA == vertexA || secVertexA == vertexB) {
                (*it2) = newVertex;
            }
        }
        /* == Remove the vertices == */
        if (spider::log::enabled<spider::log::OPTIMS>()) {
            spider::log::verbose<spider::log::OPTIMS>("Optimizer: removing [%s] and [%s] vertices.\n",
                                                      vertexB->name().c_str(), vertexA->name().c_str());
        }
        graph->removeVertex(vertexB);
        graph->removeVertex(vertexA);
    }
    return verticesToOptimize.empty();
}
