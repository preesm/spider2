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

#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Graph.h>

/* === Static function(s) === */

static spider::pisdf::Edge *disconnectEdge(spider::vector<spider::pisdf::Edge *> &edges, size_t ix) {
    auto *&edge = edges.at(ix);
    spider::pisdf::Edge *ret = edge;
    if (edge) {
        edge = nullptr;
    }
    return ret;
}

static void connectEdge(spider::vector<spider::pisdf::Edge *> &edges, spider::pisdf::Edge *edge, size_t ix) {
    auto *&current = edges.at(ix);
    if (!current) {
        current = edge;
        return;
    }
    throwSpiderException("Edge already exists at position: %zu", ix);
}

/* === Function(s) definition === */

spider::pisdf::Vertex::Vertex(std::string name, size_t edgeINCount, size_t edgeOUTCount) : name_{ std::move(name) } {
    inputEdgeVector_.resize(edgeINCount, nullptr);
    outputEdgeVector_.resize(edgeOUTCount, nullptr);
}

spider::pisdf::Vertex::Vertex(std::string name,
                              size_t edgeINCount,
                              size_t edgeOUTCount,
                              size_t paramINCount,
                              size_t paramOUTCount,
                              const spider::pisdf::Vertex *reference) : name_{ std::move(name) },
                                                                        reference_{ this } {
    inputEdgeVector_.resize(edgeINCount, nullptr);
    outputEdgeVector_.resize(edgeOUTCount, nullptr);
    if (reference) {
        reference_ = reference;
        reference->copyCount_++;
        rtInformation_ = reference->rtInformation_;
    }
    inputParamVector_.reserve(paramINCount);
    outputParamVector_.reserve(paramOUTCount);
}

spider::pisdf::Vertex::Vertex(const Vertex &other) : name_{ other.name_ },
                                                     inputEdgeVector_{ other.inputEdgeVector_.size(), nullptr },
                                                     outputEdgeVector_{ other.outputEdgeVector_.size(), nullptr },
                                                     rtInformation_{ other.rtInformation_ },
                                                     reference_{ &other },
                                                     graph_{ other.graph_ },
                                                     scheduleJobIx_{ other.scheduleJobIx_ },
                                                     instanceValue_{ other.instanceValue_ },
                                                     ix_{ other.ix_ },
                                                     repetitionValue_{ other.repetitionValue_ } {
    other.copyCount_ += 1;
    inputParamVector_.reserve(other.inputParamCount());
    outputParamVector_.reserve(other.outputParamCount());
    refinementParamVector_.reserve(other.refinementParamVector_.size());
}

spider::pisdf::Vertex::~Vertex() noexcept {
    if (copyCount_ && log::enabled()) {
        log::error("Removing vertex [%s] with copies out there.\n", name().c_str());
    }
    this->reference_->copyCount_ -= 1;
}

void spider::pisdf::Vertex::connectInputEdge(Edge *edge, size_t ix) {
    connectEdge(inputEdgeVector_, edge, ix);
}

void spider::pisdf::Vertex::connectOutputEdge(Edge *edge, size_t ix) {
    connectEdge(outputEdgeVector_, edge, ix);
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

void spider::pisdf::Vertex::addInputParameter(std::shared_ptr<Param> param) {
    inputParamVector_.emplace_back(std::move(param));
}

void spider::pisdf::Vertex::addOutputParameter(std::shared_ptr<Param> param) {
    if (subtype() != VertexType::CONFIG) {
        throwSpiderException("[%s] can not have output parameter.", name().c_str());
    }
    outputParamVector_.emplace_back(std::move(param));
}

void spider::pisdf::Vertex::addRefinementParameter(std::shared_ptr<Param> param) {
    refinementParamVector_.emplace_back(std::move(param));
}

const std::string &spider::pisdf::Vertex::name() const {
    return (reference_ == this) ? name_ : reference_->name_;
}

std::string spider::pisdf::Vertex::vertexPath() const {
    if (graph_) {
        return graph_->vertexPath().append(":").append(name_);
    }
    return name_;
}

void spider::pisdf::Vertex::setGraph(Graph *graph) {
    if (graph) {
        graph_ = graph;
    }
}

void spider::pisdf::Vertex::setInstanceValue(size_t value) {
    if (value >= reference_->repetitionValue()) {
        throwSpiderException("invalid instance value for vertex [%s].", name_.c_str());
    }
    instanceValue_ = value;
}
