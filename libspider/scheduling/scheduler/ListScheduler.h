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
#ifndef SPIDER2_LISTSCHEDULER_H
#define SPIDER2_LISTSCHEDULER_H

/* === Include(s) === */

#include <scheduling/scheduler/Scheduler.h>
#include <containers/containers.h>

namespace spider {

    /* === Class definition === */

    class ListScheduler : public Scheduler {
    public:

        ~ListScheduler() override = default;

        /* === Method(s) === */

        sched::Schedule &mappingScheduling() override = 0;

        void update() override;

        /* === Getter(s) === */

        /* === Setter(s) === */

    protected:
        size_t lastSchedulableVertex_ = 0;
        size_t lastScheduledVertex_ = 0;

        struct ListVertex {
            pisdf::Vertex *vertex_ = nullptr;
            int64_t level_ = -1;
            size_t updateIx_ = 0;

            explicit ListVertex(pisdf::Vertex *vertex, int32_t level = -1) : vertex_{ vertex },
                                                                             level_{ level } { };
        };

        explicit ListScheduler(pisdf::Graph *graph) : ListScheduler(graph, graph->params()) { };

        ListScheduler(pisdf::Graph *graph, const spider::vector<pisdf::Param *> &params);

        stack_vector(sortedVertexVector_, ListVertex, StackID::SCHEDULE);

        /* === Protected method(s) === */

    private:

        /* === Private method(s) === */

        /**
         * @brief Add vertices of the member @refitem pisdf::Graph in sorted order into the sortedVertexVector_
         * for mappingScheduling.
         * @remark This method also updates the number of jobs in the member @refitem sched::Schedule.
         */
        void addVerticesAndSortList();

        /**
         * @brief Compute recursively the schedule level used to sort the vertices for scheduling.
         * The criteria used is based on the critical execution time path.
         * @example:
         *         input graph:
         *             A (100) -> B(200)
         *                     -> C(100) -> D(100)
         *                               -> E(300)
         *         result:
         *           level(A) = max(level(C) + time(C); level(B) + time(B)) = 400
         *           level(B) = level(D) = level(E) = 0
         *           level(C) = max(level(D) + time(D); level(E) + time(E)) = 300
         * @param listVertex       Pointer to the current @refitem ListVertex evaluated.
         * @param listVertexVector Vector of @refitem ListVertex to evaluate.
         * @return
         */
        int64_t computeScheduleLevel(ListVertex &listVertex, spider::vector<ListVertex> &listVertexVector) const;
    };
}

#endif //SPIDER2_LISTSCHEDULER_H
