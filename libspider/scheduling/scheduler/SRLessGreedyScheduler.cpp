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

void spider::sched::SRLessGreedyScheduler::schedule(const srless::GraphHandler *graphHandler) {
    tasks_.clear();
    for (auto &firing : graphHandler->firings()) {
        if (firing->isResolved() && (firing->ix() == SIZE_MAX)) {
            for (const auto &vertex : graphHandler->graph()->vertices()) {
                if (vertex->scheduleTaskIx() == SIZE_MAX) {
                    const auto vertexRV = firing->getRV(vertex.get());
                    vertex->setScheduleTaskIx(unscheduledVertices_.size());
                    for (size_t k = 0; k < vertexRV; ++k) {
                        unscheduledVertices_.push_back({ vertex.get(), firing.get(), vertex->executable(), false });
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
spider::sched::SRLessGreedyScheduler::evaluate(SRLessGreedyScheduler::iterator_t it) {
    auto *vertex = it->vertex_;
    /* == add vertex to task vector == */
    const auto itFirstFiring = std::next(std::begin(unscheduledVertices_), static_cast<long>(vertex->scheduleTaskIx()));
    auto k = static_cast<u32>(std::distance(itFirstFiring, it));
    const auto dependencies = it->handler_->computeExecDependenciesByFiring(vertex, k);
    if (vertex->inputEdgeCount() > 0u) {
        for (const auto &dep : dependencies) {
            if (!dep.vertex_) {
                continue;
            }
            const auto *source = dep.vertex_;
            if (!source->executable()) {
                it->executable_ = false;
                return it + 1;
            } else if (source->scheduleTaskIx() < unscheduledVertices_.size()) {
                auto sourceIt = std::next(std::begin(unscheduledVertices_),
                                          static_cast<long>(source->scheduleTaskIx()));
                if (sourceIt->vertex_ == source) {
                    for (auto j = dep.firingStart_; j <= dep.firingEnd_; ++j) {
                        auto currentIt = sourceIt + j;
                        if (currentIt->executable_ && !currentIt->scheduled_) {
                            return currentIt;
                        }
                    }
                } else {
                    return it + 1;
                }
            }
        }
    }
    tasks_.emplace_back(make<TaskSRLess>(it->handler_, it->vertex_, static_cast<u32>(k), dependencies));
    it->scheduled_ = true;
    return it + 1;
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
