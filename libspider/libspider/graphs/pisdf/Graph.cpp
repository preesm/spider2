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
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/Interface.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/Param.h>

/* === Static variable(s) === */

/* === Static function(s) === */

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
    /* == Destroy / deallocate vertices (subgraphs included) == */
    for (auto &vertex : vertexVector_) {
        destroyVertex(vertex);
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
}

void Spider::PiSDF::Graph::addVertex(Vertex *vertex) {
    switch (vertex->type()) {
        case VertexType::CONFIG:
            configVertexVector_.emplace_back(vertex);
        case VertexType::SPECIAL:
        case VertexType::DELAY:
        case VertexType::NORMAL:
            vertex->setIx(vertexVector_.size());
            vertexVector_.emplace_back(vertex);
            break;
        case VertexType::GRAPH:
            addSubGraph(dynamic_cast<Graph *>(vertex));
            vertex->setIx(vertexVector_.size());
            vertexVector_.emplace_back(vertex);
            break;
        case VertexType::INTERFACE:
            throwSpiderException("can not add interface to graph %s", name().c_str());
        default:
            throwSpiderException("unsupported type of vertex.");
    }
}

void Spider::PiSDF::Graph::removeVertex(Vertex *vertex) {
    if (vertex->subtype() == VertexType::GRAPH) {
        throwSpiderException("removing subgraph using removeVertex. Use removeSubgraph instead.");
    } else if (vertex->type() == VertexType::CONFIG) {
        /* == configVertexVector_ is just a "viewer" for config vertices so we need to find manually == */
        for (auto &cfg : configVertexVector_) {
            if (cfg == vertex) {
                cfg = configVertexVector_.back();
                configVertexVector_.pop_back();
                break;
            }
        }
    }
    removeElement(vertexVector_, vertex);
    destroyVertex(vertex);
}

void Spider::PiSDF::Graph::removeSubgraph(Graph *subgraph) {
    if (!subgraph) {
        return;
    }
    removeElement(vertexVector_, static_cast<Vertex *>(subgraph));
    auto ix = subgraph->subIx_;
    subgraphVector_[ix] = subgraphVector_.back();
    subgraphVector_[ix]->subIx_ = ix;
    subgraphVector_.pop_back();
    destroyVertex(subgraph);
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
    if (param->type() != ParamType::INHERITED && param->dynamic() && !dynamic_) {
        /* == Set dynamic property of the graph to true == */
        dynamic_ = true;

        /* == We need to propagate this property up in the hierarchy == */
        auto *parent = containingGraph();
        while (parent && !parent->dynamic_) {
            /* == If graph was already dynamic then information is already propagated == */
            parent->dynamic_ = true;
            parent = parent->containingGraph();
        }
    }
}

void Spider::PiSDF::Graph::removeParam(Param *param) {
    removeElement(paramVector_, param);
    Spider::destroy(param);
    Spider::deallocate(param);
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
                                              "cpy-" + graph->name() + "-" + this->name_,
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

void Spider::PiSDF::Graph::addSubGraph(Graph *graph) {
    graph->subIx_ = subgraphVector_.size();
    subgraphVector_.push_back(graph);
}

void Spider::PiSDF::Graph::destroyVertex(Vertex *vertex) {
    switch (vertex->subtype()) {
        case VertexType::CONFIG:
        case VertexType::DELAY:
        case VertexType::NORMAL: {
            auto *tmp = dynamic_cast<ExecVertex *>(vertex);
            Spider::destroy(tmp);
            Spider::deallocate(tmp);
        }
            break;
        case VertexType::GRAPH: {
            auto *tmp = dynamic_cast<Graph *>(vertex);
            Spider::destroy(tmp);
            Spider::deallocate(tmp);
        }
            break;
        case VertexType::FORK: {
            auto *tmp = dynamic_cast<ForkVertex *>(vertex);
            Spider::destroy(tmp);
            Spider::deallocate(tmp);
        }
            break;
        case VertexType::JOIN: {
            auto *tmp = dynamic_cast<JoinVertex *>(vertex);
            Spider::destroy(tmp);
            Spider::deallocate(tmp);
        }
            break;
        case VertexType::UPSAMPLE: {
            auto *tmp = dynamic_cast<UpSampleVertex *>(vertex);
            Spider::destroy(tmp);
            Spider::deallocate(tmp);
        }
            break;
        case VertexType::DUPLICATE: {
            auto *tmp = dynamic_cast<DuplicateVertex *>(vertex);
            Spider::destroy(tmp);
            Spider::deallocate(tmp);
        }
            break;
        case VertexType::TAIL: {
            auto *tmp = dynamic_cast<TailVertex *>(vertex);
            Spider::destroy(tmp);
            Spider::deallocate(tmp);
        }
            break;
        case VertexType::HEAD: {
            auto *tmp = dynamic_cast<HeadVertex *>(vertex);
            Spider::destroy(tmp);
            Spider::deallocate(tmp);
        }
            break;
        case VertexType::INIT: {
            auto *tmp = dynamic_cast<InitVertex *>(vertex);
            Spider::destroy(tmp);
            Spider::deallocate(tmp);
        }
            break;
        case VertexType::END: {
            auto *tmp = dynamic_cast<EndVertex *>(vertex);
            Spider::destroy(tmp);
            Spider::deallocate(tmp);
        }
            break;
        case VertexType::INPUT: {
            auto *tmp = dynamic_cast<InputInterface *>(vertex);
            Spider::destroy(tmp);
            Spider::deallocate(tmp);
        }
            break;
        case VertexType::OUTPUT: {
            auto *tmp = dynamic_cast<OutputInterface *>(vertex);
            Spider::destroy(tmp);
            Spider::deallocate(tmp);
        }
            break;
        default:
            throwSpiderException("failed to destroy vertex [%s]", vertex->name().c_str());
    }
}
