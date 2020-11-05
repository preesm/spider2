/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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

#include <graphs-tools/transformation/pisdf/GraphAlloc.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Graph.h>
#include <scheduling/task/PiSDFTask.h>

/* === Function(s) definition === */

spider::pisdf::GraphAlloc::GraphAlloc(const Graph *graph) {
    taskIxArray_ = spider::make_unique(make_n<u32 *, StackID::SCHEDULE>(graph->vertexCount(), nullptr));
    tasksArray_ = spider::make_unique(make_n<sched::PiSDFTask *, StackID::SCHEDULE>(graph->vertexCount(), nullptr));
    edgeAllocArray_ = spider::make_unique(make_n<FifoAlloc *, StackID::SCHEDULE>(graph->edgeCount(), nullptr));
}

void spider::pisdf::GraphAlloc::clear(const Graph *graph) {
    for (const auto &vertex : graph->vertices()) {
        deallocate(taskIxArray_[vertex->ix()]);
        destroy(tasksArray_[vertex->ix()]);
    }
    for (const auto &edge : graph->edges()) {
        deallocate(edgeAllocArray_[edge->ix()]);
    }
}

void spider::pisdf::GraphAlloc::reset(const Graph *graph, const u32 *brv) {
    for (const auto &vertex : graph->vertices()) {
        reset(vertex.get(), brv[vertex->ix()]);
    }
}

void spider::pisdf::GraphAlloc::reset(const Vertex *vertex, u32 rv) {
    if (vertex->hierarchical() || vertex->subtype() == VertexType::DELAY) {
        return;
    }
    const auto ix = vertex->ix();
    if (rv != UINT32_MAX) {
        tasksArray_[ix]->reset();
        std::fill(taskIxArray_[ix], taskIxArray_[ix] + rv, UINT32_MAX);
        const auto isSpecial = vertex->subtype() == VertexType::FORK || vertex->subtype() == VertexType::DUPLICATE;
        const auto size = isSpecial ? rv : 1;
        for (const auto *edge : vertex->outputEdges()) {
            for (u32 k = 0; k < size; ++k) {
                setEdgeAddress(SIZE_MAX, edge, k);
                setEdgeOffset(0, edge, k);
            }
        }
    }
}

void spider::pisdf::GraphAlloc::initialize(GraphFiring *handler, const Vertex *vertex, u32 rv) {
    if (vertex->hierarchical() || vertex->subtype() == VertexType::DELAY) {
        return;
    }
    const auto ix = vertex->ix();
    destroy(tasksArray_[ix]);
    tasksArray_[ix] = spider::make<sched::PiSDFTask, StackID::SCHEDULE>(handler, vertex);
    deallocate(taskIxArray_[ix]);
    taskIxArray_[ix] = spider::make_n<u32, StackID::SCHEDULE>(rv, UINT32_MAX);
    const auto isSpecial = vertex->subtype() == VertexType::FORK || vertex->subtype() == VertexType::DUPLICATE;
    const auto size = isSpecial ? rv : 1;
    for (const auto *edge : vertex->outputEdges()) {
        deallocate(edgeAllocArray_[edge->ix()]);
        edgeAllocArray_[edge->ix()] = spider::make_n<FifoAlloc, StackID::SCHEDULE>(size, { SIZE_MAX, 0 });
    }
}

u32 spider::pisdf::GraphAlloc::getTaskIx(const Vertex *vertex, u32 firing) const {
    return taskIxArray_[vertex->ix()][firing];
}

size_t spider::pisdf::GraphAlloc::getEdgeAddress(const Edge *edge, u32 firing) const {
    return edgeAllocArray_[edge->ix()][firing].address_;
}

u32 spider::pisdf::GraphAlloc::getEdgeOffset(const Edge *edge, u32 firing) const {
    return edgeAllocArray_[edge->ix()][firing].offset_;
}

spider::sched::PiSDFTask *spider::pisdf::GraphAlloc::getTask(const Vertex *vertex) {
    return tasksArray_[vertex->ix()];
}

const spider::sched::PiSDFTask *spider::pisdf::GraphAlloc::getTask(const Vertex *vertex) const {
    return tasksArray_[vertex->ix()];
}

void spider::pisdf::GraphAlloc::setTaskIx(const Vertex *vertex, u32 firing, u32 taskIx) {
    taskIxArray_[vertex->ix()][firing] = taskIx;
}

void spider::pisdf::GraphAlloc::setEdgeAddress(size_t value, const Edge *edge, u32 firing) {
    edgeAllocArray_[edge->ix()][firing].address_ = value;
}

void spider::pisdf::GraphAlloc::setEdgeOffset(u32 value, const Edge *edge, u32 firing) {
    edgeAllocArray_[edge->ix()][firing].offset_ = value;
}
