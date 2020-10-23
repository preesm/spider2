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

#include <scheduling/scheduler/srdag-based/ListScheduler.h>
#include <scheduling/task/SRDAGTask.h>
#include <graphs/srdag/SRDAGGraph.h>
#include <graphs/srdag/SRDAGEdge.h>
#include <graphs/srdag/SRDAGVertex.h>


/* === Static variable === */

namespace {
    constexpr auto NON_SCHEDULABLE_LEVEL = 314159265; /* = Value is arbitrary, just needed something unique = */
}

/* === Method(s) implementation === */

spider::sched::ListScheduler::ListScheduler() : Scheduler(),
                                                sortedTaskVector_{ factory::vector<ListTask>(StackID::SCHEDULE) } {

}

spider::vector<spider::srdag::Vertex *> spider::sched::ListScheduler::schedule(const srdag::Graph *graph) {
    /* == Reserve space for the new ListTasks == */
    tasks_.clear();
    size_t count = 0;
    for (const auto &vertex : graph->vertices()) {
        count += vertex->scheduleTaskIx() == SIZE_MAX;
    }
    spider::reserve(sortedTaskVector_, count);
    /* == Reset previous non-schedulable tasks == */
    resetUnScheduledTasks();
    /* == Creates ListTasks == */
    for (const auto &vertex : graph->vertices()) {
        createListTask(vertex.get());
    }
    /* == Compute the schedule level == */
    for (auto &task : sortedTaskVector_) {
        computeScheduleLevel(task);
    }
    /* == Sort the vector == */
    sortVertices();
    /* == Remove the non-executable hierarchical vertex == */
    const auto nonSchedulableTaskCount = countNonSchedulableTasks();
    /* == Update last schedulable vertex == */
    const auto lastSchedulable = sortedTaskVector_.size() - nonSchedulableTaskCount;
    /* == Create the list of tasks to be scheduled == */
    auto result = factory::vector<srdag::Vertex *>(lastSchedulable, StackID::SCHEDULE);
    for (size_t k = 0; k < lastSchedulable; ++k) {
        auto *vertex = sortedTaskVector_[k].vertex_;
        vertex->setScheduleTaskIx(SIZE_MAX);
        result[k] = vertex;
    }
    /* == Remove scheduled vertices == */
    auto it = std::begin(sortedTaskVector_);
    for (auto k = lastSchedulable; k < sortedTaskVector_.size(); ++k) {
        std::swap(sortedTaskVector_[k], *(it++));
    }
    while (sortedTaskVector_.size() != nonSchedulableTaskCount) {
        sortedTaskVector_.pop_back();
    }
    return result;
}

void spider::sched::ListScheduler::clear() {
    Scheduler::clear();
    sortedTaskVector_.clear();
}

/* === Private method(s) implementation === */

void spider::sched::ListScheduler::resetUnScheduledTasks() {
    for (size_t k = 0; k < sortedTaskVector_.size(); ++k) {
        auto *vertex = sortedTaskVector_[k].vertex_;
        vertex->setScheduleTaskIx(k);
    }
}

void spider::sched::ListScheduler::createListTask(srdag::Vertex *vertex) {
    if (vertex->scheduleTaskIx() != SIZE_MAX) {
        return;
    }
    sortedTaskVector_.push_back({ vertex, vertex->executable() ? -1 : NON_SCHEDULABLE_LEVEL });
    vertex->setScheduleTaskIx(sortedTaskVector_.size() - 1);
}

void spider::sched::ListScheduler::recursiveSetNonSchedulable(const srdag::Vertex *vertex) {
    for (const auto *edge : vertex->outputEdges()) {
        if (edge->sinkRateValue()) {
            /* == Disable non-null edge == */
            auto &sinkTask = sortedTaskVector_[edge->sink()->scheduleTaskIx()];
            if (sinkTask.level_ != NON_SCHEDULABLE_LEVEL) {
                sinkTask.level_ = NON_SCHEDULABLE_LEVEL;
                recursiveSetNonSchedulable(sinkTask.vertex_);
            }
        }
    }
}

ifast32 spider::sched::ListScheduler::computeScheduleLevel(ListTask &listTask) {
    const auto *vertex = listTask.vertex_;
    if (listTask.level_ == NON_SCHEDULABLE_LEVEL) {
        recursiveSetNonSchedulable(vertex);
    } else if (listTask.level_ < 0) {
        const auto *platform = archi::platform();
        ifast32 level = 0;
        for (const auto *edge : vertex->inputEdges()) {
            const auto *source = edge->source();
            if (source && source->executable()) {
                const auto &sourceParams = source->inputParamVector();
                const auto *sourceRTInfo = source->runtimeInformation();
                auto minExecutionTime = INT64_MAX;
                for (auto &cluster : platform->clusters()) {
                    if (sourceRTInfo->isClusterMappable(cluster)) {
                        for (const auto &pe : cluster->peArray()) {
                            auto executionTime = sourceRTInfo->timingOnPE(pe, sourceParams);
                            if (!executionTime) {
                                throwSpiderException("Vertex [%s] has null execution time on mappable cluster.",
                                                     vertex->name().c_str());
                            }
                            minExecutionTime = std::min(minExecutionTime, executionTime);
                        }
                    }
                }
                const auto sourceTaskIx = source->scheduleTaskIx();
                if (sourceTaskIx < sortedTaskVector_.size() && sortedTaskVector_[sourceTaskIx].vertex_ == source) {
                    const auto sourceLevel = computeScheduleLevel(sortedTaskVector_[source->scheduleTaskIx()]);
                    if (sourceLevel != NON_SCHEDULABLE_LEVEL) {
                        level = std::max(level, sourceLevel + static_cast<i32>(minExecutionTime));
                    }
                }
            }
        }
        listTask.level_ = level;
    }
    return listTask.level_;
}

void spider::sched::ListScheduler::sortVertices() {
    std::sort(std::begin(sortedTaskVector_), std::end(sortedTaskVector_),
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
                  return diff < 0;
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