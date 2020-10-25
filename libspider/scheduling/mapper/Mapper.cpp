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

#include <scheduling/mapper/Mapper.h>
#include <scheduling/task/Task.h>
#include <graphs/sched/SchedVertex.h>
#include <graphs/sched/SchedEdge.h>

/* === Static function === */

/* === Method(s) implementation === */

ufast64 spider::sched::Mapper::computeStartTime(const Task *task, const Schedule *schedule) const {
    auto minTime = startTime_;
    if (task) {
        for (size_t ix = 0; ix < task->dependencyCount(); ++ix) {
            const auto *source = task->previousTask(ix, schedule);
            minTime = std::max(minTime, source ? source->endTime() : ufast64{ 0 });
        }
    }
    return minTime;
}

ufast64 spider::sched::Mapper::computeStartTime(const sched::Vertex *vertex) const {
    auto minTime = startTime_;
    if (vertex) {
        for (const auto *edge : vertex->inputEdges()) {
            const auto *source = edge->source();
            minTime = std::max(minTime, source ? source->endTime() : ufast64{ 0 });
        }
    }
    return minTime;
}

/* === Private method(s) implementation === */
