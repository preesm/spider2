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
#include <runtime/special-kernels/specialKernels.h>
#include <graphs-tools/exporter/SRDAGDOTExporter.h>

/* === Static function === */

/* === Method(s) implementation === */

void spider::srdag::Graph::clear() {
    edgeVector_.clear();
    vertexVector_.clear();
    specialVertexVector_.clear();
}

void spider::srdag::Graph::addVertex(spider::srdag::Vertex *vertex) {
    if (!vertex) {
        return;
    }
    vertex->setIx(vertexVector_.size());
    vertex->setGraph(static_cast<Graph *>(this));
    vertexVector_.emplace_back(vertex);
}

void spider::srdag::Graph::removeVertex(spider::srdag::Vertex *vertex) {
    if (!vertex) {
        return;
    }
    /* == Assert that vertex is part of the edgeVector_ == */
    auto ix = vertex->ix();
    if (ix >= vertexVector_.size()) {
        throwSpiderException("Trying to remove an element not from this graph.");
    } else if (vertexVector_[ix].get() != vertex) {
        throwSpiderException("Different element in ix position. Expected: %s -- Got: %s", vertex->name().c_str(),
                             vertexVector_[ix]->name().c_str());
    }
    /* == Reset vertex input edges == */
    for (auto &edge : vertex->inputEdges()) {
        if (edge) {
            edge->setSink(nullptr, SIZE_MAX);
        }
    }
    /* == Reset vertex output edges == */
    for (auto &edge : vertex->outputEdges()) {
        if (edge) {
            edge->setSource(nullptr, SIZE_MAX);
        }
    }
    /* == swap and destroy the element == */
    if (vertexVector_.back()) {
        vertexVector_.back()->setIx(ix);
    }
    out_of_order_erase(vertexVector_, ix);
}

void spider::srdag::Graph::addEdge(spider::srdag::Edge *edge) {
    if (!edge) {
        return;
    }
    edge->setIx(edgeVector_.size());
    edgeVector_.emplace_back(edge);
}

void spider::srdag::Graph::removeEdge(spider::srdag::Edge *edge) {
    if (!edge) {
        return;
    }
    /* == Assert that edge is part of the edgeVector_ == */
    auto ix = edge->ix();
    if (ix >= edgeVector_.size()) {
        throwSpiderException("Trying to remove an element not from this graph.");
    } else if (edgeVector_[ix].get() != edge) {
        throwSpiderException("Different element in ix position. Expected: %s -- Got: %s", edge->name().c_str(),
                             edgeVector_[ix]->name().c_str());
    }
    /* == Reset edge source and sink == */
    edge->setSource(nullptr, SIZE_MAX);
    edge->setSink(nullptr, SIZE_MAX);
    /* == swap and destroy the element == */
    if (edgeVector_.back()) {
        edgeVector_.back()->setIx(ix);
    }
    out_of_order_erase(edgeVector_, ix);
}

spider::srdag::Edge *
spider::srdag::Graph::createEdge(srdag::Vertex *source, size_t srcIx, srdag::Vertex *sink, size_t snkIx, i64 rate) {
    srdag::Edge *edge;
    if (source && source->outputEdge(srcIx)) {
        edge = source->outputEdge(srcIx);
        edge->setSink(sink, snkIx);
    } else if (sink && sink->inputEdge(snkIx)) {
        edge = sink->inputEdge(snkIx);
        edge->setSource(source, srcIx);
    } else {
        edge = make<srdag::Edge, StackID::TRANSFO>(source, srcIx, sink, snkIx, rate);
        addEdge(edge);
    }
    return edge;
}

spider::srdag::Vertex *spider::srdag::Graph::createDuplicateVertex(std::string name, size_t edgeOUTCount) {
    auto *vertex = make<pisdf::Vertex, StackID::TRANSFO>(pisdf::VertexType::DUPLICATE, std::move(name), 1u);
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(rt::DUPLICATE_KERNEL_IX);
    specialVertexVector_.emplace_back(vertex);
    auto *srVertex = make<srdag::Vertex, StackID::TRANSFO>(vertex, 0u, 1u, edgeOUTCount);
    addVertex(srVertex);
    return srVertex;
}

spider::srdag::Vertex *spider::srdag::Graph::createForkVertex(std::string name, size_t edgeOUTCount) {
    auto *vertex = make<pisdf::Vertex, StackID::TRANSFO>(pisdf::VertexType::FORK, std::move(name), 1u);
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(rt::FORK_KERNEL_IX);
    specialVertexVector_.emplace_back(vertex);
    auto *srVertex = make<srdag::Vertex, StackID::TRANSFO>(vertex, 0u, 1u, edgeOUTCount);
    addVertex(srVertex);
    return srVertex;
}

