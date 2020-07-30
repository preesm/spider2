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

#include <scheduling/scheduler/srdagless/SRLessListScheduler.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/PE.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs-tools/numerical/dependencies.h>
#include <numeric>

/* === Static variable(s) === */

static constexpr auto NON_SCHEDULABLE_LEVEL = -314159265; /* = Value is arbitrary, just needed something unique = */

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::SRLessListScheduler::SRLessListScheduler(pisdf::Graph *graph) :
        SRLessScheduler(graph),
        sortedTaskVector_{ factory::vector<ListTask>(StackID::SCHEDULE) } {

}

void spider::SRLessListScheduler::update() {
    /* == Reserve and push the vertices into the vertex == */
    for (const auto &vertex : graph_->vertices()) {
        if (vertex->subtype() == pisdf::VertexType::DELAY) {
            continue;
        }
        if (vertex->scheduleTaskIx() == SIZE_MAX) {
            for (u32 i = 0; i < vertex->repetitionValue(); ++i) {
                auto *task = make<ScheduleTask, StackID::SCHEDULE>(vertex.get());
                sortedTaskVector_.push_back({ task, i, -1 });
            }
            vertex->setScheduleTaskIx(sortedTaskVector_.size() - vertex->repetitionValue());
        }
    }

    /* == Compute the schedule level == */
    for (auto &listVertex : sortedTaskVector_) {
        computeScheduleLevel(listVertex, sortedTaskVector_);
    }

    auto iterator = sortedTaskVector_.begin() + static_cast<long>(lastSchedulableTask_);
    for (; iterator != sortedTaskVector_.end(); ++iterator) {
        auto *task = iterator->task_;
        auto *vertex = task->vertex();
        schedule_.addScheduleTask(task);
        const auto &dependencies = handler_.getVertexDependencies(vertex)[iterator->firing_];
        size_t nbDep = 0;
        for (const auto &dependency : dependencies) {
            nbDep += ((dependency.firingEnd_ - dependency.firingStart_) + 1);
        }
        task->setNumberOfDependencies(nbDep);
        size_t index = 0;
        size_t depIndex = 0;
        for (const auto &edge : vertex->inputEdgeVector()) {
            auto *source = dependencies[index].vertex_;
            const auto numberOfDependencies = 1 + (source != edge->source());
            for (auto i = 0; i < numberOfDependencies; ++i) {
                if (source->executable()) {
                    auto &srcDep = dependencies[index++];
                    for (auto k = srcDep.firingStart_; k <= srcDep.firingEnd_; ++k) {
                        auto &srcTask = sortedTaskVector_[srcDep.vertex_->scheduleTaskIx() + k];
                        task->setDependency(srcTask.task_, depIndex++);
                    }
                }
            }
        }
    }
    iterator = sortedTaskVector_.begin() + static_cast<long>(lastSchedulableTask_);
    for (; iterator != sortedTaskVector_.end(); ++iterator) {
        auto *task = iterator->task_;
        auto *vertex = task->vertex();
        vertex->setScheduleTaskIx(static_cast<size_t>(task->ix()));
    }

    /* == Sort the vector == */
    sortTasks();

    /* == Create the schedule tasks == */
    lastSchedulableTask_ = sortedTaskVector_.size() - 0;

    /* == Update minimum start time == */
    minStartTime_ = schedule_.stats().maxEndTime();
}

void spider::SRLessListScheduler::clear() {
    Scheduler::clear();
}

/* === Private method(s) === */

ifast32 spider::SRLessListScheduler::computeScheduleLevel(SRLessListScheduler::ListTask &listTask,
                                                          vector<ListTask> &listVertexVector) const {
    auto *vertex = listTask.task_->vertex();
    if ((listTask.level_ == NON_SCHEDULABLE_LEVEL) || !vertex->executable()) {
        listTask.level_ = NON_SCHEDULABLE_LEVEL;
        setNextVerticesNonSchedulable(vertex, listVertexVector);
    } else if (listTask.level_ < 0) {
        auto *platform = archi::platform();
        const auto &dependencies = handler_.getVertexDependencies(vertex);
        for (const auto &edge : vertex->outputEdgeVector()) {
            if (edge->sink()->subtype() == pisdf::VertexType::DELAY) {
                continue;
            }
            computeScheduleLevel(listVertexVector[edge->sink()->scheduleTaskIx()], listVertexVector);
        }
        for (u32 k = 0; k < vertex->repetitionValue(); ++k) {
            size_t index = 0;
            auto &firingListTask = listVertexVector[vertex->scheduleTaskIx() + k];
            firingListTask.level_ = std::max(firingListTask.level_, ifast32{ 0 });
            for (const auto &edge : vertex->inputEdgeVector()) {
                auto *source = dependencies[firingListTask.firing_][index].vertex_;
                const auto numberOfDependencies =
                        1 + ((source != edge->source()) && edge->source()->subtype() != pisdf::VertexType::DELAY);
                for (auto i = 0; i < numberOfDependencies; ++i) {
                    if (source->executable()) {
                        const auto *rtInfo = vertex->runtimeInformation();
                        auto minExecutionTime = INT64_MAX;
                        for (auto &cluster : platform->clusters()) {
                            if (rtInfo->isClusterMappable(cluster)) {
                                for (const auto &pe : cluster->peArray()) {
                                    const auto executionTime = rtInfo->timingOnPE(pe, handler_.getParameters());
                                    if (!executionTime) {
                                        throwSpiderException("Vertex [%s] has null execution time on mappable cluster.",
                                                             vertex->name().c_str());
                                    }
                                    minExecutionTime = std::min(minExecutionTime, executionTime);
                                }
                            }
                        }
                        auto &srcDep = dependencies[firingListTask.firing_][index++];
                        for (auto delta = srcDep.firingStart_; delta <= srcDep.firingEnd_; ++delta) {
                            auto &srcTask = listVertexVector[srcDep.vertex_->scheduleTaskIx() + delta];
                            srcTask.level_ = std::max(srcTask.level_, firingListTask.level_ + static_cast<ifast32>(minExecutionTime));
                        }
                        source = srcDep.vertex_;
                    }
                }
            }
        }
    }
    return listTask.level_;
}

void spider::SRLessListScheduler::setNextVerticesNonSchedulable(pisdf::Vertex *vertex,
                                                                vector<ListTask> &listVertexVector) const {
    for (const auto &edge : vertex->outputEdgeVector()) {
        /* == Disable non-null edge == */
        if (edge->sinkRateExpression().evaluate(handler_.getParameters())) {
            for (u32 i = 0; i < edge->sink()->repetitionValue(); ++i) {
                auto &sinkTask = listVertexVector[edge->sink()->scheduleTaskIx() + i];
                sinkTask.level_ = NON_SCHEDULABLE_LEVEL;
            }
            setNextVerticesNonSchedulable(vertex, listVertexVector);
        }
    }
}

void spider::SRLessListScheduler::sortTasks() {
    std::sort(std::begin(sortedTaskVector_), std::end(sortedTaskVector_),
              [](const ListTask &A, const ListTask &B) -> bool {
                  const auto *vertexA = A.task_->vertex();
                  const auto *vertexB = B.task_->vertex();
                  const auto diff = A.level_ - B.level_;
                  if (!diff) {
                      if (vertexA == vertexB) {
                          return A.firing_ < B.firing_;
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
