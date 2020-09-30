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
#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <scheduling/scheduler/ListScheduler.h>
#include <scheduling/task/TaskVertex.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Edge.h>


/* === Static variable === */

namespace {
    constexpr auto NON_SCHEDULABLE_LEVEL = -314159265; /* = Value is arbitrary, just needed something unique = */
}

/* === Method(s) implementation === */

spider::sched::ListScheduler::ListScheduler() : Scheduler(),
                                                sortedTaskVector_{ factory::vector<ListTask>(StackID::SCHEDULE) } {

}

void spider::sched::ListScheduler::schedule(const pisdf::Graph *graph) {
    /* == Reserve space for the new ListTasks == */
    tasks_.clear();
    sortedTaskVector_.reserve(graph->vertexCount());

    /* == Reset previous non-schedulable tasks == */
    lastScheduledTask_ = lastSchedulableTask_;
    resetUnScheduledTasks();

    /* == Creates ListTasks == */
    for (const auto &vertex : graph->vertices()) {
        createListTask(vertex.get());
    }

    /* == Compute the schedule level == */
    auto it = std::next(std::begin(sortedTaskVector_), static_cast<long>(lastSchedulableTask_));
    for (; it != std::end(sortedTaskVector_); ++it) {
        computeScheduleLevel(*it, sortedTaskVector_);
    }

    /* == Sort the vector == */
    sortVertices();

    /* == Remove the non-executable hierarchical vertex == */
    const auto nonSchedulableTaskCount = countNonSchedulableTasks();

    /* == Update last schedulable vertex == */
    lastSchedulableTask_ = sortedTaskVector_.size() - nonSchedulableTaskCount;

    /* == Create the list of tasks to be scheduled == */
    for (auto k = lastScheduledTask_; k < lastSchedulableTask_; ++k) {
        tasks_.emplace_back(make<TaskVertex>(sortedTaskVector_[k].vertex_));
        sortedTaskVector_[k].vertex_->setScheduleTaskIx(SIZE_MAX);
    }
}

void spider::sched::ListScheduler::clear() {
    Scheduler::clear();
    sortedTaskVector_.clear();
    lastSchedulableTask_ = 0;
    lastScheduledTask_ = 0;
}

/* === Private method(s) implementation === */

void spider::sched::ListScheduler::resetUnScheduledTasks() {
    auto last = sortedTaskVector_.size();
    for (auto k = lastSchedulableTask_; k < last; ++k) {
        auto *vertex = sortedTaskVector_[k].vertex_;
        vertex->setScheduleTaskIx(k);
    }
}

void spider::sched::ListScheduler::createListTask(pisdf::Vertex *vertex) {
    if (vertex->scheduleTaskIx() != SIZE_MAX) {
        return;
    }
    sortedTaskVector_.push_back({ vertex, -1 });
    vertex->setScheduleTaskIx(sortedTaskVector_.size() - 1);
}

ifast32 spider::sched::ListScheduler::computeScheduleLevel(ListTask &listTask,
                                                           spider::vector<ListTask> &listVertexVector) const {
    const auto *vertex = listTask.vertex_;
    if ((listTask.level_ == NON_SCHEDULABLE_LEVEL) || !vertex->executable()) {
        listTask.level_ = NON_SCHEDULABLE_LEVEL;
        for (auto &edge : vertex->outputEdgeVector()) {
            if (edge->sinkRateValue()) {
                /* == Disable non-null edge == */
                auto &sinkTask = listVertexVector[edge->sink()->scheduleTaskIx()];
                sinkTask.level_ = NON_SCHEDULABLE_LEVEL;
                computeScheduleLevel(sinkTask, listVertexVector);
            }
        }
    } else if (listTask.level_ < 0) {
        const auto *platform = archi::platform();
        ifast32 level = 0;
        for (auto &edge : vertex->outputEdgeVector()) {
            const auto *sink = edge->sink();
            if (sink && sink->executable()) {
                const auto &sinkParams = sink->inputParamVector();
                const auto *sinkRTInfo = sink->runtimeInformation();
                auto minExecutionTime = INT64_MAX;
                for (auto &cluster : platform->clusters()) {
                    if (sinkRTInfo->isClusterMappable(cluster)) {
                        for (const auto &pe : cluster->peArray()) {
                            auto executionTime = sinkRTInfo->timingOnPE(pe, sinkParams);
                            if (!executionTime) {
                                throwSpiderException("Vertex [%s] has null execution time on mappable cluster.",
                                                     vertex->name().c_str());
                            }
                            minExecutionTime = std::min(minExecutionTime, executionTime);
                        }
                    }
                }
                const auto sinkLevel = computeScheduleLevel(listVertexVector[sink->scheduleTaskIx()],
                                                            listVertexVector);
                if (sinkLevel != NON_SCHEDULABLE_LEVEL) {
                    level = std::max(level, sinkLevel + static_cast<ifast32>(minExecutionTime));
                }
            }
        }
        listTask.level_ = level;
    }
    return listTask.level_;
}

void spider::sched::ListScheduler::sortVertices() {
    std::sort(std::next(std::begin(sortedTaskVector_), static_cast<long>(lastSchedulableTask_)),
              std::end(sortedTaskVector_),
              [](const ListTask &A, const ListTask &B) -> bool {
                  const auto diff = A.level_ - B.level_;
                  if (!diff) {
                      const auto *vertexA = A.vertex_;
                      const auto *vertexB = B.vertex_;
                      if (vertexB->reference() == vertexA->reference()) {
                          return vertexA->instanceValue() < vertexB->instanceValue();
                      } else if ((vertexA->subtype() != vertexB->subtype()) &&
                                 ((vertexA->subtype() == pisdf::VertexType::INIT) ||
                                  (vertexB->subtype() == pisdf::VertexType::END))) {
                          return true;
                      }
                      return vertexA->name() > vertexB->name();
                  }
                  return (diff > 0);
              });
}

size_t spider::sched::ListScheduler::countNonSchedulableTasks() {
    auto it = sortedTaskVector_.rbegin();
    for (; (it->level_ == NON_SCHEDULABLE_LEVEL) && (it != sortedTaskVector_.rend()); ++it) {
        auto isExec = it->vertex_->executable();
        if (!isExec) {
            std::swap(*it, sortedTaskVector_.back());
            sortedTaskVector_.pop_back();
        }
        it->level_ = -1;      /* = Reset the schedule level = */
    }
    return static_cast<size_t>(std::distance(sortedTaskVector_.rbegin(), it));
}
#endif