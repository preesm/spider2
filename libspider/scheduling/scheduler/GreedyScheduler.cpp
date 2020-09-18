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

#include <scheduling/scheduler/GreedyScheduler.h>
#include <scheduling/task/TaskVertex.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Vertex.h>

/* === Static function === */

/* === Method(s) implementation === */

void spider::sched::GreedyScheduler::schedule(const pisdf::Graph *graph) {
    tasks_.clear();
    unscheduledVertices_.reserve(graph->vertexCount());

    /* == Reset previous non-schedulable tasks == */
    resetUnScheduledTasks();

    /* == Generate the list of vertex to be scheduled == */
    for (const auto &vertex : graph->vertices()) {
        if (vertex->scheduleTaskIx() == SIZE_MAX) {
            vertex->setScheduleTaskIx(unscheduledVertices_.size());
            unscheduledVertices_.push_back({ vertex.get(), vertex->executable() });
        }
    }

    /* == Schedule actors == */
    auto it = std::begin(unscheduledVertices_);
    while (it != std::end(unscheduledVertices_)) {
        if (it->executable_) {
            it = evaluate(it);
        } else {
            if (!it->vertex_->executable()) {
                /* == Recursively set all output as non exec == */
                setSinksAsNonExecutable(*it);
            }
            it++;
        }
    }

    /* == Remove non executable vertices == */
    removeNonExecutableVertices();
}

void spider::sched::GreedyScheduler::clear() {
    Scheduler::clear();
    unscheduledVertices_.clear();
}

/* === Private method(s) implementation === */

void spider::sched::GreedyScheduler::setSinksAsNonExecutable(ScheduleVertex &scheduleVertex) {
    scheduleVertex.executable_ = false;
    const auto *vertex = scheduleVertex.vertex_;
    for (const auto *edge : vertex->outputEdgeVector()) {
        const auto *sink = edge->sink();
        if (edge->sinkRateValue()) {
            setSinksAsNonExecutable(unscheduledVertices_[sink->scheduleTaskIx()]);
        }
    }
}

void spider::sched::GreedyScheduler::resetUnScheduledTasks() {
    auto last = unscheduledVertices_.size();
    for (size_t k = 0u; k < last; ++k) {
        auto *vertex = unscheduledVertices_[k].vertex_;
        vertex->setScheduleTaskIx(k);
        unscheduledVertices_[k].executable_ = true;
    }
}

spider::sched::GreedyScheduler::iterator_t
spider::sched::GreedyScheduler::evaluate(iterator_t it) {
    if (it->vertex_->inputEdgeCount() > 0u) {
        for (const auto *edge : it->vertex_->inputEdgeVector()) {
            const auto *source = edge->source();
            if (!edge->sourceRateValue()) {
                continue;
            }
            if (!source || !source->executable()) {
                setSinksAsNonExecutable(*it);
                return it + 1;
            } else if (source->scheduleTaskIx() < unscheduledVertices_.size()) {
                auto &srcSchedVertex = unscheduledVertices_[source->scheduleTaskIx()];
                if (srcSchedVertex.vertex_ == source) {
                    if (srcSchedVertex.executable_) {
                        std::swap(*it, srcSchedVertex);
                        srcSchedVertex.vertex_->setScheduleTaskIx(it->vertex_->scheduleTaskIx());
                        const auto itPos = std::distance(std::begin(unscheduledVertices_), it);
                        it->vertex_->setScheduleTaskIx(static_cast<size_t>(itPos));
                        return it;
                    } else {
                        setSinksAsNonExecutable(srcSchedVertex);
                        return it + 1;
                    }
                }
            }
        }
    }
    /* == add vertex to task vector == */
    tasks_.emplace_back(make<TaskVertex>(it->vertex_));
    it->vertex_->setScheduleTaskIx(SIZE_MAX);
    return removeAndSwap(it);
}

spider::sched::GreedyScheduler::iterator_t
spider::sched::GreedyScheduler::removeAndSwap(iterator_t it) {
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

void spider::sched::GreedyScheduler::removeNonExecutableVertices() {
    auto it = std::begin(unscheduledVertices_);
    while (it != std::end(unscheduledVertices_)) {
        if (!it->vertex_->executable()) {
            /* == Distinguish between intrinsically non executable vertices from the one deriving from them == */
            const auto pos = std::distance(std::begin(unscheduledVertices_), it);
            std::swap(*it, unscheduledVertices_.back());
            unscheduledVertices_.pop_back();
            it = std::next(std::begin(unscheduledVertices_), pos);
        } else {
            it++;
        }
    }
}
