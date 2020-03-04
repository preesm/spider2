/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
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

#include <scheduling/scheduler/ListScheduler.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/PE.h>
#include <graphs/pisdf/ExecVertex.h>

/* === Static variable(s) === */

static constexpr auto NON_SCHEDULABLE_LEVEL = -314159265; /* = Value is arbitrary, just needed something unique = */

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::ListScheduler::ListScheduler(pisdf::Graph *graph) : Scheduler(graph),
                                                            sortedTaskVector_{
                                                                    factory::vector<ListTask>(StackID::SCHEDULE) } {
}

void spider::ListScheduler::clear() {
    Scheduler::clear();
    lastScheduledTask_ = 0;
    lastSchedulableTask_ = 0;
    sortedTaskVector_.clear();
}

void spider::ListScheduler::update() {
    /* == Reserve, creates and add the new ScheduleTask == */
    sortedTaskVector_.reserve(graph_->vertexCount());

    /* == Reset previous non-schedulable tasks == */
    {
        auto last = sortedTaskVector_.size();
        for (auto t = lastSchedulableTask_; t < last; ++t) {
            auto *task = sortedTaskVector_[t].task_;
            auto *vertex = task->vertex();
            vertex->setScheduleTaskIx(t);
            size_t i = 0;
            for (auto &edge : vertex->inputEdgeVector()) {
                auto *source = edge->source();
                if (source->executable() && source->scheduleTaskIx() == SIZE_MAX) {
                    auto *newTask = make<ScheduleTask, StackID::SCHEDULE>(source);
                    newTask->setNumberOfDependencies(source->inputEdgeCount());
                    /* == Set the dependency == */
                    task->setDependency(newTask, i);
                    /* == Add the task to the sorted vector == */
                    sortedTaskVector_.push_back({ newTask, -1 });
                    source->setScheduleTaskIx(sortedTaskVector_.size() - 1);
                }
                i++;
            }
        }
    }

    /* == Reserve and push the vertices into the vertex == */
    for (const auto &vertex : graph_->vertices()) {
        if (vertex->scheduleTaskIx() == SIZE_MAX) {
            auto *task = make<ScheduleTask, StackID::SCHEDULE>(vertex.get());
            task->setNumberOfDependencies(vertex->inputEdgeCount());
            sortedTaskVector_.push_back({ task, -1 });
            vertex->setScheduleTaskIx(sortedTaskVector_.size() - 1);
        }
    }

    /* == Compute the schedule level == */
    {
        auto iterator = sortedTaskVector_.begin() + static_cast<long>(lastSchedulableTask_);
        while (iterator != std::end(sortedTaskVector_)) {
            computeScheduleLevel((*(iterator++)), sortedTaskVector_);
        }
    }

    /* == Sort the vector == */
    sortVertices();

    /* == Remove the non-executable hierarchical vertex == */
    const auto nonSchedulableTaskCount = resetNonSchedulableTasks();

    {
        auto iterator = sortedTaskVector_.begin() + static_cast<long>(lastSchedulableTask_);
        for (; iterator != sortedTaskVector_.end(); ++iterator) {
            auto *task = iterator->task_;
            auto *vertex = task->vertex();
            schedule_.addScheduleTask(task);
            for (size_t i = 0; i < vertex->inputEdgeCount(); ++i) {
                auto *source = vertex->inputEdge(i)->source();
                if (source->executable()) {
                    task->setDependency(schedule_.task(source->scheduleTaskIx()), i);
                }
            }
            vertex->setScheduleTaskIx(static_cast<size_t>(task->ix()));
        }
    }

    /* == Create the schedule tasks == */
    lastSchedulableTask_ = sortedTaskVector_.size() - nonSchedulableTaskCount;

    /* == Update minimum start time == */
    minStartTime_ = schedule_.stats().maxEndTime();
}

/* === Private method(s) === */

ifast32 spider::ListScheduler::computeScheduleLevel(ListTask &listTask,
                                                    vector<ListTask> &listVertexVector) const {
    auto *vertex = listTask.task_->vertex();
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
        auto *platform = archi::platform();
        ifast32 level = 0;
        for (auto &edge : vertex->outputEdgeVector()) {
            auto *sink = edge->sink();
            if (sink && sink->executable()) {
                const auto &sinkParams = sink->inputParamVector();
                auto *sinkRTInfo = sink->runtimeInformation();
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
                const auto sinkLevel = computeScheduleLevel(listVertexVector[sink->scheduleTaskIx()], listVertexVector);
                if (sinkLevel != NON_SCHEDULABLE_LEVEL) {
                    level = std::max(level, sinkLevel + static_cast<ifast32>(minExecutionTime));
                }
            }
        }
        listTask.level_ = level;
    }
    return listTask.level_;
}

void spider::ListScheduler::sortVertices() {
    std::sort(std::begin(sortedTaskVector_) + static_cast<long>(lastSchedulableTask_),
              std::end(sortedTaskVector_),
              [](const ListTask &A, const ListTask &B) -> bool {
                  auto *vertexA = A.task_->vertex();
                  auto *vertexB = B.task_->vertex();
                  const auto diff = A.level_ - B.level_;
                  if (!diff) {
                      if (vertexB->reference() == vertexA->reference()) {
                          return vertexA->instanceValue() < vertexB->instanceValue();
                      }
                      return (vertexA->subtype() == pisdf::VertexType::INIT) ||
                             (vertexB->subtype() == pisdf::VertexType::END);
                  }
                  return (diff > 0);
              });
}

size_t spider::ListScheduler::resetNonSchedulableTasks() {
    auto iterator = sortedTaskVector_.rbegin();
    for (; (iterator->level_ == NON_SCHEDULABLE_LEVEL) && (iterator != sortedTaskVector_.rend()); ++iterator) {
        auto isExec = iterator->task_->vertex()->executable();
        if (!isExec) {
            destroy(iterator->task_);
            std::swap(*iterator, sortedTaskVector_.back());
            sortedTaskVector_.pop_back();
        }
        iterator->level_ = -1;      /* = Reset the schedule level = */
    }
    return static_cast<size_t>(std::distance(sortedTaskVector_.rbegin(), iterator));
}

