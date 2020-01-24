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
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/PE.h>
#include <api/archi-api.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs-tools/helper/visitors/PiSDFDefaultVisitor.h>

/* === Static variable(s) === */

static constexpr auto NON_EXECUTABLE_LEVEL = -314159265; /* = Value is arbitrary, just needed something unique = */

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::ListScheduler::ListScheduler(pisdf::Graph *graph) : Scheduler(graph) {
    /* == Add vertices of the graph and sort the obtained list == */
    addVerticesAndSortList();
}

void spider::ListScheduler::update() {
    /* == Add vertices of the graph and sort the obtained list == */
    addVerticesAndSortList();
}

/* === Private method(s) === */

void spider::ListScheduler::addVerticesAndSortList() {
    /* == Reserve and push the vertices into the vertex == */
    sortedVertexVector_.reserve(graph_->vertexCount());
    for (const auto &vertex : graph_->vertices()) {
        if (vertex->scheduleJobIx() == SIZE_MAX) {
            sortedVertexVector_.emplace_back(ListVertex(vertex.get(), -1));
            sortedVertexVector_.back().vertex_->setScheduleJobIx(sortedVertexVector_.size() - 1);
        }
    }

    /* == Compute the schedule level == */
    auto iterator = sortedVertexVector_.begin() + static_cast<long>(lastSchedulableVertex_);
    while (iterator != std::end(sortedVertexVector_)) {
        computeScheduleLevel((*(iterator++)), sortedVertexVector_);
    }

    /* == Sort the vector == */
    std::sort(std::begin(sortedVertexVector_) + static_cast<long>(lastSchedulableVertex_),
              std::end(sortedVertexVector_),
              [](const ListVertex &A, const ListVertex &B) -> int32_t {
                  bool isSame = B.vertex_->reference() == A.vertex_->reference();
                  return isSame ? B.vertex_->executable() && (B.vertex_->instanceValue() > A.vertex_->instanceValue()) :
                         (B.level_ < A.level_) || (A.vertex_->subtype() == pisdf::VertexType::CONFIG);
              });

    /* == Remove the non-executable hierarchical vertex == */
    size_t nonSchedulableVertexCount = 0;
    auto reverseIterator = sortedVertexVector_.rbegin();
    while ((reverseIterator != sortedVertexVector_.rend()) && (reverseIterator->level_ == NON_EXECUTABLE_LEVEL)) {
        auto isExecutable = reverseIterator->vertex_->executable();
        if (!isExecutable) {
            std::swap((*reverseIterator), sortedVertexVector_.back());
            sortedVertexVector_.pop_back();
        }
        nonSchedulableVertexCount += (isExecutable); /* = Increase the number of non-schedulable vertex = */
        reverseIterator->level_ = -1;                /* = Reset the schedule level = */
        reverseIterator++;
    }

    /* == Set the schedule job ix of the vertices == */
    iterator = std::begin(sortedVertexVector_) + static_cast<long>(lastSchedulableVertex_);
    auto endIterator = std::end(sortedVertexVector_) - nonSchedulableVertexCount;
    while (iterator != endIterator) {
        schedule_.addJobToSchedule((iterator++)->vertex_);
    }
    lastSchedulableVertex_ = sortedVertexVector_.size() - nonSchedulableVertexCount;
}

int64_t spider::ListScheduler::computeScheduleLevel(ListVertex &listVertex,
                                                    spider::vector<ListVertex> &listVertexVector) const {
    if ((listVertex.level_ == NON_EXECUTABLE_LEVEL) || !listVertex.vertex_->executable()) {
        listVertex.level_ = NON_EXECUTABLE_LEVEL;
        for (auto &edge : listVertex.vertex_->outputEdgeVector()) {
            listVertexVector[edge->sink()->ix()].level_ = NON_EXECUTABLE_LEVEL;
        }
    } else if (listVertex.level_ < 0) {
        auto *platform = archi::platform();
        auto *vertex = listVertex.vertex_;
        int64_t level = 0;
        for (auto &edge : vertex->outputEdgeVector()) {
            auto *sink = edge->sink();
            if (sink && sink->executable()) {
                const auto &sinkParams = sink->inputParamVector();
                auto *sinkRTInfo = sink->runtimeInformation();
                auto minExecutionTime = INT64_MAX;
                for (auto &cluster : platform->clusters()) {
                    for (auto &pe : cluster->array()) {
                        if (sinkRTInfo->isPEMappable(pe)) {
                            auto executionTime = sinkRTInfo->timingOnPE(pe, sinkParams);
                            if (!executionTime) {
                                throwSpiderException("Vertex [%s] has null execution time on mappable PE [%s].",
                                                     vertex->name().c_str(), pe->name().c_str());
                            }
                            minExecutionTime = std::min(minExecutionTime, executionTime);
                            break; /* = We can break because any other PE of the cluster will have the same timing = */
                        }
                    }
                }
                const auto &sinkLevel = computeScheduleLevel(listVertexVector[sink->scheduleJobIx()], listVertexVector);
                if (sinkLevel != NON_EXECUTABLE_LEVEL) {
                    level = std::max(level, sinkLevel + minExecutionTime);
                }
            }
        }
        listVertex.level_ = level;
    }
    return listVertex.level_;
}