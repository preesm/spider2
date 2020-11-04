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

#include <scheduling/scheduler/pisdf-based/PiSDFGreedyScheduler.h>
#include <scheduling/task/PiSDFTask.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs-tools/transformation/pisdf/GraphAlloc.h>
#include <graphs-tools/numerical/dependencies.h>
#include <graphs-tools/numerical/detail/dependenciesImpl.h>

/* === Static function === */

/* === Method(s) implementation === */

void spider::sched::PiSDFGreedyScheduler::schedule(pisdf::GraphHandler *graphHandler, Schedule *schedule) {
    /* == Evaluate tasks == */
    evaluate(graphHandler, schedule);
}

/* === Private method(s) implementation === */

void spider::sched::PiSDFGreedyScheduler::evaluate(pisdf::GraphHandler *graphHandler, Schedule *schedule) {
    for (auto *firingHandler : graphHandler->firings()) {
        if (firingHandler->isResolved()) {
            for (const auto &vertex : graphHandler->graph()->vertices()) {
                if (vertex->subtype() != spider::pisdf::VertexType::DELAY && vertex->executable()) {
                    const auto vertexRV = firingHandler->getRV(vertex.get());
                    for (u32 k = 0u; k < vertexRV; ++k) {
                        evaluate(firingHandler, vertex.get(), k, schedule);
                    }
                }
            }
            for (auto *subgraphHandler : firingHandler->subgraphHandlers()) {
                evaluate(subgraphHandler, schedule);
            }
        }
    }
}

bool spider::sched::PiSDFGreedyScheduler::evaluate(pisdf::GraphFiring *handler,
                                                   const pisdf::Vertex *vertex,
                                                   u32 firing,
                                                   Schedule *schedule) {
    auto schedulable = true;
    if (handler->getAlloc()->getTaskIx(vertex, firing) == UINT32_MAX) {
        auto lambda = [&schedule, &schedulable](const pisdf::DependencyInfo &dep) {
            if (!dep.rate_) {
                return;
            } else if (!dep.vertex_ || dep.rate_ < 0) {
                schedulable = false;
                return;
            }
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                schedulable &= evaluate(const_cast<pisdf::GraphFiring *>(dep.handler_), dep.vertex_, k, schedule);
            }
        };
        for (const auto *edge : vertex->inputEdges()) {
            const auto snkRate = handler->getSnkRate(edge);
            pisdf::detail::computeExecDependency(edge, snkRate * firing, snkRate * (firing + 1) - 1, handler, lambda);
            if (!schedulable) {
                return false;
            }
        }
        if (schedulable) {
            Scheduler::addTask(schedule, handler, vertex, firing);
        }
    }
    return schedulable;
}