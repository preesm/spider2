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

#include <scheduling/scheduler/SRLessGreedyScheduler.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs-tools/transformation/srless/GraphHandler.h>
#include <graphs-tools/transformation/srless/FiringHandler.h>
#include <scheduling/task/TaskSRLess.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::SRLessGreedyScheduler::SRLessGreedyScheduler() :
        Scheduler(),
        unscheduledVertices_{ factory::vector<ScheduleVertex>(StackID::SCHEDULE) } {

}

void spider::sched::SRLessGreedyScheduler::schedule(srless::GraphHandler *graphHandler) {
    tasks_.clear();
    unscheduledVertices_.reserve(graphHandler->graph()->vertexCount());
    /* == add vertices == */
    recursiveAddVertices(graphHandler);

    /* == Schedule actors == */
    auto it = std::begin(unscheduledVertices_);
    while (it != std::end(unscheduledVertices_)) {
        if (it->executable_) {
            it = evaluate(it);
        } else {
            it++;
        }
    }
}

void spider::sched::SRLessGreedyScheduler::clear() {
    Scheduler::clear();
    unscheduledVertices_.clear();
}

/* === Private method(s) implementation === */

void spider::sched::SRLessGreedyScheduler::recursiveAddVertices(spider::srless::GraphHandler *graphHandler) {
    for (auto &firing : graphHandler->firings()) {
        if (firing.isResolved()) {
            for (const auto &vertex : graphHandler->graph()->vertices()) {
                if (vertex->subtype() != spider::pisdf::VertexType::DELAY) {
                    const auto vertexRV = firing.getRV(vertex.get());
                    for (u32 k = 0u; k < vertexRV; ++k) {
                        if (firing.getTaskIx(vertex.get(), k) == UINT32_MAX) {
                            firing.registerTaskIx(vertex.get(), k, static_cast<u32>(this->unscheduledVertices_.size()));
                            this->unscheduledVertices_.push_back(
                                    { vertex.get(), &firing, k, vertex->executable(), false });
                        }
                    }
                }
            }
        }
        for (auto *child : firing.children()) {
            recursiveAddVertices(child);
        }
    }
}

spider::sched::SRLessGreedyScheduler::iterator_t spider::sched::SRLessGreedyScheduler::evaluate(iterator_t it) {
    auto current = *it;
    if (it->vertex_->inputEdgeCount() > 0u) {
        for (u32 i = 0; i < static_cast<u32>(it->vertex_->inputEdgeCount()); ++i) {
            const auto dep = it->handler_->computeExecDependenciesByEdge(it->vertex_, it->firing_, i);
            auto res = evaluateCurrentDependency(it, dep.first_);
            if (*(res) != current) {
                return res;
            } else if (*(res = evaluateCurrentDependency(it, dep.second_)) != current) {
                return res;
            }
        }
    }
    /* == add vertex to task vector == */
    tasks_.emplace_back(make<TaskSRLess>(it->handler_, it->vertex_, it->firing_));
    it->handler_->registerTaskIx(it->vertex_, it->firing_, UINT32_MAX);
    return removeAndSwap(it);
}

spider::sched::SRLessGreedyScheduler::iterator_t
spider::sched::SRLessGreedyScheduler::evaluateCurrentDependency(iterator_t it,
                                                                const srless::ExecDependencyInfo &dependencyInfo) {
    if (!dependencyInfo.vertex_ || !dependencyInfo.rate_) {
        return it;
    }
    const auto *source = dependencyInfo.vertex_;
    for (auto k = dependencyInfo.firingStart_; k <= dependencyInfo.firingEnd_; ++k) {
        const auto ix = dependencyInfo.handler_->getTaskIx(source, k);
        if (ix < unscheduledVertices_.size()) {
            auto &srcSchedVertex = unscheduledVertices_[ix];
            if (srcSchedVertex.vertex_ == source) {
                if (source->hierarchical()) {
                    const auto *graph = source->convertTo<pisdf::Graph>();
                    const auto *graphFiring = srcSchedVertex.handler_->getChildFiring(graph, srcSchedVertex.firing_);
                    if (graphFiring->isResolved()) {
                        const auto *interface = graph->outputInterface(dependencyInfo.edgeIx_);
                        const auto dep = graphFiring->computeExecDependenciesByEdge(interface, 0u, 0u);
                        auto res = evaluateCurrentDependency(it, dep.first_);
                        if ((*res) != (*it)) {
                            return res;
                        }
                    } else {
                        it->executable_ = false;
                        return it + 1;
                    }
                } else if (srcSchedVertex.executable_) {
                    const auto distance = std::distance(std::begin(unscheduledVertices_), it);
                    std::swap(*it, srcSchedVertex);
                    srcSchedVertex.handler_->registerTaskIx(srcSchedVertex.vertex_, srcSchedVertex.firing_, ix);
                    it = std::next(std::begin(unscheduledVertices_), distance);
                    it->handler_->registerTaskIx(it->vertex_, it->firing_, static_cast<u32>(distance));
                    return it;
                } else {
                    it->executable_ = false;
                    return it + 1;
                }
            }
        }
    }
    return it;
}

spider::sched::SRLessGreedyScheduler::iterator_t
spider::sched::SRLessGreedyScheduler::removeAndSwap(iterator_t it) {
    const auto distance = std::distance(std::begin(unscheduledVertices_), it);
    /* == Swap and pop == */
    out_of_order_erase(unscheduledVertices_, it);
    /* == update value of swapped element == */
    it = std::next(std::begin(unscheduledVertices_), distance);
    if (it != std::end(unscheduledVertices_)) {
        it->handler_->registerTaskIx(it->vertex_, it->firing_, static_cast<u32>(distance));
    }
    return it;
}

spider::sched::SRLessGreedyScheduler::iterator_t
spider::sched::SRLessGreedyScheduler::evaluateHierarchical(iterator_t it, const pisdf::Graph *graph, u32 edgeIx) {
    const auto *graphFiring = it->handler_->getChildFiring(graph, it->firing_);
    if (graphFiring->isResolved()) {
        const auto *interface = graph->outputInterface(edgeIx);
        const auto dep = graphFiring->computeExecDependenciesByEdge(interface, 0u, 0u);
        const auto *vertex = it->vertex_;
        const auto k = it->firing_;
        auto res = evaluateCurrentDependency(it, dep.first_);
        if ((res->vertex_ != vertex) || (res->firing_ != k)) {
            return res;
        }
    } else {
        it->executable_ = false;
        return it + 1;
    }
    return it;
}
