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

#include <scheduling/scheduler/SRLessListScheduler.h>
#include <scheduling/task/TaskSRLess.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs-tools/transformation/srless/GraphHandler.h>
#include <graphs-tools/transformation/srless/GraphFiring.h>
#include <graphs-tools/numerical/dependencies.h>

/* === Static function === */

namespace {
    constexpr auto NON_SCHEDULABLE_LEVEL = -314159265; /* = Value is arbitrary, just needed something unique = */
}

/* === Method(s) implementation === */

spider::sched::SRLessListScheduler::SRLessListScheduler() :
        Scheduler(),
        sortedTaskVector_{ factory::vector<ListTask>(StackID::SCHEDULE) } {

}

void spider::sched::SRLessListScheduler::schedule(srless::GraphHandler *graphHandler) {
    /* == Reserve space for the new ListTasks == */
    tasks_.clear();

    /* == Reset previous non-schedulable tasks == */
    lastScheduledTask_ = lastSchedulableTask_;
    resetUnScheduledTasks();

    /* == Creates ListTasks == */
    recursiveAddVertices(graphHandler);

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
        const auto &task = sortedTaskVector_[k];
        const auto depInfo = countDependenciesAndMergedFifos(task);
        tasks_.emplace_back(make<TaskSRLess>(task.handler_, task.vertex_, task.firing_, depInfo.first, depInfo.second));
        sortedTaskVector_[k].vertex_->setScheduleTaskIx(SIZE_MAX);
    }
}

void spider::sched::SRLessListScheduler::clear() {
    Scheduler::clear();
    sortedTaskVector_.clear();
    lastSchedulableTask_ = 0;
    lastScheduledTask_ = 0;
}

/* === Private method(s) implementation === */

void spider::sched::SRLessListScheduler::resetUnScheduledTasks() {
    auto last = sortedTaskVector_.size();
    for (auto k = lastSchedulableTask_; k < last; ++k) {
        auto &listTask = sortedTaskVector_[k];
        listTask.handler_->registerTaskIx(listTask.vertex_, listTask.firing_, static_cast<u32>(k));
    }
}

void spider::sched::SRLessListScheduler::recursiveAddVertices(srless::GraphHandler *graphHandler) {
    for (auto &firingHandler : graphHandler->firings()) {
        if (firingHandler->isResolved()) {
            for (const auto &vertex : graphHandler->graph()->vertices()) {
                if (vertex->subtype() != spider::pisdf::VertexType::DELAY) {
                    const auto vertexRV = firingHandler->getRV(vertex.get());
                    for (u32 k = 0u; k < vertexRV; ++k) {
                        createListTask(vertex.get(), k, firingHandler);
                    }
                }
            }
            for (auto *child : firingHandler->children()) {
                recursiveAddVertices(child);
            }
        } else {
            const auto *graph = graphHandler->graph();
            const auto *handler = graphHandler->handler();
            const auto firing = firingHandler->firingValue();
            recursiveSetNonSchedulable(graph, firing, handler);
        }
    }
}

void spider::sched::SRLessListScheduler::createListTask(pisdf::Vertex *vertex,
                                                        u32 firing,
                                                        srless::GraphFiring *handler) {
    const auto vertexTaskIx = handler->getTaskIx(vertex, firing);
    if (vertexTaskIx == UINT32_MAX && vertex->executable()) {
        sortedTaskVector_.push_back({ vertex, handler, -1, firing });
        handler->registerTaskIx(vertex, firing, static_cast<u32>(sortedTaskVector_.size() - 1));
    }
}

void spider::sched::SRLessListScheduler::recursiveSetNonSchedulable(const pisdf::Vertex *vertex,
                                                                    u32 firing,
                                                                    const srless::GraphFiring *handler) {
    for (const auto *edge : vertex->outputEdgeVector()) {
        const auto deps = pisdf::computeConsDependency(vertex, firing, edge->sourcePortIx(), handler);
        for (const auto &dep : deps) {
            if (dep.rate_ < 0) {
                continue;
            }
            /* == Disable non-null edge == */
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto ix = dep.handler_->getTaskIx(dep.vertex_, k);
                auto &sinkTask = sortedTaskVector_[ix];
                sinkTask.level_ = NON_SCHEDULABLE_LEVEL;
                recursiveSetNonSchedulable(dep.vertex_, k, dep.handler_);
            }
        }
    }
}

