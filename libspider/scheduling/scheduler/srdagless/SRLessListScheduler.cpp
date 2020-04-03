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

#include <scheduling/scheduler/srdagless/SRLessListScheduler.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/PE.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs-tools/numerical/dependencies.h>

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
                sortedTaskVector_.push_back({ task, i, 0, -1 });
            }
            vertex->setScheduleTaskIx(sortedTaskVector_.size() - vertex->repetitionValue());
        }
    }

    /* == Compute the schedule level == */
    for (auto &listVertex : sortedTaskVector_) {
        computeScheduleLevel(listVertex, sortedTaskVector_);
    }

    /* == Sort the vector == */
    sortVertices();

    for (auto &listTask : sortedTaskVector_) {
        auto *task = listTask.task_;
        auto *vertex = task->vertex();
        schedule_.addScheduleTask(task);
        task->setNumberOfDependencies(listTask.dependencyCount_);
        size_t pos = 0;
        for (auto edge : vertex->inputEdgeVector()) {
            auto *source = edge->source();
            if (source->subtype() == pisdf::VertexType::DELAY) {
                auto *delayVertex = source->convertTo<pisdf::DelayVertex>();
                auto *delayEdge = delayVertex->delay()->edge();
                source = delayEdge->source();
                auto delay = delayEdge->delay()->value();
                delay = delay - delayEdge->sinkRateValue() * delayEdge->sink()->repetitionValue();
                const auto sourceRate = delayEdge->sinkRateValue();
                const auto getterRate = edge->sinkRateValue();
                const auto depMin = pisdf::computeConsLowerDep(getterRate, sourceRate, listTask.firing_, delay);
                const auto depMax = pisdf::computeConsUpperDep(getterRate, sourceRate, listTask.firing_, delay);
                for (auto i = depMin; i <= depMax; ++i) {
                    task->setDependency(schedule_.task(source->scheduleTaskIx() + static_cast<ufast64>(i)), pos++);
                }
            } else if (source->executable()) {
                const auto sinkRate = edge->sinkRateValue();
                const auto sourceRate = edge->sourceRateValue();
                const auto delay = edge->delay() ? edge->delay()->value() : 0;
                auto depMin = pisdf::computeConsLowerDep(sinkRate, sourceRate, listTask.firing_, delay);
                const auto depMax = pisdf::computeConsUpperDep(sinkRate, sourceRate, listTask.firing_, delay);
                if (depMin < 0) {
                    depMin = 0;

                }
                for (auto i = depMin; i <= depMax; ++i) {
                    task->setDependency(schedule_.task(source->scheduleTaskIx() + static_cast<ufast64>(i)), pos++);
                }
            }
        }
        vertex->setScheduleTaskIx(static_cast<size_t>(task->ix()));
    }

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
        // TODO: Get the proper rate of the edge -> get the firing of the containing graph to get the parameters.
        setNextVerticesNonSchedulable(vertex, listVertexVector);
    } else if (listTask.level_ < 0) {
        auto *platform = archi::platform();
        ifast32 level = 0;
        for (auto &edge : vertex->outputEdgeVector()) {
            auto *sink = edge->sink();
            if (!sink) {
                continue;
            }
            if (sink->subtype() == pisdf::VertexType::DELAY) {
                auto *delayVertex = sink->convertTo<pisdf::DelayVertex>();
                sink = delayVertex->delay()->edge()->sink();
            }
            if (sink->executable()) {
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
                const auto sinkRate = edge->sinkRateValue();
                const auto sourceRate = edge->sourceRateValue();
                const auto delay = edge->delay() ? edge->delay()->value() : 0;
                const auto depMin = pisdf::computeProdLowerDep(sinkRate, sourceRate, listTask.firing_, delay);
                auto depMax = pisdf::computeProdUpperDep(sinkRate, sourceRate, listTask.firing_, delay);
                if (depMax >= sink->repetitionValue()) {
                    depMax = sink->repetitionValue() - 1;
                    pisdf::Vertex *end = nullptr;
                    if (!edge->delay()) {
                        /* == Case of setter / init to end == */
                        auto *delayVertex = edge->sink()->convertTo<pisdf::DelayVertex>();
                        end = delayVertex->delay()->getter();
                    } else {
                        /* == Case of produced to end == */
                        end = edge->delay()->getter();
                    }
                    const auto sinkLevel = computeScheduleLevel(listVertexVector[end->scheduleTaskIx()],
                                                                listVertexVector);
                    if (sinkLevel != NON_SCHEDULABLE_LEVEL) {
                        level = std::max(level, sinkLevel + static_cast<ifast32>(minExecutionTime));
                    }
                }
                for (auto i = depMin; i <= depMax; ++i) {
                    auto &nextListTask = listVertexVector[sink->scheduleTaskIx() + static_cast<ufast64>(i)];
                    nextListTask.dependencyCount_ += 1;
                    const auto sinkLevel = computeScheduleLevel(nextListTask, listVertexVector);
                    if (sinkLevel != NON_SCHEDULABLE_LEVEL) {
                        level = std::max(level, sinkLevel + static_cast<ifast32>(minExecutionTime));
                    }
                }
            }
        }
        listTask.level_ = level;
    }
    return listTask.level_;
}

void spider::SRLessListScheduler::setNextVerticesNonSchedulable(pisdf::Vertex *vertex,
                                                                vector<ListTask> &listVertexVector) const {
    for (auto &edge : vertex->outputEdgeVector()) {
        if (edge->sinkRateValue()) {
            /* == Disable non-null edge == */
            auto &sinkTask = listVertexVector[edge->sink()->scheduleTaskIx()];
            sinkTask.level_ = NON_SCHEDULABLE_LEVEL;
            computeScheduleLevel(sinkTask, listVertexVector);
        }
    }
}

void spider::SRLessListScheduler::sortVertices() {
    std::sort(std::begin(sortedTaskVector_), std::end(sortedTaskVector_),
              [](const ListTask &A, const ListTask &B) -> bool {
                  auto *vertexA = A.task_->vertex();
                  auto *vertexB = B.task_->vertex();
                  const auto diff = A.level_ - B.level_;
                  if (!diff) {
                      if (vertexA == vertexB) {
                          return A.firing_ < B.firing_;
                      }
                      return (vertexA->subtype() == pisdf::VertexType::INIT) ||
                             (vertexB->subtype() == pisdf::VertexType::END);
                  }
                  return (diff > 0);
              });
}
