/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
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

#include <graphs/pisdf/Graph.h>
#include <graphs-tools/helper/visitors/PiSDFDefaultVisitor.h>
#include <graphs/pisdf/Vertex.h>
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
        Vertex(VertexType::GRAPH, std::move(name), numberOfInputEdge, numberOfOutputEdge),
        vertexVector_{ factory::vector<spider::unique_ptr<pisdf::Vertex>>(StackID::PISDF) },
        edgeVector_{ factory::vector<spider::unique_ptr<pisdf::Edge>>(StackID::PISDF) },
        configVertexVector_{ factory::vector<Vertex *>(StackID::PISDF) },
        subgraphVector_{ factory::vector<Graph *>(StackID::PISDF) },
        paramVector_{ factory::vector<std::shared_ptr<Param>>(StackID::PISDF) },
        inputInterfaceVector_{ factory::vector<spider::unique_ptr<Interface>>(StackID::PISDF) },
        outputInterfaceVector_{ factory::vector<spider::unique_ptr<Interface>>(StackID::PISDF) } {

    /* == Reserve the memory == */
    vertexVector_.reserve(vertexCount);
    edgeVector_.reserve(edgeCount);
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

void spider::pisdf::Graph::visit(pisdf::Visitor *visitor) {
    visitor->visit(this);
}

void spider::pisdf::Graph::clear() {
    edgeVector_.clear();
    vertexVector_.clear();
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
        auto *tmp = spider::make_n<Edge *, StackID::PISDF>(inputInterfaceVector_.size(), nullptr);
        std::move(inputEdgeArray_.get(), inputEdgeArray_.get() + nINEdges_, tmp);
        inputEdgeArray_.reset(tmp);
        nINEdges_++;
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
        auto *tmp = spider::make_n<Edge *, StackID::PISDF>(outputInterfaceVector_.size(), nullptr);
        std::move(outputEdgeArray_.get(), outputEdgeArray_.get() + nOUTEdges_, tmp);
        outputEdgeArray_.reset(tmp);
        nOUTEdges_++;
    }
}

void spider::pisdf::Graph::addVertex(Vertex *vertex) {
    if (!vertex) {
        return;
    }
    vertex->setIx(vertexVector_.size());
    vertex->setGraph(static_cast<Graph *>(this));
    vertexVector_.emplace_back(vertex);
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
    /* == Assert that vertex is part of the edgeVector_ == */
    assertElement(vertex, vertexVector_);
    /* == Reset vertex input edges == */
    for (auto &edge : vertex->inputEdges()) {
        if (edge) {
            edge->setSink(nullptr, SIZE_MAX, Expression());
        }
    }
    /* == Reset vertex output edges == */
    for (auto &edge : vertex->outputEdges()) {
        if (edge) {
            edge->setSource(nullptr, SIZE_MAX, Expression());
        }
    }
    /* == swap and destroy the element == */
    swapElement(vertex, vertexVector_);
}

void spider::pisdf::Graph::moveVertex(Vertex *vertex, Graph *graph) {
    if (!vertex || !graph || (graph == this)) {
        return;
    }
    /* == Assert that elt is part of the vertexVector_ == */
    assertElement(vertex, vertexVector_);
    /* == Release the unique_ptr before swap to avoid destruction == */
    vertex = vertexVector_[vertex->ix()].release();
    swapElement(vertex, vertexVector_);
    /* == Add the edge to the other graph == */
    graph->addVertex(vertex);
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

void spider::pisdf::Graph::addEdge(Edge *edge) {
    if (!edge) {
        return;
    }
    edge->setIx(edgeVector_.size());
    edgeVector_.emplace_back(edge);
}

void spider::pisdf::Graph::removeEdge(Edge *edge) {
    if (!edge) {
        return;
    }
    /* == Assert that edge is part of the edgeVector_ == */
    assertElement(edge, edgeVector_);
    /* == Reset edge source and sink == */
    edge->setSource(nullptr, SIZE_MAX, Expression());
    edge->setSink(nullptr, SIZE_MAX, Expression());
    /* == swap and destroy the element == */
    swapElement(edge, edgeVector_);
}

void spider::pisdf::Graph::moveEdge(Edge *edge, Graph *graph) {
    if (!graph || (graph == this) || !edge) {
        return;
    }
    /* == Assert that elt is part of the edgeVector_ == */
    assertElement(edge, edgeVector_);
    /* == Release the unique_ptr before swap to avoid destruction == */
    edge = edgeVector_[edge->ix()].release();
    swapElement(edge, edgeVector_);
    /* == Add the edge to the other graph == */
    graph->addEdge(edge);
}

void spider::pisdf::Graph::addParam(std::shared_ptr<Param> param) {
    /* == Check if a parameter with the same name already exists in the scope of this graph == */
    for (const auto &p : paramVector_) {
        if (p->name() == param->name()) {
            throwSpiderException("Parameter [%s] already exist in graph [%s].", param->name().c_str(), name().c_str());
        }
    }
    param->setIx(paramVector_.size());
    paramVector_.emplace_back(std::move(param));
}

void spider::pisdf::Graph::removeParam(const std::shared_ptr<Param> &param) {
    if (!param || param->ix() >= paramVector_.size()) {
        return;
    } else if (paramVector_[param->ix()] == param) {
        const auto tmp = param;
        out_of_order_erase(paramVector_, tmp->ix());
        paramVector_[tmp->ix()]->setIx(tmp->ix());
    }
}

std::shared_ptr<spider::pisdf::Param> spider::pisdf::Graph::paramFromName(const std::string &name) const {
    auto iStrEquals = [](const std::string &s0, const std::string &s1) {
        if (s0.size() != s1.size()) {
            return false;
        }
        auto itS1 = std::begin(s1);
        for (const auto &c0 : s0) {
            if (::tolower(c0) != ::tolower(*(itS1++))) {
                return false;
            }
        }
        return true;
    };
    for (const auto &param : paramVector_) {
        if (iStrEquals(param->name(), name)) {
            return param;
        }
    }
    return nullptr;
}

bool spider::pisdf::Graph::dynamic() const {
    for (const auto &param : paramVector_) {
        if (param->type() == ParamType::DYNAMIC) {
            return true;
        }
    }
    return false;
}

/* === Private method(s) implementation === */

template<class T>
void spider::pisdf::Graph::assertElement(T *elt, spider::vector<unique_ptr<T>> &eltVector) {
    auto ix = elt->ix();
    if (ix >= eltVector.size()) {
        throwSpiderException("Trying to remove an element not from this graph.");
    } else if (eltVector[ix].get() != elt) {
        throwSpiderException("Different element in ix position. Expected: %s -- Got: %s", elt->name().c_str(),
                             eltVector[ix]->name().c_str());
    }
}

template<class T, class U>
void spider::pisdf::Graph::swapElement(T *elt, spider::vector<U> &eltVector) {
    auto ix = elt->ix();
    if (eltVector.back()) {
        eltVector.back()->setIx(ix);
    }
    out_of_order_erase(eltVector, ix);
}
