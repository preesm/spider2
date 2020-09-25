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
    for (auto &firing : graphHandler->firings()) {
        if (firing.isResolved() && (firing.ix() == SIZE_MAX)) {
            for (const auto &vertex : graphHandler->graph()->vertices()) {
                if (vertex->subtype() != pisdf::VertexType::DELAY) {
                    const auto vertexRV = firing.getRV(vertex.get());
                    for (u32 k = 0u; k < vertexRV; ++k) {
                        if (firing.getTaskIx(vertex.get(), k) == UINT32_MAX) {
                            firing.registerTaskIx(vertex.get(), k, static_cast<u32>(unscheduledVertices_.size()));
                            unscheduledVertices_.push_back({ vertex.get(), &firing, k, vertex->executable(), false });
                        }
                    }
                }
            }
        }
    }

    /* == Schedule actors == */
    auto it = std::begin(unscheduledVertices_);
    while (it != std::end(unscheduledVertices_)) {
        if (it->executable_ && !it->scheduled_) {
            it = evaluate(it);
            if (it == std::end(unscheduledVertices_)) {
                it = std::begin(unscheduledVertices_);
            }
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

spider::sched::SRLessGreedyScheduler::iterator_t
spider::sched::SRLessGreedyScheduler::evaluate(iterator_t it) {
    auto *vertex = it->vertex_;
    /* == add vertex to task vector == */
    auto k = it->firing_;
    if (vertex->inputEdgeCount() > 0u) {
        for (const auto *edge : vertex->inputEdgeVector()) {
            if (!edge->source() || !edge->source()->executable()) {
                it->executable_ = false;
                return it + 1;
            }
            const auto edgeIx = static_cast<u32>(edge->sinkPortIx());
            const auto dep = it->handler_->computeExecDependenciesByEdge(vertex, k, edgeIx);
            auto res = evaluateCurrentDependency(it, dep.setter_);
            if ((res->vertex_ != vertex) || (res->firing_ != k)) {
                return res;
            } else {
                res = evaluateCurrentDependency(it, dep.source_);
                if ((res->vertex_ != vertex) || (res->firing_ != k)) {
                    return res;
                }
            }
        }
    }
    tasks_.emplace_back(make<TaskSRLess>(it->handler_, it->vertex_, k));
    it->scheduled_ = true;
    return it + 1;
}

spider::sched::SRLessGreedyScheduler::iterator_t
spider::sched::SRLessGreedyScheduler::evaluateCurrentDependency(iterator_t it,
                                                                const srless::ExecDependencyInfo &dependencyInfo) {
    if (!dependencyInfo.vertex_) {
        return it;
    }
    const auto *source = dependencyInfo.vertex_;
    if (!source->executable()) {
        it->executable_ = false;
        return it + 1;
    }
    for (auto j = dependencyInfo.firingStart_; j <= dependencyInfo.firingEnd_; ++j) {
        const auto ix = dependencyInfo.handler_->getTaskIx(source, j);
        if (ix < unscheduledVertices_.size()) {
            auto currentIt = std::next(std::begin(unscheduledVertices_), static_cast<long>(ix));
            if ((currentIt->vertex_ == source) && (currentIt->firing_ == j)) {
                if (currentIt->executable_ && !currentIt->scheduled_) {
                    std::swap(*it, *currentIt);
                    currentIt->handler_->registerTaskIx(currentIt->vertex_, currentIt->firing_, ix);
                    const auto itPos = std::distance(std::begin(unscheduledVertices_), it);
                    it->handler_->registerTaskIx(it->vertex_, it->firing_, static_cast<u32>(itPos));
                    return it;
                } else if (!currentIt->executable_) {
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
        it->vertex_->setScheduleTaskIx(static_cast<size_t>(distance));
    }
    return it;
}
