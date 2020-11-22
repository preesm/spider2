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
#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <graphs/srdag/SRDAGEdge.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <graphs/srdag/SRDAGGraph.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Param.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::srdag::Vertex::Vertex(const pisdf::Vertex *reference,
                              size_t instanceValue,
                              size_t edgeINCount,
                              size_t edgeOUTCount) :
        inputParamVector_{ factory::vector<std::shared_ptr<pisdf::Param>>(StackID::TRANSFO) },
        refinementParamVector_{ factory::vector<std::shared_ptr<pisdf::Param>>(StackID::TRANSFO) },
        outputParamVector_{ factory::vector<std::shared_ptr<pisdf::Param>>(StackID::TRANSFO) },
        reference_{ reference },
        nINEdges_{ static_cast<u32>(edgeINCount) },
        nOUTEdges_{ static_cast<u32>(edgeOUTCount) } {
    if (!reference) {
        throwNullptrException();
    }
    inputEdgeArray_ = spider::make_n<srdag::Edge *, StackID::TRANSFO>(edgeINCount, nullptr);
    outputEdgeArray_ = spider::make_n<srdag::Edge *, StackID::TRANSFO>(edgeOUTCount, nullptr);
    if (instanceValue >= reference_->repetitionValue()) {
        throwSpiderException("invalid instance value for vertex [%s].", name().c_str());
    }
    instanceValue_ = instanceValue;
    scheduleTask_ = spider::make_unique(spider::make<sched::SRDAGTask, StackID::SCHEDULE>(this));
}

spider::srdag::Vertex::~Vertex() {
    deallocate(inputEdgeArray_);
    deallocate(outputEdgeArray_);
}

void spider::srdag::Vertex::connectInputEdge(spider::srdag::Edge *edge, size_t pos) {
    connectEdge(inputEdgeArray_, edge, pos);
}

void spider::srdag::Vertex::connectOutputEdge(spider::srdag::Edge *edge, size_t pos) {
    connectEdge(outputEdgeArray_, edge, pos);
}

spider::srdag::Edge *spider::srdag::Vertex::disconnectInputEdge(size_t ix) {
    auto *edge = disconnectEdge(inputEdgeArray_, ix);
    if (edge) {
        /* == Reset the Edge == */
        edge->setSink(nullptr, SIZE_MAX);
    }
    return edge;
}

spider::srdag::Edge *spider::srdag::Vertex::disconnectOutputEdge(size_t ix) {
    auto *edge = disconnectEdge(outputEdgeArray_, ix);
    if (edge) {
        /* == Reset the Edge == */
        edge->setSource(nullptr, SIZE_MAX);
    }
    return edge;
}

void spider::srdag::Vertex::addInputParameter(std::shared_ptr<pisdf::Param> param) {
    if (reference_->subtype() != pisdf::VertexType::GRAPH) {
        inputParamVector_.emplace_back(std::move(param));
    }
}

void spider::srdag::Vertex::addOutputParameter(std::shared_ptr<pisdf::Param> param) {
    if (reference_->subtype() != pisdf::VertexType::CONFIG) {
        throwSpiderException("Failed to set output parameter [%s] of vertex [%s]: not a config actor.",
                             param->name().c_str(), name().c_str());
    }
    outputParamVector_.emplace_back(std::move(param));
}

void spider::srdag::Vertex::addRefinementParameter(std::shared_ptr<pisdf::Param> param) {
    if (reference_->subtype() != pisdf::VertexType::GRAPH) {
        refinementParamVector_.emplace_back(std::move(param));
    }
}

std::string spider::srdag::Vertex::vertexPath() const {
    if (graph_) {
        return graph_->vertexPath().append(":").append(name());
    }
    return name();
}

std::string spider::srdag::Vertex::name() const {
    return reference_->name() + ":" + std::to_string(instanceValue_);
}

spider::RTInfo *spider::srdag::Vertex::runtimeInformation() const {
    return reference_->runtimeInformation();
}

spider::pisdf::VertexType spider::srdag::Vertex::subtype() const {
    return reference_->hierarchical() ? pisdf::VertexType::NORMAL : reference_->subtype();
}

/* === Private method(s) implementation === */

spider::srdag::Edge *spider::srdag::Vertex::disconnectEdge(srdag::Edge **edges, size_t ix) {
    auto *&edge = edges[ix];
    auto *ret = edge;
    if (edge) {
        edge = nullptr;
    }
    return ret;
}

void spider::srdag::Vertex::connectEdge(srdag::Edge **edges, srdag::Edge *edge, size_t ix) {
    auto *&current = edges[ix];
    if (!current) {
        current = edge;
    } else {
        throwSpiderException("Edge already exists at position: %zu", ix);
    }
}

#endif
