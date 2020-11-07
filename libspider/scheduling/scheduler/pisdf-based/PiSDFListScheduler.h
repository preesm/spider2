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
#ifndef SPIDER2_PISDFLISTSCHEDULER_H
#define SPIDER2_PISDFLISTSCHEDULER_H

/* === Include(s) === */

#include <scheduling/scheduler/Scheduler.h>

namespace spider {

    class RTInfo;

    namespace pisdf {
        struct DependencyIterator;
        struct DependencyInfo;

        class GraphFiring;
    }

    namespace sched {

        /* === Class definition === */

        class PiSDFListScheduler final : public Scheduler {
        public:

            PiSDFListScheduler();

            ~PiSDFListScheduler() noexcept override = default;

            /* === Method(s) === */

            void schedule(pisdf::GraphHandler *graphHandler, Schedule *schedule) override;

            void clear() override;

            /* === Getter(s) === */

            /* === Setter(s) === */

        private:

            /* === Types definition === */

            struct ListTask {
                pisdf::Vertex *vertex_;
                pisdf::GraphFiring *handler_;
                i32 level_;
                u32 firing_;
            };

            /* === Members === */

            spider::vector<ListTask> sortedTaskVector_;

            /* == Private method(s) === */

            /**
             * @brief Reset unscheduled task from previous schedule iteration.
             */
            void resetUnScheduledTasks();

            /**
             * @brief Recursively add vertices into the sortedTaskVector_ vector.
             * @param graphHandler  Top level graph base;
             */
            void recursiveAddVertices(spider::pisdf::GraphHandler *graphHandler);

            /**
             * @brief Create @refitem ListScheduler::ListTask for every non-scheduled vertex.
             * @remark The attribute @refitem pisdf::Vertex::scheduleTaskIx_ of the vertex is set to the last position of
             *         sortedTaskVector_.
             * @param vertex  Pointer to the vertex associated.
             * @param firing  Firing of the vertex.
             * @param handler Pointer to the base of the vertex.
             */
            void createListTask(pisdf::Vertex *vertex, u32 firing, pisdf::GraphFiring *handler);

            /**
             * @brief Recursively set all consumer of this vertex as not schedulable.
             * @param handler Pointer to the base of the vertex.
             * @param vertex  Pointer to the vertex associated.
             * @param firing  Firing of the vertex.
             */
            static void recursiveSetNonSchedulable(spider::vector<ListTask> &sortedTaskVector,
                                                   const pisdf::GraphFiring *handler,
                                                   const pisdf::Vertex *vertex,
                                                   u32 firing);

            /**
             * @brief Compute recursively the schedule level used to sort the vertices for scheduling.
             * The criteria used is based on the critical execution time path.
             * @example:
             *         input graph:
             *             A (100) -> B(200)
             *                     -> C(100) -> D(100)
             *                               -> E(300) -> G(100)
             *                                  F(100) ->
             *         result:
             *           level(A) = 0
             *           level(B) = max(level(A) + time(A); 0) = 100
             *           level(C) = max(level(A) + time(A); 0) = 100
             *           level(D) = max(level(C) + time(C); 0) = 200
             *           level(E) = max(level(C) + time(C); 0) = 200
             *           level(G) = max(level(E) + time(E); level(F) + time(F)) = 500
             * @param listTask  Reference to the current @refitem ListVertex evaluated.
             * @return level value of the vertex for its given firing.
             */
            static i32 computeScheduleLevel(spider::vector<ListTask> &sortedTaskVector, ListTask &listTask);

            static void computeLevelForDep(const pisdf::DependencyInfo &dep, spider::vector<ListTask> &sortedTaskVector, i32 &level);

            static i64
            computeMinExecTime(const RTInfo *rtInfo, const spider::vector<std::shared_ptr<pisdf::Param>> &params);

            /**
             * @brief Sort the list of vertices.
             */
            void sortVertices();

            /**
             * @brief Removes all the non executable vertices from the list for scheduling.
             * @return number of non schedulable tasks.
             */
            size_t countNonSchedulableTasks();

        };
    }
}
#endif //SPIDER2_SRLESSGREEDYSCHEDULER_H
