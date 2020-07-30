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
#ifndef SPIDER2_SCHEDULERLEGACY_H
#define SPIDER2_SCHEDULERLEGACY_H

/* === Include(s) === */

#include <runtime/interface/Message.h>
#include <scheduling/schedule/ScheduleLegacy.h>
#include <graphs/pisdf/Graph.h>
#include <common/Types.h>

namespace spider {

    /* === Forward declaration === */

    class ScheduleTask;

    class FifoAllocator;

    /* === Class definition === */

    class SchedulerLegacy {
    public:

        enum ScheduleMode {
            JIT_SEND = 0, /*!< Send job just after scheduling. Maximize resource utilization. */
            DELAYED_SEND, /*!< Send jobs after every jobs have been scheduled. Minimize synchronizations. */
        };

        explicit SchedulerLegacy(pisdf::Graph *graph, ScheduleMode mode = DELAYED_SEND, FifoAllocator *allocator = nullptr);

        virtual ~SchedulerLegacy() = default;

        /* === Method(s) === */

        /**
         * @brief Set the task memory allocator to be used.
         * @param allocator pointer to the fifo allocator.
         */
        void setAllocator(FifoAllocator *allocator);

        /**
         * @brief Set the scheduling and execution mode.
         * @param mode  Mode to set.
         */
        void setMode(ScheduleMode mode);

        /**
         * @brief Update internal state of the scheduler (mostly for dynamic applications)
         */
        virtual void update() = 0;

        /**
         * @brief Perform the mapping and scheduling of a given graph.
         * @return @refitem Schedule.
         */
        virtual ScheduleLegacy &execute() = 0;

        /**
         * @brief Clears scheduler resources.
         */
        virtual void clear();

        /* === Getter(s) === */

        /**
         * @brief Returns the @refitem Schedule owned by the Scheduler.
         * @return const reference to @refitem Schedule.
         */
        inline ScheduleLegacy &schedule() {
            return schedule_;
        }

    protected:
        ScheduleLegacy schedule_;
        pisdf::Graph *graph_ = nullptr;
        ScheduleMode mode_ = JIT_SEND;
        FifoAllocator *allocator_ = nullptr;
        ufast64 minStartTime_ = 0;

        /* === Protected struct(s) === */

        struct DataDependency {
            ScheduleTask *task_;
            PE *sender_;
            ufast64 size_;
            i32 position_;
        };

        using TimePredicate = i64 (*)(const PE *, const void *);
        using SkipPredicate = bool (*)(const PE *, const void *);

        /* === Protected method(s) === */

        /**
         * @brief Compute the minimum start time possible for a given vertex.
         * @param task  Vertex to evaluate.
         * @return Minimum start time for the vertex.
         */
        ufast64 computeMinStartTime(ScheduleTask *task) const;

        PE *findBestPEFit(const Cluster *cluster,
                          ufast64 minStartTime,
                          const void *info,
                          TimePredicate execTimePredicate,
                          SkipPredicate skipPredicate);

        /**
         * @brief Inserts a communication task on given cluster.
         *        The method may modify the dependency array of the task passed in parameter.
         * @param cluster      Pointer to the cluster.
         * @param distCluster  Pointer to the distant cluster.
         * @param dataSize     Size in bytes of the data to transfer.
         * @param previousTask Pointer to the task sending data.
         * @param type         Type of the task to insert (@refitem TaskType::SYNC_SEND or @refitem TaskType::SYNC_RECEIVE).
         * @return pointer to the created ScheduleTask.
         */
        ScheduleTask *insertCommunicationTask(Cluster *cluster,
                                              Cluster *distCluster,
                                              ufast64 dataSize,
                                              ScheduleTask *previousTask,
                                              TaskType type);

        /**
         * @brief Schedules inter cluster communications (if any are needed).
         * @param task           Pointer to the task.
         * @param dependencies   Reference to the data dependencies of the task.
         * @param cluster  Pointer on which the task is mapped.
         */
        void scheduleCommunications(ScheduleTask *task, vector<DataDependency> &dependencies, Cluster *cluster);

        /**
         * @brief Build a vector of Data dependency. Only the dependencies with exchange of data > 0 are taken into
         *        account.
         * @param task  Pointer to the task.
         * @return vector of @refitem DataDependency.
         */
        static vector<DataDependency> getDataDependencies(const ScheduleTask *task);

        /**
         * @brief Default task mapper that try to best fit.
         * @param task Pointer to the task to map.
         */
        virtual void mapTask(ScheduleTask *task);

        /**
         * @brief Allocate virtual addresses for a given task.
         * @param task  Pointer to the task.
         */
        virtual void allocateTaskMemory(ScheduleTask *task);
    };

    /**
     * @brief Make a new scheduler based on the scheduling algorithm.
     * @param algorithm Algorithm type (see @refitem SchedulingAlgorithm).
     * @param graph  Pointer to the graph.
     * @return unique_ptr of the created scheduler.
     */
    spider::unique_ptr<SchedulerLegacy> makeScheduler(SchedulingPolicy algorithm, pisdf::Graph *graph);
}
#endif //SPIDER2_SCHEDULERLEGACY_H
