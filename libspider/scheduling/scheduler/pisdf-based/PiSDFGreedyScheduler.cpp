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

#include <scheduling/scheduler/pisdf-based/PiSDFGreedyScheduler.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs-tools/numerical/dependencies.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::vector<spider::pisdf::VertexFiring>
spider::sched::PiSDFGreedyScheduler::schedule(pisdf::GraphHandler *graphHandler) {
    /* == Reserve space for the new ListTasks == */
    tasks_.clear();
    auto result = factory::vector<pisdf::VertexFiring>(StackID::SCHEDULE);
    /* == Evaluate tasks == */
    evaluate(graphHandler, result);
    return result;
}

/* === Private method(s) implementation === */

void spider::sched::PiSDFGreedyScheduler::evaluate(pisdf::GraphHandler *graphHandler,
                                                   spider::vector<pisdf::VertexFiring> &result) {
    for (auto *firingHandler : graphHandler->firings()) {
        if (firingHandler->isResolved()) {
            for (const auto &vertex : graphHandler->graph()->vertices()) {
                if (vertex->subtype() != spider::pisdf::VertexType::DELAY && vertex->executable()) {
                    const auto vertexRV = firingHandler->getRV(vertex.get());
                    for (u32 k = 0u; k < vertexRV; ++k) {
                        evaluate(firingHandler, vertex.get(), k, result);
                    }
                }
            }
            for (auto *subgraphHandler : firingHandler->subgraphHandlers()) {
                evaluate(subgraphHandler, result);
            }
        }
    }
}

bool spider::sched::PiSDFGreedyScheduler::evaluate(pisdf::GraphFiring *handler,
                                                   const pisdf::Vertex *vertex,
                                                   u32 firing,
                                                   spider::vector<pisdf::VertexFiring> &result) {
    auto schedulable = true;
    if (handler->getTaskIx(vertex, firing) == UINT32_MAX) {
        u32 depCount{ };
        u32 mergedFifoCount{ };
        for (const auto *edge : vertex->inputEdges()) {
            const auto current = depCount;
            const auto deps = pisdf::computeExecDependency(vertex, firing, edge->sinkPortIx(), handler);
            for (const auto &dep : deps) {
                depCount += (dep.firingEnd_ - dep.firingStart_ + 1u);
                if (!dep.rate_) {
                    continue;
                } else if (!dep.vertex_ || dep.rate_ < 0) {
                    return false;
                }
                for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                    if (dep.handler_->getTaskIx(dep.vertex_, k) == UINT32_MAX) {
                        schedulable &= evaluate(const_cast<pisdf::GraphFiring *>(dep.handler_), dep.vertex_, k, result);
                    }
                }
            }
            mergedFifoCount += ((current + 1) < depCount);
        }
        if (schedulable) {
            handler->registerTaskIx(vertex, firing, static_cast<u32>(tasks_.size()));
            result.push_back({ handler, vertex, firing });
        }
    }
    return schedulable;
}
