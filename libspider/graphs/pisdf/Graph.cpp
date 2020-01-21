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

#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/visitors/PiSDFGraphAddVertexVisitor.h>
#include <graphs/pisdf/visitors/PiSDFGraphRemoveVertexVisitor.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/interfaces/Interface.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Delay.h>

/* === Method(s) implementation === */

spider::pisdf::Graph::Graph(std::string name,
                            size_t vertexCount,
                            size_t edgeCount,
                            size_t paramCount,
                            size_t edgeINCount,
                            size_t edgeOUTCount,
                            size_t cfgVertexCount) : Vertex(std::move(name),
                                                            edgeINCount,
                                                            edgeOUTCount) {
    /* == Reserve the memory == */
    vertexVector_.reserve(vertexCount);
    edgeVector_.reserve(edgeCount);
    paramVector_.reserve(paramCount);
    configVertexVector_.reserve(cfgVertexCount);
    inputInterfaceVector_.resize(edgeINCount, nullptr);
    outputInterfaceVector_.resize(edgeOUTCount, nullptr);

    /* == Create the input interfaces == */
    for (size_t i = 0; i < edgeINCount; ++i) {
        auto *interface = make<InputInterface>(StackID::PISDF, "in_" + std::to_string(i));
        interface->setIx(i);
        inputInterfaceVector_[i] = interface;
        interface->setGraph(this);
    }

    /* == Create the output interfaces == */
    for (size_t i = 0; i < edgeOUTCount; ++i) {
        auto *interface = make<OutputInterface>(StackID::PISDF, "out_" + std::to_string(i));
        interface->setIx(i);
        outputInterfaceVector_[i] = interface;
        interface->setGraph(this);
    }
}

spider::pisdf::Graph::~Graph() noexcept {
    /* == Destroy / deallocate edges == */
    /* ==
     * It is necessary to start with the edges because if an Edge has a delay, it will remove the associated vertices.
     * == */
    for (auto &edge : edgeVector_) {
        destroy(edge);
    }

    /* == Destroy / deallocate vertices (subgraphs included) == */
    GraphRemoveVertexVisitor rmVertexVisitor{ this };
    for (auto &vertex : vertexVector_) {
        vertex->visit(&rmVertexVisitor);
    }

    /* == Destroy / deallocate interfaces == */
    for (auto &interface : inputInterfaceVector_) {
        destroy(interface);
    }
    for (auto &interface : outputInterfaceVector_) {
        destroy(interface);
    }

    /* == Destroy / deallocate params == */
    for (auto &param : paramVector_) {
        destroy(param);
    }
}

void spider::pisdf::Graph::addVertex(Vertex *vertex) {
    GraphAddVertexVisitor addVertexVisitor{ this };
    vertex->visit(&addVertexVisitor);
}

void spider::pisdf::Graph::removeVertex(Vertex *vertex) {
    GraphRemoveVertexVisitor rmVertexVisitor{ this };
    vertex->visit(&rmVertexVisitor);
}

void spider::pisdf::Graph::addEdge(Edge *edge) {
    edge->setIx(edgeVector_.size());
    edgeVector_.emplace_back(edge);
    edge->setGraph(this);
}

void spider::pisdf::Graph::removeEdge(Edge *edge) {
    removeElement(edgeVector_, edge);
    destroy(edge);
}

void spider::pisdf::Graph::addParam(Param *param) {
    /* == Check if a parameter with the same name already exists in the scope of this graph == */
    for (auto &p : paramVector_) {
        if (p->name() == param->name()) {
            throwSpiderException("Parameter [%s] already exist in graph [%s].", param->name().c_str(), name().c_str());
        }
    }
    param->setIx(paramVector_.size());
    param->setGraph(this);
    paramVector_.emplace_back(param);
    dynamic_ |= (param->dynamic() && param->type() != ParamType::INHERITED);
}

void spider::pisdf::Graph::moveVertex(spider::pisdf::Vertex *elt, spider::pisdf::Graph *graph) {
    if (!graph || !elt) {
        return;
    }
    removeElement(vertexVector_, elt);
    graph->addVertex(elt);
}

void spider::pisdf::Graph::moveEdge(spider::pisdf::Edge *elt, spider::pisdf::Graph *graph) {
    if (!graph || !elt) {
        return;
    }
    removeElement(edgeVector_, elt);
    graph->addEdge(elt);
}

void spider::pisdf::Graph::moveParam(spider::pisdf::Param *elt, spider::pisdf::Graph *graph) {
    if (!graph || !elt) {
        return;
    }
    removeElement(paramVector_, elt);
    graph->addParam(elt);
}

spider::pisdf::Param *spider::pisdf::Graph::paramFromName(const std::string &name) {
    std::string lowerCaseName = name;
    std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), ::tolower);
    for (auto &param : paramVector_) {
        if (param->name() == lowerCaseName) {
            return param;
        }
    }
    return nullptr;
}

bool spider::pisdf::Graph::setRunGraphReference(const spider::pisdf::Graph *runGraph) {
    if (dynamic() || !configVertexCount() || runGraphReference_ || !runGraph) {
        return false;
    }
    runGraphReference_ = runGraph;
    return true;
}

/* === Private method(s) === */

template<class T>
void spider::pisdf::Graph::removeElement(spider::vector<T> &eltVector, T &elt) {
    if (!elt) {
        return;
    }
    if (elt->graph() != this) {
        throwSpiderException("Trying to remove an element not from this graph.");
    }
    auto ix = elt->ix();
    if (eltVector[ix] != elt) {
        throwSpiderException("Different element in ix position. Expected: %s -- Got: %s", elt->name().c_str(),
                             eltVector[ix]->name().c_str());
    }
    eltVector[ix] = std::move(eltVector.back());
    eltVector[ix]->setIx(ix);
    eltVector.pop_back();
}
