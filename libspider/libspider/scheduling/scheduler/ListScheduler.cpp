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
#include <graphs/pisdf/ExecVertex.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/PE.h>
#include <spider-api/archi.h>
#include <scenario/Scenario.h>

/* === Static variable(s) === */

/* === Static function(s) === */

int64_t spider::ListScheduler::computeScheduleLevel(ListVertex &listVertex,
                                                    spider::vector<ListVertex> &sortedVertexVector) {
    if (listVertex.level_ < 0) {
        auto *platform = spider::platform();
        auto *vertex = listVertex.vertex_;
        int64_t level = 0;
        for (auto &edge : vertex->outputEdgeArray()) {
            auto *sink = edge->sink();
            if (sink) {
                auto *scenario = vertex->graph()->scenario();
                auto minExecutionTime = INT64_MAX;
                for (auto &cluster : platform->clusters()) {
                    for (auto &pe : cluster->processingElements()) {
                        if (scenario->isMappable(sink, pe)) {
                            auto executionTime = scenario->executionTiming(sink, cluster->PEType());
                            if (!executionTime) {
                                throwSpiderException("Vertex [%s] has null execution time on mappable PE [%s].",
                                                     vertex->name().c_str(), pe->name().c_str());
                            }
                            minExecutionTime = std::min(minExecutionTime, executionTime);
                            break; /* = We can break because any other PE of the cluster will have the same timing = */
                        }
                    }
                }
                level = std::max(level, computeScheduleLevel(sortedVertexVector[sink->ix()], sortedVertexVector) +
                                        minExecutionTime);
            }
        }
        listVertex.level_ = level;
        return level;
    }

    return listVertex.level_;
}

/* === Method(s) implementation === */

spider::ListScheduler::ListScheduler(PiSDFGraph *graph,
                                     const spider::vector<PiSDFParam *> &params) : Scheduler(graph, params) {
    /* == Reserve and push the vertices into the vertex == */
    sortedVertexVector_.reserve(graph_->vertexCount());
    for (auto *vertex : graph_->vertices()) {
        sortedVertexVector_.push_back(ListVertex(vertex, -1));
    }

    /* == Compute the schedule level == */
    for (auto &listVertex : sortedVertexVector_) {
        computeScheduleLevel(listVertex, sortedVertexVector_);
    }

    /* == Sort the vector == */
    std::sort(std::begin(sortedVertexVector_), std::end(sortedVertexVector_),
              [](const ListVertex &A, const ListVertex &B) -> int32_t {
                  if (B.vertex_->subtype() == PiSDFVertexType::NORMAL &&
                      A.vertex_->reference() == B.vertex_->reference() &&
                      A.level_ == B.level_) {
                      return B.vertex_->ix() > A.vertex_->ix();
                  }
                  return B.level_ < A.level_;
              });
}
