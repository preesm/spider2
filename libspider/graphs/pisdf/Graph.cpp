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
#include <graphs-tools/helper/visitors/PiSDFDefaultVisitor.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/Interface.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Delay.h>

/* === Private structure(s) === */

struct spider::pisdf::Graph::RemoveSubgraphVisitor final : public DefaultVisitor {

    explicit RemoveSubgraphVisitor(Graph *graph) : graph_{ graph } { };

    /* === Method(s) === */

    inline void visit(Graph *subgraph) override {
        /* = Save the index in the subgraphVector_ = */
        auto ix = subgraph->subIx_;

        /* == Remove the subgraph from the subgraph vector == */
        graph_->subgraphVector_[ix] = graph_->subgraphVector_.back();
        graph_->subgraphVector_[ix]->subIx_ = ix;
        graph_->subgraphVector_.pop_back();
    }

    Graph *graph_ = nullptr;
};

struct spider::pisdf::Graph::AddSubgraphVisitor final : public DefaultVisitor {

    explicit AddSubgraphVisitor(Graph *graph) : graph_{ graph } { }

    inline void visit(Graph *subgraph) override {
        /* == Add the subgraph in the "viewer" vector == */
        subgraph->subIx_ = graph_->subgraphVector_.size();
        graph_->subgraphVector_.emplace_back(subgraph);
    }

    Graph *graph_ = nullptr;
};

/* === Method(s) implementation === */

spider::pisdf::Graph::Graph(std::string name,
                            size_t vertexCount,
                            size_t edgeCount,
                            size_t paramCount,
                            size_t numberOfInputEdge,
                            size_t numberOfOutputEdge,
                            size_t cfgVertexCount) :
        AbstractGraph<Graph, Vertex, Edge>(stack_t < StackID::PISDF > { }, vertexCount, edgeCount),
        Vertex(VertexType::GRAPH, std::move(name), numberOfInputEdge, numberOfOutputEdge),
        configVertexVector_{ factory::vector<Vertex *>(StackID::PISDF) },
        subgraphVector_{ factory::vector<Graph *>(StackID::PISDF) },
        paramVector_{ factory::vector<std::shared_ptr<Param>>(StackID::PISDF) },
        inputInterfaceVector_{ factory::vector<spider::unique_ptr<Interface>>(StackID::PISDF) },
        outputInterfaceVector_{ factory::vector<spider::unique_ptr<Interface>>(StackID::PISDF) } {

    /* == Reserve the memory == */
    paramVector_.reserve(paramCount);
    configVertexVector_.reserve(cfgVertexCount);
    inputInterfaceVector_.reserve(numberOfInputEdge);
    outputInterfaceVector_.reserve(numberOfOutputEdge);

    /* == Create the input interfaces == */
    for (size_t i = 0; i < numberOfInputEdge; ++i) {
        auto *interface = make<Interface, StackID::PISDF>(VertexType::INPUT, "in_" + std::to_string(i));
        addInputInterface(interface);
    }

    /* == Create the output interfaces == */
    for (size_t i = 0; i < numberOfOutputEdge; ++i) {
        auto *interface = make<Interface, StackID::PISDF>(VertexType::OUTPUT, "out_" + std::to_string(i));
        addOutputInterface(interface);
    }
}

void spider::pisdf::Graph::clear() {
    AbstractGraph::clear();
    paramVector_.clear();
    subgraphVector_.clear();
    configVertexVector_.clear();
}

size_t spider::pisdf::Graph::totalActorCount() const {
    size_t totalCount{ vertexCount() - subgraphCount() };
    for (auto &subgraph : subgraphs()) {
        totalCount += subgraph->totalActorCount();
    }
    return totalCount;
}

void spider::pisdf::Graph::addInputInterface(Interface *interface) {
    if (!interface || interface->subtype() != VertexType::INPUT) {
        return;
    }
    /* == Adds the interface to the graph == */
    interface->setIx(inputInterfaceVector_.size());
    interface->setGraph(this);
    inputInterfaceVector_.emplace_back(interface);

    /* == Resize the input edge vector == */
    if (inputEdgeCount() < inputInterfaceVector_.size()) {
        inputEdgeVector_.emplace_back(nullptr);
    }
}

