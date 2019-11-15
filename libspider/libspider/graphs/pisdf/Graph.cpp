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
#include <graphs/pisdf/GraphVisitors.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/interfaces/Interface.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/params/Param.h>


/* === Method(s) implementation === */

Spider::PiSDF::Graph::Graph(std::string name,
                            std::uint32_t vertexCount,
                            std::uint32_t edgeCount,
                            std::uint32_t paramCount,
                            std::uint32_t edgeINCount,
                            std::uint32_t edgeOUTCount,
                            std::uint32_t cfgVertexCount,
                            Graph *graph,
                            StackID stack) : Vertex(std::move(name),
                                                    VertexType::GRAPH,
                                                    edgeINCount,
                                                    edgeOUTCount,
                                                    graph,
                                                    stack),
                                             inputInterfaceArray_{ edgeINCount, stack },
                                             outputInterfaceArray_{ edgeOUTCount, stack } {
    vertexVector_.reserve(vertexCount);
    edgeVector_.reserve(edgeCount);
    paramVector_.reserve(paramCount);
    configVertexVector_.reserve(cfgVertexCount);

    /* == Create the input interfaces == */
    for (std::uint32_t i = 0; i < edgeINCount; ++i) {
        auto *interface = Spider::allocate<PiSDFInputInterface>(stack);
        Spider::construct(interface, "in_" + std::to_string(i), this, stack);
        interface->setIx(i);
        inputInterfaceArray_[i] = interface;
    }

    /* == Create the output interfaces == */
    for (std::uint32_t i = 0; i < edgeOUTCount; ++i) {
        auto *interface = Spider::allocate<PiSDFOutputInterface>(stack);
        Spider::construct(interface, "out_" + std::to_string(i), this, stack);
        interface->setIx(i);
        outputInterfaceArray_[i] = interface;
    }
}

Spider::PiSDF::Graph::~Graph() {
    RemoveVertexVisitor rmVertexVisitor{ this };
    /* == Destroy / deallocate vertices (subgraphs included) == */
    for (auto &vertex : vertexVector_) {
        vertex->visit(&rmVertexVisitor);
    }

    /* == Destroy / deallocate interfaces == */
    for (auto &interface : inputInterfaceArray_) {
        Spider::destroy(interface);
        Spider::deallocate(interface);
    }
    for (auto &interface : outputInterfaceArray_) {
        Spider::destroy(interface);
        Spider::deallocate(interface);
    }

    /* == Destroy / deallocate edges == */
    for (auto &edge : edgeVector_) {
        Spider::destroy(edge);
        Spider::deallocate(edge);
    }

    /* == Destroy / deallocate edges == */
    for (auto &param : paramVector_) {
        Spider::destroy(param);
        Spider::deallocate(param);
    }
}

void Spider::PiSDF::Graph::addVertex(Vertex *vertex) {
    AddVertexVisitor addVertexVisitor{ this };
    vertex->visit(&addVertexVisitor);
}

void Spider::PiSDF::Graph::removeVertex(Vertex *vertex) {
    RemoveVertexVisitor rmVertexVisitor{ this };
    vertex->visit(&rmVertexVisitor);
}

void Spider::PiSDF::Graph::addEdge(Edge *edge) {
    edge->setIx(edgeVector_.size());
    edgeVector_.push_back(edge);
}

void Spider::PiSDF::Graph::removeEdge(Edge *edge) {
    removeElement(edgeVector_, edge);
    Spider::destroy(edge);
    Spider::deallocate(edge);
}

void Spider::PiSDF::Graph::addParam(Param *param) {
    /* == Check if a parameter with the same name already exists in the scope of this graph == */
    if (this->param(param->name())) {
        throwSpiderException("Parameter [%s] already exist in graph [%s].", param->name().c_str(), name().c_str());
    }
    param->setIx(paramVector_.size());
    paramVector_.push_back(param);
    dynamic_ |= (param->dynamic() && param->type() != ParamType::INHERITED);
}

void Spider::PiSDF::Graph::removeParam(Param *param) {
    removeElement(paramVector_, param);
    Spider::destroy(param);
    Spider::deallocate(param);
}

void Spider::PiSDF::Graph::moveParam(Param *elt, Graph *graph) {
    if (graph) {
        removeElement(paramVector_, elt);
        graph->addParam(elt);
        elt->setGraph(graph);
    }
}

void Spider::PiSDF::Graph::moveEdge(Edge *elt, Graph *graph) {
    if (graph) {
        removeElement(edgeVector_, elt);
        graph->addEdge(elt);
        elt->setGraph(graph);
    }
}


Spider::PiSDF::Param *Spider::PiSDF::Graph::param(const std::string &name) const {
    for (auto &p : paramVector_) {
        if (p->name() == name) {
            return p;
        }
    }
    return nullptr;
}

Spider::PiSDF::Vertex *Spider::PiSDF::Graph::forwardEdge(const Edge *e) {
    if (e->source() == this) {
        return outputInterfaceArray_[e->sourcePortIx()];
    }
    return inputInterfaceArray_[e->sinkPortIx()];
}

/* === Private method(s) === */

Spider::PiSDF::Vertex *Spider::PiSDF::Graph::clone(StackID stack, Graph *graph) const {
    graph = graph ? graph : this->graph_;
    auto *result = Spider::API::createSubraph(graph,
                                              this->name_,
                                              this->vertexCount(),
                                              this->edgeCount(),
                                              this->paramCount(),
                                              this->edgesINCount(),
                                              this->edgesOUTCount(),
                                              this->configVertexCount(),
                                              stack);
    result->dynamic_ = this->dynamic_;
    result->reference_ = this;
    this->copyCount_ += 1;
    return result;
}

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
}