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

#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Graph.h>
#include <graphs-tools/helper/visitors/PiSDFVisitor.h>

/* === Function(s) definition === */

spider::pisdf::Vertex::Vertex(VertexType type, std::string name, size_t edgeINCount, size_t edgeOUTCount) :
        inputEdgeVector_{ factory::vector<Edge *>(edgeINCount, nullptr, StackID::PISDF) },
        outputEdgeVector_{ factory::vector<Edge *>(edgeOUTCount, nullptr, StackID::PISDF) },
        inputParamVector_{ factory::vector<std::shared_ptr<Param>>(StackID::PISDF) },
        refinementParamVector_{ factory::vector<std::shared_ptr<Param>>(StackID::PISDF) },
        outputParamVector_{ factory::vector<std::shared_ptr<Param>>(StackID::PISDF) },
        name_{ std::move(name) },
        subtype_{ type } {
    checkTypeConsistency();
}

void spider::pisdf::Vertex::connectInputEdge(Edge *edge, size_t pos) {
    connectEdge(inputEdgeVector_, edge, pos);
}

void spider::pisdf::Vertex::connectOutputEdge(Edge *edge, size_t pos) {
    connectEdge(outputEdgeVector_, edge, pos);
}

spider::pisdf::Edge *spider::pisdf::Vertex::disconnectInputEdge(size_t ix) {
    auto *edge = disconnectEdge(inputEdgeVector_, ix);
    if (edge) {
        /* == Reset the Edge == */
        edge->setSink(nullptr, SIZE_MAX, Expression());
    }
    return edge;
}

spider::pisdf::Edge *spider::pisdf::Vertex::disconnectOutputEdge(size_t ix) {
    auto *edge = disconnectEdge(outputEdgeVector_, ix);
    if (edge) {
        /* == Reset the Edge == */
        edge->setSource(nullptr, SIZE_MAX, Expression());
    }
    return edge;
}

void spider::pisdf::Vertex::visit(Visitor *visitor) {
    visitor->visit(this);
    // LCOV_IGNORE: this line can not be reached because above line throw exception
}

void spider::pisdf::Vertex::setAsReference(Vertex *clone) {
    clone->inputEdgeVector_.resize(this->inputEdgeVector_.size(), nullptr);
    clone->outputEdgeVector_.resize(this->outputEdgeVector_.size(), nullptr);
    clone->reference_ = this;
    clone->rtInformation_ = this->rtInformation_;
    clone->inputParamVector_.reserve(this->inputParamVector_.size());
    clone->refinementParamVector_.reserve(this->refinementParamVector_.size());
    clone->outputParamVector_.reserve(this->outputParamVector_.size());
}

void spider::pisdf::Vertex::addInputParameter(std::shared_ptr<Param> param) {
    if (subtype_ != VertexType::GRAPH) {
        inputParamVector_.emplace_back(std::move(param));
    }
}

void spider::pisdf::Vertex::addOutputParameter(std::shared_ptr<Param> param) {
    if (subtype() != VertexType::CONFIG) {
        throwSpiderException("Failed to set output parameter [%s] of vertex [%s]: not a config actor.",
                             param->name().c_str(), name().c_str());
    }
    outputParamVector_.emplace_back(std::move(param));
}

void spider::pisdf::Vertex::addRefinementParameter(std::shared_ptr<Param> param) {
    if (subtype_ != VertexType::GRAPH) {
        refinementParamVector_.emplace_back(std::move(param));
    }
}

std::string spider::pisdf::Vertex::vertexPath() const {
    if (graph_) {
        return graph_->vertexPath().append(":").append(name_);
    }
    return name_;
}

void spider::pisdf::Vertex::setRepetitionValue(uint32_t value) {
    switch (subtype_) {
        case VertexType::CONFIG:
        case VertexType::DELAY:
        case VertexType::EXTERN_IN:
        case VertexType::EXTERN_OUT:
            if (value > 1) {
                throwSpiderException("Vertex [%s] can not have repetition value greater than 1.", name_.c_str());
            }
            repetitionValue_ = value;
            break;
        default:
            repetitionValue_ = value;
            break;
    }
}

void spider::pisdf::Vertex::setInstanceValue(size_t value) {
    if (value >= reference_->repetitionValue()) {
        throwSpiderException("invalid instance value for vertex [%s].", name_.c_str());
    }
    instanceValue_ = value;
}

void spider::pisdf::Vertex::setGraph(spider::pisdf::Graph *graph) {
    if (graph) {
        graph_ = graph;
    }
}

/* === Protected method(s) === */

void spider::pisdf::Vertex::checkTypeConsistency() const {
    switch (subtype_) {
        case VertexType::FORK:
            if (inputEdgeCount() != 1) {
                throwSpiderException("FORK vertex [%s] has more than one input edge.", name_.c_str());
            }
            break;
        case VertexType::JOIN:
            if (outputEdgeCount() != 1) {
                throwSpiderException("JOIN vertex [%s] has more than one output edge.", name_.c_str());
            }
            break;
        case VertexType::TAIL:
            if (outputEdgeCount() != 1) {
                throwSpiderException("TAIL vertex [%s] has more than one output edge.", name_.c_str());
            }
            break;
        case VertexType::HEAD:
            if (outputEdgeCount() != 1) {
                throwSpiderException("HEAD vertex [%s] has more than one output edge.", name_.c_str());
            }
            break;
        case VertexType::DELAY:
            if (inputEdgeCount() != 1) {
                throwSpiderException("DELAY vertex [%s] has more than one input edge.", name_.c_str());
            } else if (outputEdgeCount() != 1) {
                throwSpiderException("DELAY vertex [%s] has more than one output edge.", name_.c_str());
            }
            break;
        case VertexType::REPEAT:
            if (inputEdgeCount() != 1) {
                throwSpiderException("REPEAT vertex [%s] has more than one input edge.", name_.c_str());
            } else if (outputEdgeCount() != 1) {
                throwSpiderException("REPEAT vertex [%s] has more than one output edge.", name_.c_str());
            }
            break;
        case VertexType::DUPLICATE:
            if (inputEdgeCount() != 1) {
                throwSpiderException("DUPLICATE vertex [%s] has more than one input edge.", name_.c_str());
            }
            break;
        case VertexType::INIT:
            if (outputEdgeCount() != 1) {
                throwSpiderException("INIT vertex [%s] has more than one output edge.", name_.c_str());
            } else if (inputEdgeCount()) {
                throwSpiderException("INIT vertex [%s] has at least one input edge.", name_.c_str());
            }
            break;
        case VertexType::END:
            if (inputEdgeCount() != 1) {
                throwSpiderException("END vertex [%s] has more than one input edge.", name_.c_str());
            } else if (outputEdgeCount()) {
                throwSpiderException("END vertex [%s] has at least one output edge.", name_.c_str());
            }
            break;
        default:
            break;
    }
}


/* === Private method(s) === */

spider::pisdf::Edge *spider::pisdf::Vertex::disconnectEdge(spider::vector<Edge *> &edges, size_t ix) {
    auto *&edge = edges.at(ix);
    auto *ret = edge;
    if (edge) {
        edge = nullptr;
    }
    return ret;
}

void spider::pisdf::Vertex::connectEdge(spider::vector<Edge *> &edges, Edge *edge, size_t ix) {
    auto *&current = edges.at(ix);
    if (!current) {
        current = edge;
    } else {
        throwSpiderException("Edge already exists at position: %zu", ix);
    }
}