ifast32 spider::sched::SRLessListScheduler::computeScheduleLevel(ListTask &listTask,
                                                                 spider::vector<ListTask> &listVertexVector) {
    const auto *vertex = listTask.vertex_;
    const auto *handler = listTask.handler_;
    const auto firing = listTask.firing_;
    if ((listTask.level_ == NON_SCHEDULABLE_LEVEL) || !vertex->executable()) {
        listTask.level_ = NON_SCHEDULABLE_LEVEL;
        recursiveSetNonSchedulable(vertex, firing, handler);
    } else if (listTask.level_ < 0) {
        const auto *platform = archi::platform();
        ifast32 level = 0;
        for (const auto *edge : vertex->outputEdgeVector()) {
            const auto deps = pisdf::computeConsDependency(vertex, firing, edge->sourcePortIx(), handler);
            for (const auto &dep : deps) {
                const auto *sink = dep.vertex_;
                if (sink && dep.rate_ >= 0) {
                    const auto *sinkRTInfo = sink->runtimeInformation();
                    for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                        auto minExecutionTime = INT64_MAX;
                        for (auto &cluster : platform->clusters()) {
                            if (sinkRTInfo->isClusterMappable(cluster)) {
                                for (const auto &pe : cluster->peArray()) {
                                    auto executionTime = sinkRTInfo->timingOnPE(pe, dep.handler_->getParams());
                                    if (!executionTime) {
                                        throwSpiderException(
                                                "Vertex [%s:%u] has null execution time on mappable cluster.",
                                                sink->name().c_str(), k);
                                    }
                                    minExecutionTime = std::min(minExecutionTime, executionTime);
                                }
                            }
                        }
                        const auto sinkTaskIx = dep.handler_->getTaskIx(sink, k);
                        const auto sinkLevel = computeScheduleLevel(listVertexVector[sinkTaskIx], listVertexVector);
                        if (sinkLevel != NON_SCHEDULABLE_LEVEL) {
                            level = std::max(level, sinkLevel + static_cast<ifast32>(minExecutionTime));
                        }
                    }
                } else if (dep.rate_ < 0) {
                    listTask.level_ = NON_SCHEDULABLE_LEVEL;
                    recursiveSetNonSchedulable(vertex, firing, handler);
                    return listTask.level_;
                }
            }
        }
        listTask.level_ = level;
    }
    return listTask.level_;
}

void spider::sched::SRLessListScheduler::sortVertices() {
    std::sort(std::next(std::begin(sortedTaskVector_), static_cast<long>(lastSchedulableTask_)),
              std::end(sortedTaskVector_),
              [](const ListTask &A, const ListTask &B) -> bool {
                  const auto diff = A.level_ - B.level_;
                  if (!diff) {
                      const auto *vertexA = A.vertex_;
                      const auto *vertexB = B.vertex_;
                      if (vertexB->reference() == vertexA->reference()) {
                          auto firingA = A.firing_;
                          auto firingB = B.firing_;
                          const auto *handlerA = A.handler_;
                          const auto *handlerB = B.handler_;
                          while ((handlerA && handlerB) && (firingA == firingB)) {
                              firingA = handlerA->firingValue();
                              firingB = handlerB->firingValue();
                              handlerA = handlerA->getParent()->handler();
                              handlerB = handlerB->getParent()->handler();
                          }
                          return firingA < firingB;
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

size_t spider::sched::SRLessListScheduler::countNonSchedulableTasks() {
    auto it = sortedTaskVector_.rbegin();
    size_t count{ };
    for (; (it->level_ == NON_SCHEDULABLE_LEVEL) && (it != sortedTaskVector_.rend()); ++it) {
        count++;
        it->level_ = -1;      /* = Reset the schedule level = */
    }
    return count;
}

std::pair<u32, u32> spider::sched::SRLessListScheduler::countDependenciesAndMergedFifos(const ListTask &task) const {
    u32 depCount{ };
    u32 mergedFifoCount{ };
    for (u32 i = 0; i < static_cast<u32>(task.vertex_->inputEdgeCount()); ++i) {
        const auto current = depCount;
        const auto deps = pisdf::computeExecDependency(task.vertex_, task.firing_, i, task.handler_);
        for (const auto dep : deps) {
            depCount += dep.firingEnd_ - dep.firingStart_ + 1u;
        }
        mergedFifoCount += ((current + 1) < depCount);
    }
    return { depCount, mergedFifoCount };
}
