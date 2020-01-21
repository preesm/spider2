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

#include <scheduling/scheduler/GreedyScheduler.h>
#include <scheduling/schedule/Schedule.h>
#include <containers/containers.h>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::sched::Schedule &spider::GreedyScheduler::mappingScheduling() {
    auto vertexVector = containers::vector<pisdf::Vertex *>(StackID::SCHEDULE);
    vertexVector.reserve(graph_->vertexCount());

    /* == Initialize vector of vertex == */
    for (const auto &vertex : graph_->vertices()) {
        vertexVector.emplace_back(vertex);
    }

    /* == Initialize the jobs of the schedule == */
    schedule_.updateScheduleSize(vertexVector.size());

    /* == Iterate on vector until a schedulable vertex is found == */
    auto it = vertexVector.begin();
    while (!vertexVector.empty()) {
        /* == Reset iterator to start over == */
        if (it == vertexVector.end()) {
            it = vertexVector.begin();
        }

        /* == Test if vertex is schedulable == */
        if (isSchedulable((*it))) {
            /* == Map the vertex == */
            Scheduler::vertexMapper((*it));

            /* == Remove current vertex from the vector == */
            std::swap((*it), vertexVector.back());
            vertexVector.pop_back();
        } else {
            it++;
        }
    }
    return schedule_;
}

bool spider::GreedyScheduler::isSchedulable(pisdf::Vertex *vertex) const {
    if (!vertex->inputEdgeCount()) {
        return true;
    }
    for (const auto &edge : vertex->inputEdgeVector()) {
        const auto &source = edge->source();
        const auto &job = schedule_.job(source->ix());
        if (job.mappingInfo().PEIx == UINT32_MAX) {
            return false;
        }
    }
    return true;
}
