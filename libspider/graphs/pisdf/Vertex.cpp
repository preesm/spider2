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
        nINEdges_{ static_cast<u32>(edgeINCount) },
        nOUTEdges_{ static_cast<u32>(edgeOUTCount) },
        subtype_{ type } {
    inputEdgeArray_.reset(spider::make_n<Edge *, StackID::PISDF>(edgeINCount, nullptr));
    outputEdgeArray_.reset(spider::make_n<Edge *, StackID::PISDF>(edgeOUTCount, nullptr));
    rtInformation_ = spider::make_unique<RTInfo>(StackID::RUNTIME);
    setName(std::move(name));
    checkTypeConsistency();
}

void spider::pisdf::Vertex::connectInputEdge(Edge *edge, size_t pos) {
    if (pos >= nINEdges_) {
        throwSpiderException("trying to connect edge out of bound.");
    }
    connectEdge(inputEdgeArray_.get(), edge, pos);
}

void spider::pisdf::Vertex::connectOutputEdge(Edge *edge, size_t pos) {
    if (pos >= nOUTEdges_) {
        throwSpiderException("trying to connect edge out of bound.");
    }
    connectEdge(outputEdgeArray_.get(), edge, pos);
}

spider::pisdf::Edge *spider::pisdf::Vertex::disconnectInputEdge(size_t ix) {
    auto *edge = disconnectEdge(inputEdgeArray_.get(), ix);
    if (edge) {
        /* == Reset the Edge == */
        edge->setSink(nullptr, SIZE_MAX, Expression());
    }
    return edge;
}

spider::pisdf::Edge *spider::pisdf::Vertex::disconnectOutputEdge(size_t ix) {
    auto *edge = disconnectEdge(outputEdgeArray_.get(), ix);
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

void spider::pisdf::Vertex::addInputParameter(const std::shared_ptr<Param> &param) {
    if (subtype_ != VertexType::GRAPH) {
        auto *tmp = spider::make_n<u32, StackID::PISDF>(nINParams_ + 1);
        std::move(inputParamArray_.get(), inputParamArray_.get() + nINParams_, tmp);
        inputParamArray_.reset(tmp);
        inputParamArray_[nINParams_++] = static_cast<u32>(param->ix());
    }
}

void spider::pisdf::Vertex::addOutputParameter(const std::shared_ptr<Param> &param) {
    if (subtype() != VertexType::CONFIG) {
        throwSpiderException("Failed to set output parameter [%s] of vertex [%s]: not a config actor.",
                             param->name().c_str(), name().c_str());
    }
    auto *tmp = spider::make_n<u32, StackID::PISDF>(nOUTParams_ + 1);
    std::move(outputParamArray_.get(), outputParamArray_.get() + nOUTParams_, tmp);
    outputParamArray_.reset(tmp);
    outputParamArray_[nOUTParams_++] = static_cast<u32>(param->ix());
}

void spider::pisdf::Vertex::addRefinementParameter(const std::shared_ptr<Param> &param) {
    if (subtype_ != VertexType::GRAPH) {
        auto *tmp = spider::make_n<u32, StackID::PISDF>(nRefinementParams_ + 1);
        std::move(refinementParamArray_.get(), refinementParamArray_.get() + nRefinementParams_, tmp);
        refinementParamArray_.reset(tmp);
        refinementParamArray_[nRefinementParams_++] = static_cast<u32>(param->ix());
    }
}

std::string spider::pisdf::Vertex::vertexPath() const {
    if (graph_) {
        return graph_->vertexPath().append(":").append(name_.get());
    }
    return name_.get();
}

void spider::pisdf::Vertex::setRepetitionValue(uint32_t value) {
    switch (subtype_) {
        case VertexType::CONFIG:
        case VertexType::DELAY:
        case VertexType::EXTERN_IN:
        case VertexType::EXTERN_OUT:
            if (value > 1) {
                throwSpiderException("Vertex [%s] can not have repetition value greater than 1.", name_.get());
            }
            repetitionValue_ = value;
            break;
        default:
            repetitionValue_ = value;
            break;
    }
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
                throwSpiderException("FORK vertex [%s] has more than one input edge.", name_.get());
            }
            break;
        case VertexType::JOIN:
            if (outputEdgeCount() != 1) {
                throwSpiderException("JOIN vertex [%s] has more than one output edge.", name_.get());
            }
            break;
        case VertexType::TAIL:
            if (outputEdgeCount() != 1) {
                throwSpiderException("TAIL vertex [%s] has more than one output edge.", name_.get());
            }
            break;
        case VertexType::HEAD:
            if (outputEdgeCount() != 1) {
                throwSpiderException("HEAD vertex [%s] has more than one output edge.", name_.get());
            }
            break;
        case VertexType::DELAY:
            if (inputEdgeCount() != 1) {
                throwSpiderException("DELAY vertex [%s] has more than one input edge.", name_.get());
            } else if (outputEdgeCount() != 1) {
                throwSpiderException("DELAY vertex [%s] has more than one output edge.", name_.get());
            }
            break;
        case VertexType::REPEAT:
            if (inputEdgeCount() != 1) {
                throwSpiderException("REPEAT vertex [%s] has more than one input edge.", name_.get());
            } else if (outputEdgeCount() != 1) {
                throwSpiderException("REPEAT vertex [%s] has more than one output edge.", name_.get());
            }
            break;
        case VertexType::DUPLICATE:
            if (inputEdgeCount() != 1) {
                throwSpiderException("DUPLICATE vertex [%s] has more than one input edge.", name_.get());
            }
            break;
        case VertexType::INIT:
            if (outputEdgeCount() != 1) {
                throwSpiderException("INIT vertex [%s] has more than one output edge.", name_.get());
            } else if (inputEdgeCount()) {
                throwSpiderException("INIT vertex [%s] has at least one input edge.", name_.get());
            }
            break;
        case VertexType::END:
            if (inputEdgeCount() != 1) {
                throwSpiderException("END vertex [%s] has more than one input edge.", name_.get());
            } else if (outputEdgeCount()) {
                throwSpiderException("END vertex [%s] has at least one output edge.", name_.get());
            }
            break;
        default:
            break;
    }
}

void spider::pisdf::Vertex::setName(std::string name) {
    const auto size = name.size();
    name_.reset(spider::make_n<char, StackID::PISDF>(size + 1));
    std::move(std::begin(name), std::end(name), name_.get());
    name_[size - 1] = '\0';
}


/* === Private method(s) === */

spider::pisdf::Edge *spider::pisdf::Vertex::disconnectEdge(Edge **edges, size_t ix) {
    auto *&edge = edges[ix];
    auto *ret = edge;
    if (edge) {
        edge = nullptr;
    }
    return ret;
}

void spider::pisdf::Vertex::connectEdge(Edge **edges, Edge *edge, size_t ix) {
    auto *&current = edges[ix];
    if (!current) {
        current = edge;
    } else {
        throwSpiderException("Edge already exists at position: %zu", ix);
    }
}