spider::srdag::Vertex *spider::srdag::Graph::createJoinVertex(std::string name, size_t edgeINCount) {
    auto *vertex = make<pisdf::Vertex, StackID::TRANSFO>(pisdf::VertexType::JOIN, std::move(name), 0u, 1u);
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(rt::JOIN_KERNEL_IX);
    specialVertexVector_.emplace_back(vertex);
    auto *srVertex = make<srdag::Vertex, StackID::TRANSFO>(vertex, 0u, edgeINCount, 1u);
    addVertex(srVertex);
    return srVertex;
}

spider::srdag::Vertex *
spider::srdag::Graph::createVertex(std::string name, size_t edgeINCount, size_t edgeOUTCount) {
    auto *vertex = make<pisdf::Vertex, StackID::TRANSFO>(pisdf::VertexType::NORMAL, std::move(name));
    specialVertexVector_.emplace_back(vertex);
    auto *srVertex = make<srdag::Vertex, StackID::TRANSFO>(vertex, 0u, edgeINCount, edgeOUTCount);
    addVertex(srVertex);
    return srVertex;
}

spider::srdag::Vertex *
spider::srdag::Graph::createVoidVertex(std::string name, size_t edgeINCount, size_t edgeOUTCount) {
    auto *vertex = make<pisdf::Vertex, StackID::TRANSFO>(pisdf::VertexType::NORMAL, std::move(name));
    specialVertexVector_.emplace_back(vertex);
    auto *srVertex = make<srdag::Vertex, StackID::TRANSFO>(vertex, 0u, edgeINCount, edgeOUTCount);
    srVertex->setExecutable(false);
    addVertex(srVertex);
    return srVertex;
}

spider::srdag::Vertex *spider::srdag::Graph::createRepeatVertex(std::string name) {
    auto *vertex = make<pisdf::Vertex, StackID::TRANSFO>(pisdf::VertexType::REPEAT, std::move(name), 1u, 1u);
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(rt::REPEAT_KERNEL_IX);
    specialVertexVector_.emplace_back(vertex);
    auto *srVertex = make<srdag::Vertex, StackID::TRANSFO>(vertex, 0u, 1u, 1u);
    addVertex(srVertex);
    return srVertex;
}

spider::srdag::Vertex *spider::srdag::Graph::createTailVertex(std::string name, size_t edgeINCount) {
    auto *vertex = make<pisdf::Vertex, StackID::TRANSFO>(pisdf::VertexType::TAIL, std::move(name), 0u, 1u);
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(rt::TAIL_KERNEL_IX);
    specialVertexVector_.emplace_back(vertex);
    auto *srVertex = make<srdag::Vertex, StackID::TRANSFO>(vertex, 0u, edgeINCount, 1u);
    addVertex(srVertex);
    return srVertex;
}

spider::srdag::Vertex *spider::srdag::Graph::createHeadVertex(std::string name, size_t edgeINCount) {
    auto *vertex = make<pisdf::Vertex, StackID::TRANSFO>(pisdf::VertexType::HEAD, std::move(name), 0u, 1u);
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(rt::HEAD_KERNEL_IX);
    specialVertexVector_.emplace_back(vertex);
    auto *srVertex = make<srdag::Vertex, StackID::TRANSFO>(vertex, 0u, edgeINCount, 1u);
    addVertex(srVertex);
    return srVertex;
}

spider::srdag::Vertex *spider::srdag::Graph::createInitVertex(std::string name) {
    auto *vertex = make<pisdf::Vertex, StackID::TRANSFO>(pisdf::VertexType::INIT, std::move(name), 0u, 1u);
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(rt::INIT_KERNEL_IX);
    specialVertexVector_.emplace_back(vertex);
    auto *srVertex = make<srdag::Vertex, StackID::TRANSFO>(vertex, 0u, 0u, 1u);
    addVertex(srVertex);
    return srVertex;
}

spider::srdag::Vertex *spider::srdag::Graph::createEndVertex(std::string name) {
    auto *vertex = make<pisdf::Vertex, StackID::TRANSFO>(pisdf::VertexType::END, std::move(name), 1u);
    auto *runtimeInfo = vertex->runtimeInformation();
    runtimeInfo->setKernelIx(rt::END_KERNEL_IX);
    specialVertexVector_.emplace_back(vertex);
    auto *srVertex = make<srdag::Vertex, StackID::TRANSFO>(vertex, 0u, 1u);
    addVertex(srVertex);
    return srVertex;
}

#ifndef _NO_BUILD_GRAPH_EXPORTER

void spider::srdag::Graph::exportToDOT(const std::string &path) const {
    auto exporter = pisdf::SRDAGDOTExporter(const_cast<Graph *>(this));
    exporter.printFromPath(path);
}

#endif

#endif
