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
/* === Include(s) === */

#include <scheduling/scheduler/pisdf-based/PiSDFListScheduler.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/task/PiSDFTask.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>

#include <graphs-tools/numerical/dependencies.h>
#include <graphs-tools/numerical/detail/dependenciesImpl.h>

/* === Static function === */

namespace {
    constexpr auto NON_SCHEDULABLE_LEVEL = 314159265; /* = Value is arbitrary, just needed something unique = */
}

/* === Method(s) implementation === */

spider::sched::PiSDFListScheduler::PiSDFListScheduler() :
        Scheduler(),
        sortedTaskVector_{ factory::vector<ListTask>(StackID::SCHEDULE) } {

}

void spider::sched::PiSDFListScheduler::schedule(pisdf::GraphHandler *graphHandler, Schedule *schedule) {
    /* == Reset previous non-schedulable tasks == */
    resetUnScheduledTasks();
    /* == Creates ListTasks == */
    recursiveAddVertices(graphHandler);
    /* == Compute the schedule level == */
    for (auto &task : sortedTaskVector_) {
        computeScheduleLevel(sortedTaskVector_, task);
    }
    /* == Sort the vector == */
    sortVertices();
    /* == Remove the non-executable hierarchical vertex == */
    const auto nonSchedulableTaskCount = countNonSchedulableTasks();
    /* == Update last schedulable vertex == */
    const auto lastSchedulable = sortedTaskVector_.size() - nonSchedulableTaskCount;
    /* == Create the list of tasks to be scheduled == */
    schedule->reserve(lastSchedulable);
    for (size_t k = 0; k < lastSchedulable; ++k) {
        auto &task = sortedTaskVector_[k];
        task.handler_->setTaskIx(task.vertex_, task.firing_, UINT32_MAX);
    }
    for (size_t k = 0; k < lastSchedulable; ++k) {
        auto &task = sortedTaskVector_[k];
        Scheduler::addTask(schedule, task.handler_, task.vertex_, task.firing_);
    }
    /* == Remove scheduled vertices == */
    auto it = std::begin(sortedTaskVector_);
    for (auto k = lastSchedulable; k < sortedTaskVector_.size(); ++k) {
        std::swap(sortedTaskVector_[k], *(it++));
    }
    while (sortedTaskVector_.size() != nonSchedulableTaskCount) {
        sortedTaskVector_.pop_back();
    }
}

void spider::sched::PiSDFListScheduler::clear() {
    Scheduler::clear();
    sortedTaskVector_.clear();
}

/* === Private method(s) implementation === */

void spider::sched::PiSDFListScheduler::resetUnScheduledTasks() {
    for (size_t k = 0; k < sortedTaskVector_.size(); ++k) {
        auto &listTask = sortedTaskVector_[k];
        listTask.handler_->setTaskIx(listTask.vertex_, listTask.firing_, static_cast<u32>(k));
    }
}

void spider::sched::PiSDFListScheduler::recursiveAddVertices(pisdf::GraphHandler *graphHandler) {
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
            for (auto *child : firingHandler->subgraphHandlers()) {
                recursiveAddVertices(child);
            }
        } else {
            const auto *graph = graphHandler->graph();
            const auto *handler = graphHandler->base();
            const auto firing = firingHandler->firingValue();
            recursiveSetNonSchedulable(sortedTaskVector_, handler, graph, firing);
        }
    }
}

void spider::sched::PiSDFListScheduler::createListTask(pisdf::Vertex *vertex,
                                                       u32 firing,
                                                       pisdf::GraphFiring *handler) {
    const auto vertexTaskIx = handler->getTaskIx(vertex, firing);
    if (vertexTaskIx == UINT32_MAX && vertex->executable()) {
        sortedTaskVector_.push_back({ vertex, handler, -1, firing });
        handler->setTaskIx(vertex, firing, static_cast<u32>(sortedTaskVector_.size() - 1));
    }
}

void spider::sched::PiSDFListScheduler::recursiveSetNonSchedulable(spider::vector<ListTask> &sortedTaskVector,
                                                                   const pisdf::GraphFiring *handler,
                                                                   const pisdf::Vertex *vertex,
                                                                   u32 firing) {
    const auto lambda = [&sortedTaskVector](const pisdf::DependencyInfo &dep) {
        if (dep.vertex_ && dep.rate_ > 0) {
            /* == Disable non-null edge == */
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                auto &sinkTask = sortedTaskVector[dep.handler_->getTaskIx(dep.vertex_, k)];
                sinkTask.level_ = NON_SCHEDULABLE_LEVEL;
                recursiveSetNonSchedulable(sortedTaskVector, dep.handler_, dep.vertex_, k);
            }
        }
    };
    for (const auto *edge : vertex->outputEdges()) {
        const auto srcRate = handler->getSrcRate(edge);
        pisdf::detail::computeConsDependency(edge, srcRate * firing, srcRate * (firing + 1) - 1, handler, lambda);
    }
}

