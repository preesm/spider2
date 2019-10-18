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

#include <graphs/tmp/Graph.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

Spider::PiSDF::Graph::Graph(std::string name,
                            std::uint32_t edgeINCount,
                            std::uint32_t edgeOUTCount,
                            Graph *graph,
                            StackID stack) : Vertex(std::move(name),
                                                    VertexType::INTERFACE,
                                                    edgeINCount,
                                                    edgeOUTCount,
                                                    graph,
                                                    stack) {
    inputInterfaceVector_.reserve(edgeINCount);
    outputInterfaceVector_.reserve(edgeOUTCount);
}

Spider::PiSDF::Graph::~Graph() {
    /* == Destroy / deallocate subgraphs == */
    for (auto &subgraph : subgraphVector_) {
        Spider::destroy(subgraph);
        Spider::deallocate(subgraph);
    }

    /* == Destroy / deallocate vertices == */
    for (auto &vertex : vertexVector_) {
        Spider::destroy(vertex);
        Spider::deallocate(vertex);
    }
    for (auto &vertex : configVertexVector_) {
        Spider::destroy(vertex);
        Spider::deallocate(vertex);
    }

    /* == Destroy / deallocate interfaces == */
    for (auto &interface : inputInterfaceVector_) {
        Spider::destroy(interface);
        Spider::deallocate(interface);
    }
    for (auto &interface : outputInterfaceVector_) {
        Spider::destroy(interface);
        Spider::deallocate(interface);
    }

    /* == Destroy / deallocate edges == */
    for (auto &edge : edgeVector_) {
        Spider::destroy(edge);
        Spider::deallocate(edge);
    }
}

void Spider::PiSDF::Graph::removeVertex(Vertex *vertex) {
    removeElement(vertexVector_, vertex);
}

void Spider::PiSDF::Graph::removeEdge(Edge *edge) {
    removeElement(edgeVector_, edge);
}

/* === Private method(s) === */

template<class T>
void Spider::PiSDF::Graph::removeElement(Spider::vector<T *> &eltVector, T *elt) {
    if (!elt) {
        return;
    }
    if (elt->containingGraph() != this) {
        throwSpiderException("Trying to remove an element not from this graph.");
    }
    auto ix = elt->ix();
    if (eltVector[ix] != elt) {
        throwSpiderException("Different element in ix position. Expected: %s -- Got: %s", elt->name().c_str(),
                             eltVector[ix]->name().c_str());
    }
    eltVector[ix] = eltVector.back();
    eltVector[ix]->setIx(ix);
    eltVector.pop_back();
    Spider::destroy(elt);
    Spider::deallocate(elt);
}

void Spider::PiSDF::Graph::addVertex(Vertex *vertex) {
    switch (vertex->type()) {
        case VertexType::NORMAL:
            vertex->setIx(vertexVector_.size());
            vertexVector_.push_back(vertex);
            break;
        case VertexType::CONFIG:
            break;
        case VertexType::GRAPH:
            addSubGraph(dynamic_cast<Graph *>(vertex));
            break;
        case VertexType::INTERFACE:
            addInterface(dynamic_cast<Interface *>(vertex));
            break;
        case VertexType::DELAY:
            break;
    }
}

void Spider::PiSDF::Graph::addInterface(Interface *interface) {
    Spider::vector<Interface *> *interfaceVector = nullptr;
    switch (interface->subtype()) {
        case InterfaceType::INPUT:
            if (inputInterfaceVector_.size() == edgesINCount()) {
                throwSpiderException("Graph [%s]: can not have more interfaces than input edges.", name().c_str());
            }
            interfaceVector = &inputInterfaceVector_;
            break;
        case InterfaceType::OUTPUT:
            if (outputInterfaceVector_.size() == edgesOUTCount()) {
                throwSpiderException("Graph [%s]: can not have more interfaces than output edges.", name().c_str());
            }
            interfaceVector = &outputInterfaceVector_;
            break;
        default:
            throwSpiderException("Invalid interface type.");
    }
    interface->setIx(interfaceVector->size());
    interfaceVector->push_back(interface);
}

void Spider::PiSDF::Graph::addSubGraph(Graph *graph) {
    graph->setIx(subgraphVector_.size());
    subgraphVector_.push_back(graph);
}
