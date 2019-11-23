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
#include <graphs/pisdf/visitors/GraphAddVertexVisitor.h>
#include <graphs/pisdf/visitors/GraphRemoveVertexVisitor.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/interfaces/Interface.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/params/Param.h>
#include <scenario/Scenario.h>


/* === Method(s) implementation === */

spider::pisdf::Graph::Graph(std::string name,
                            uint32_t vertexCount,
                            uint32_t edgeCount,
                            uint32_t paramCount,
                            uint32_t edgeINCount,
                            uint32_t edgeOUTCount,
                            uint32_t cfgVertexCount,
                            StackID stack) : Vertex(std::move(name),
                                                    edgeINCount,
                                                    edgeOUTCount,
                                                    stack),
                                             inputInterfaceArray_{ edgeINCount, stack },
                                             outputInterfaceArray_{ edgeOUTCount, stack } {
    vertexVector_.reserve(vertexCount);
    edgeVector_.reserve(edgeCount);
    paramVector_.reserve(paramCount);
    configVertexVector_.reserve(cfgVertexCount);

    /* == Create the input interfaces == */
    for (uint32_t i = 0; i < edgeINCount; ++i) {
        auto *interface = spider::make<InputInterface>(stack, "in_" + std::to_string(i), stack);
        interface->setIx(i);
        inputInterfaceArray_[i] = interface;
        interface->setGraph(this);
    }

    /* == Create the output interfaces == */
    for (uint32_t i = 0; i < edgeOUTCount; ++i) {
        auto *interface = spider::make<OutputInterface>(stack, "out_" + std::to_string(i), stack);
        interface->setIx(i);
        outputInterfaceArray_[i] = interface;
        interface->setGraph(this);
    }
}

spider::pisdf::Graph::~Graph() {
    GraphRemoveVertexVisitor rmVertexVisitor{ this };
    /* == Destroy / deallocate vertices (subgraphs included) == */
    for (auto &vertex : vertexVector_) {
        vertex->visit(&rmVertexVisitor);
    }

    /* == Destroy / deallocate interfaces == */
    for (auto &interface : inputInterfaceArray_) {
        spider::destroy(interface);
    }
    for (auto &interface : outputInterfaceArray_) {
        spider::destroy(interface);
    }

    /* == Destroy / deallocate edges == */
    for (auto &edge : edgeVector_) {
        spider::destroy(edge);
    }

    /* == Destroy / deallocate params == */
    for (auto &param : paramVector_) {
        spider::destroy(param);
    }

    /* == Destroy the scenario (if any) == */
    spider::destroy(scenario_);
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
    edge->setIx(static_cast<uint32_t>(edgeVector_.size()));
    edgeVector_.push_back(edge);
    edge->setGraph(this);
}

void spider::pisdf::Graph::removeEdge(Edge *edge) {
    removeElement(edgeVector_, edge);
    spider::destroy(edge);
}

void spider::pisdf::Graph::moveEdge(Edge *elt, Graph *graph) {
    if (graph) {
        removeElement(edgeVector_, elt);
        graph->addEdge(elt);
    }
}

void spider::pisdf::Graph::addParam(Param *param) {
    /* == Check if a parameter with the same name already exists in the scope of this graph == */
    if (this->param(param->name())) {
        throwSpiderException("Parameter [%s] already exist in graph [%s].", param->name().c_str(), name().c_str());
    }
    param->setIx(static_cast<uint32_t>(paramVector_.size()));
    param->setGraph(this);
    paramVector_.push_back(param);
    dynamic_ |= (param->dynamic() && param->type() != ParamType::INHERITED);
}

void spider::pisdf::Graph::removeParam(Param *param) {
    removeElement(paramVector_, param);
    spider::destroy(param);
}

void spider::pisdf::Graph::moveParam(Param *elt, Graph *graph) {
    if (graph) {
        removeElement(paramVector_, elt);
        graph->addParam(elt);
    }
}

spider::pisdf::Param *spider::pisdf::Graph::param(const std::string &name) const {
    for (auto &p : paramVector_) {
        if (p->name() == name) {
            return p;
        }
    }
    return nullptr;
}

spider::pisdf::Vertex *spider::pisdf::Graph::forwardEdge(const Edge *e) {
    if (e->source() == this) {
        return outputInterfaceArray_[e->sourcePortIx()];
    }
    return inputInterfaceArray_[e->sinkPortIx()];
}

spider::Scenario *spider::pisdf::Graph::createScenario() {
    if (!scenario_) {
        scenario_ = spider::make<Scenario, StackID::SCENARIO>();
    }
    return scenario_;
}

/* === Private method(s) === */

template<class T>
void spider::pisdf::Graph::removeElement(spider::vector<T *> &eltVector, T *elt) {
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