i32 spider::sched::PiSDFListScheduler::computeScheduleLevel(spider::vector<ListTask> &sortedTaskVector,
                                                            ListTask &listTask) {
    const auto *vertex = listTask.vertex_;
    const auto *handler = listTask.handler_;
    const auto firing = listTask.firing_;
    if (listTask.level_ == NON_SCHEDULABLE_LEVEL) {
        recursiveSetNonSchedulable(sortedTaskVector, handler, vertex, firing);
    } else if (listTask.level_ < 0) {
        i32 level = 0;
        const auto lambda = [&level, &sortedTaskVector](const pisdf::DependencyInfo &dep) {
            const auto *source = dep.vertex_;
            if (!source || dep.rate_ <= 0) {
                return;
            }
            const auto *sourceRTInfo = source->runtimeInformation();
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto minExecutionTime = computeMinExecTime(sourceRTInfo, dep.handler_->getParams());
                const auto sourceTaskIx = dep.handler_->getTaskIx(source, k);
                if (sourceTaskIx < sortedTaskVector.size()) {
                    auto &srcTask = sortedTaskVector[sourceTaskIx];
                    if (srcTask.vertex_ == source &&
                        srcTask.handler_ == dep.handler_ &&
                        srcTask.firing_ >= dep.firingStart_ && srcTask.firing_ <= dep.firingEnd_) {
                        const auto sourceLevel = computeScheduleLevel(sortedTaskVector, srcTask);
                        if (sourceLevel != NON_SCHEDULABLE_LEVEL) {
                            level = std::max(level, sourceLevel + static_cast<i32>(minExecutionTime));
                        }
                    }
                }
            }
        };
        for (const auto *edge : vertex->inputEdges()) {
            const auto snkRate = handler->getSnkRate(edge);
            pisdf::detail::computeExecDependency(edge, snkRate * firing, snkRate * (firing + 1) - 1, handler, lambda);
        }
        listTask.level_ = level;
    }
    return listTask.level_;
}

i64 spider::sched::PiSDFListScheduler::computeMinExecTime(const RTInfo *rtInfo,
                                                          const spider::vector<std::shared_ptr<pisdf::Param>> &params) {
    auto minExecutionTime = INT64_MAX;
    const auto *platform = archi::platform();
    for (auto &cluster : platform->clusters()) {
        if (rtInfo->isClusterMappable(cluster)) {
            for (const auto &pe : cluster->peArray()) {
                auto executionTime = rtInfo->timingOnPE(pe, params);
                minExecutionTime = std::min(minExecutionTime, executionTime);
            }
        }
    }
    return minExecutionTime;
}

void spider::sched::PiSDFListScheduler::sortVertices() {
    std::sort(std::begin(sortedTaskVector_), std::end(sortedTaskVector_),
              [](const ListTask &A, const ListTask &B) -> bool {
                  const auto diff = A.level_ - B.level_;
                  if (!diff) {
                      const auto *vertexA = A.vertex_;
                      const auto *vertexB = B.vertex_;
                      if (vertexB == vertexA) {
                          auto firingA = A.firing_;
                          auto firingB = B.firing_;
                          const auto *handlerA = A.handler_;
                          const auto *handlerB = B.handler_;
                          while ((handlerA && handlerB) && (firingA == firingB)) {
                              firingA = handlerA->firingValue();
                              firingB = handlerB->firingValue();
                              handlerA = handlerA->getParent()->base();
                              handlerB = handlerB->getParent()->base();
                          }
                          return firingA < firingB;
                      }
                      return (vertexA->subtype() != vertexB->subtype()) &&
                             (vertexA->subtype() == pisdf::VertexType::INIT ||
                              vertexB->subtype() == pisdf::VertexType::END);
                  }
                  return diff < 0;
              });
}

size_t spider::sched::PiSDFListScheduler::countNonSchedulableTasks() {
    auto it = sortedTaskVector_.rbegin();
    size_t count{ };
    for (; (it->level_ == NON_SCHEDULABLE_LEVEL) && (it != sortedTaskVector_.rend()); ++it) {
        count++;
        it->handler_->setTaskIx(it->vertex_, it->firing_, UINT32_MAX);
        it->level_ = -1; /* = Reset the schedule level = */
    }
    return count;
}
