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
#ifndef SPIDER2_SRLESSLISTSCHEDULER_H
#define SPIDER2_SRLESSLISTSCHEDULER_H

/* === Include(s) === */

#include <scheduling/scheduler_legacy/srdagless/SRLessScheduler.h>
#include <containers/vector.h>
#include <common/Types.h>

namespace spider {

    /* === Class definition === */

    class SRLessListScheduler : public SRLessScheduler {
    public:

        ~SRLessListScheduler() override = default;

        /* === Method(s) === */

        /**
         * @brief Add vertices of the member @refitem pisdf::Graph in sorted order into the sortedVertexVector_
         * for mappingScheduling.
         * @remark This method also updates the number of jobs in the member @refitem sched::Schedule.
         */
        void update() override;

        /**
         * @brief ListScheduler is an abstract class regrouping common method for list based schedulers but it can not
         *        actually perform a scheduling. The scheduling and mapping algorithm must be implemented in derived
         *        class.
         */
        Schedule &execute() override = 0;

        void clear() override;

    protected:
        struct ListTask {
            ScheduleTask *task_;
            u32 firing_;
            ifast32 level_;
        };
        vector<ListTask> sortedTaskVector_;
        size_t lastSchedulableTask_ = 0;
        size_t lastScheduledTask_ = 0;

        /* === Protected method(s) === */

        explicit SRLessListScheduler(pisdf::Graph *graph);

    private:

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
         * @param listTask       Pointer to the current @refitem ListVertex evaluated.
         * @param listVertexVector Vector of @refitem ListVertex to evaluate.
         * @return
         */
        ifast32 computeScheduleLevel(ListTask &listTask, vector<ListTask> &listVertexVector) const;

        void setNextVerticesNonSchedulable(pisdf::Vertex *vertex, vector<ListTask> &listVertexVector) const;

        void sortTasks();
    };
}

#endif //SPIDER2_SRLESSLISTSCHEDULER_H