void spider::pisdf::Graph::addOutputInterface(Interface *interface) {
    if (!interface || interface->subtype() != VertexType::OUTPUT) {
        return;
    }
    /* == Adds the interface to the graph == */
    interface->setIx(outputInterfaceVector_.size());
    interface->setGraph(this);
    outputInterfaceVector_.emplace_back(interface);

    /* == Resize the output edge vector == */
    if (outputEdgeCount() < outputInterfaceVector_.size()) {
        outputEdgeVector_.emplace_back(nullptr);
    }
}

void spider::pisdf::Graph::addVertex(Vertex *vertex) {
    if (!vertex) {
        return;
    }
    AbstractGraph::addVertex(vertex);
    if (vertex->subtype() == VertexType::CONFIG) {
        /* == Add config vertex to the "viewer" vector == */
        configVertexVector_.emplace_back(vertex);
    } else if (vertex->hierarchical()) {
        AddSubgraphVisitor visitor{ this };
        vertex->visit(&visitor);
    }
}

void spider::pisdf::Graph::removeVertex(Vertex *vertex) {
    if (!vertex) {
        return;
    }
    if (vertex->subtype() == VertexType::CONFIG) {
        /* == configVertexVector_ is just a "viewer" for config vertices so we need to find manually == */
        for (auto &cfg : configVertexVector_) {
            if (cfg == vertex) {
                cfg = configVertexVector_.back();
                configVertexVector_.pop_back();
                break;
            }
        }
    } else if (vertex->hierarchical()) {
        RemoveSubgraphVisitor visitor{ this };
        vertex->visit(&visitor);
    }
    AbstractGraph::removeVertex(vertex);
}


void spider::pisdf::Graph::moveVertex(Vertex *vertex, Graph *graph) {
    if (!vertex || !graph) {
        return;
    }
    AbstractGraph::moveVertex(vertex, graph);
    if (vertex->subtype() == VertexType::CONFIG) {
        /* == configVertexVector_ is just a "viewer" for config vertices so we need to find manually == */
        for (auto &cfg : configVertexVector_) {
            if (cfg == vertex) {
                cfg = configVertexVector_.back();
                configVertexVector_.pop_back();
                break;
            }
        }
    } else if (vertex->hierarchical()) {
        RemoveSubgraphVisitor visitor{ this };
        vertex->visit(&visitor);
    }
}

void spider::pisdf::Graph::addParam(std::shared_ptr<Param> param) {
    /* == Check if a parameter with the same name already exists in the scope of this graph == */
    for (auto &p : paramVector_) {
        if (p->name() == param->name()) {
            throwSpiderException("Parameter [%s] already exist in graph [%s].", param->name().c_str(), name().c_str());
        }
    }
    if (!param->graph()) {
        param->setIx(paramVector_.size());
        param->setGraph(this);
    }
    dynamic_ |= (param->dynamic() && param->type() != ParamType::INHERITED);
    paramVector_.emplace_back(std::move(param));
}

void spider::pisdf::Graph::removeParam(const std::shared_ptr<Param> &param) {
    if (!param || param->graph() != this) {
        return;
    }
    out_of_order_erase(paramVector_, param->ix());
    paramVector_[param->ix()]->setIx(param->ix());
    if (param->dynamic() && param->type() != ParamType::INHERITED) {
        /* == Update dynamic property == */
        dynamic_ = false;
        for (auto &p : paramVector_) {
            dynamic_ |= (p->dynamic() && p->type() != ParamType::INHERITED);
            if (dynamic_) {
                break;
            }
        }
    }
}

spider::pisdf::Param *spider::pisdf::Graph::paramFromName(const std::string &name) {
    std::string lowerCaseName = name;
    std::transform(lowerCaseName.begin(), lowerCaseName.end(), lowerCaseName.begin(), ::tolower);
    for (auto &param : paramVector_) {
        if (param->name() == lowerCaseName) {
            return param.get();
        }
    }
    return nullptr;
}

void spider::pisdf::Graph::overrideDynamicProperty(bool value) {
    dynamic_ = value;
}
