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
#ifndef SPIDER2_SCHEDULER_H
#define SPIDER2_SCHEDULER_H

/* === Include(s) === */

#include <runtime/interface/Message.h>
#include <scheduling/schedule/Schedule.h>
#include <graphs/pisdf/Graph.h>

namespace spider {

    /* === Forward declaration === */

    class ScheduleTask;

    class ScheduleVertexTask;

    /* === Class definition === */

    class Scheduler {
    public:

        explicit Scheduler(pisdf::Graph *graph);

        virtual ~Scheduler() = default;

        /* === Method(s) === */

        /**
         * @brief Perform the mapping and scheduling of a given graph.
         * @param jitSend Type of schedule strategy: true  -> send job just after scheduling.
         *                                                    maximize resource utilization.
         *                                           false -> send jobs after every jobs have been scheduled.
         *                                                    minimize synchronization.
         * @return @refitem Schedule.
         */
        virtual Schedule &schedule(bool jitSend) = 0;

        /**
         * @brief Update internal state of the scheduler (mostly for dynamic applications)
         */
        virtual void update() = 0;

        /**
         * @brief Clears scheduler.
         */
        virtual void clear();

        /* === Getter(s) === */

        /**
         * @brief Returns the @refitem Schedule owned by the Scheduler.
         * @return const reference to @refitem Schedule.
         */
        inline Schedule &schedule() {
            return schedule_;
        }

    protected:
        Schedule schedule_;
        pisdf::Graph *graph_ = nullptr;

        /* === Protected struct(s) === */

        struct DataDependency {
            ScheduleTask *task_ = nullptr;
            PE *sender_ = nullptr;
            uint64_t size_ = 0;

            DataDependency(ScheduleTask *task, PE *pe, uint64_t size) : task_{ task }, sender_{ pe }, size_{ size } { }

            DataDependency(const DataDependency &) = default;

            DataDependency(DataDependency &&) = default;

            DataDependency &operator=(const DataDependency &) = default;

            DataDependency &operator=(DataDependency &&) = default;
        };

        /* === Protected method(s) === */

        /**
         * @brief Compute the minimum start time possible for a given vertex.
         * @param vertex  Vertex to evaluate.
         * @return Minimum start time for the vertex.
         */
        uint64_t computeMinStartTime(const pisdf::Vertex *vertex);

        /**
         * @brief Default vertex mapper that try to best fit.
         * @param vertex Vertex to map.
         */
        virtual void vertexMapper(const pisdf::Vertex *vertex);

        /**
         * @brief Compute the minimum start time possible for a given vertex.
         * @param task  Vertex to evaluate.
         * @return Minimum start time for the vertex.
         */
        static uint64_t computeMinStartTime(ScheduleTask *task);

        template<class SkipPredicate>
        PE *findBestPEFit(Cluster *cluster,
                          uint_fast64_t minStartTime,
                          uint_fast64_t execTime,
                          SkipPredicate skipPredicate);

        /**
         * @brief Inserts a communication task on given cluster.
         *        The method may modify the dependency array of the task passed in parameter.
         * @param cluster      Pointer to the cluster.
         * @param kernel       Pointer to the kernel to use.
         * @param comTime      Estimated time of the communication (in s).
         * @param taskToUpdate Pointer to the task sending data.
         * @param direction    Direction of the communication.
         * @return send task.
         */
        ScheduleTask *insertSendTask(Cluster *cluster, RTKernel *kernel, uint64_t comTime, ScheduleTask *taskToUpdate);

        /**
         * @brief Inserts a communication task on given cluster.
         *        The method may modify the dependency array of the task passed in parameter.
         * @param cluster      Pointer to the cluster.
         * @param kernel       Pointer to the kernel to use.
         * @param comTime      Estimated time of the communication (in s).
         * @param pos          Position of the dependency to replace.
         * @param taskToUpdate Pointer to the task receiving data.
         * @param direction    Direction of the communication.
         * @return end time of the mapped task.
         */
        uint64_t
        insertRecvTask(Cluster *cluster, RTKernel *kernel, uint64_t comTime, size_t pos, ScheduleTask *taskToUpdate);

        /**
         * @brief Schedules inter cluster communications (if any are needed).
         * @param task           Pointer to the task.
         * @param dependencies   Reference to the data dependencies of the task.
         * @param mappedCluster  Pointer on which the task is mapped.
         */
        void
        scheduleCommunications(ScheduleVertexTask *task, vector<DataDependency> &dependencies, Cluster *mappedCluster);

        /**
         * @brief Build a vector of Data dependency. Only the dependencies with exchange of data > 0 are taken into
         *        account.
         * @param task  Pointer to the task.
         * @return vector of @refitem DataDependency.
         */
        static vector<DataDependency> getDataDependencies(ScheduleVertexTask *task);

        /**
         * @brief Default task mapper that try to best fit.
         * @param task Pointer to the task to map.
         */
        virtual void taskMapper(ScheduleVertexTask *task);
    };

    /**
     * @brief Make a new scheduler based on the scheduling algorithm.
     * @param algorithm Algorithm type (see @refitem SchedulingAlgorithm).
     * @param graph  Pointer to the graph.
     * @return unique_ptr of the created scheduler.
     */
    unique_ptr<Scheduler> makeScheduler(SchedulingAlgorithm algorithm, pisdf::Graph *graph);
}
#endif //SPIDER2_SCHEDULER_H
