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
#ifndef SPIDER2_MAPPER_H
#define SPIDER2_MAPPER_H

/* === Include(s) === */

#include <utility>
#include <common/Types.h>
#include <containers/vector.h>
#include <scheduling/schedule/ScheduleStats.h>
#include <graphs-tools/numerical/dependencies.h>

namespace spider {

    class PE;

    namespace sched {

        class Task;

        class PiSDFTask;

        class SRDAGTask;

        class Schedule;

        /* === Class definition === */

        class Mapper {
        public:
            Mapper() = default;

            virtual ~Mapper() noexcept = default;

            /* === Method(s) === */

            /**
             * @brief Map a vertexTask onto available resources.
             * @param task     pointer to the task to map.
             * @param schedule pointer to the schedule to update.
             * @throw @refitem spider::Exception if the mapper was unable to find any processing elements for the vertexTask.
             */
            void map(sched::Task *task, Schedule *schedule);

            /**
             * @brief Map a vertexTask onto available resources.
             * @param task     pointer to the task to map.
             * @param schedule pointer to the schedule to update.
             * @throw @refitem spider::Exception if the mapper was unable to find any processing elements for the vertexTask.
             */
            void map(sched::PiSDFTask *task, Schedule *schedule);

            /* === Getter(s) === */

            /* === Setter(s) === */

            inline void setStartTime(ufast64 time) { startTime_ = time; }

        protected:

            struct MappingResult {
                const PE *mappingPE{ nullptr };
                ufast64 startTime{ UINT_FAST64_MAX };
                ufast64 endTime{ UINT_FAST64_MAX };
                ufast64 scheduleCost{ UINT_FAST64_MAX };
                bool needToAddCommunication{ false };
            };

            /**
             * @brief Find which PE is the best fit inside a given cluster.
             * @param cluster       Cluster to go through.
             * @param stats         Schedule information about current usage of PEs.
             * @param minStartTime  Lower bound for start time.
             * @param task          Pointer to the vertexTask.
             * @return best fit PE found, nullptr if no fit was found.
             */
            virtual const PE *
            findPE(const Cluster *cluster, const Stats &stats, const Task *task, ufast64 minStartTime) const = 0;

        private:

            ufast64 startTime_{ 0U };

            /* === Private method(s) === */

            template<class T>
            void mapImpl(T *task, Schedule *schedule);

            /**
             * @brief Compute the minimum start time possible for a given task.
             * @param task      Pointer to the task.
             * @param schedule  Pointer to the schedule.
             * @return value of the minimum start time possible
             */
            ufast64 computeStartTime(Task *task, const Schedule *schedule, u32 *comRates) const;

            /**
             * @brief Compute the minimum start time possible for a given task.
             * @param task         Pointer to the task.
             * @param schedule     Pointer to the schedule.
             * @return value of the minimum start time possible
             */
            ufast64 computeStartTime(PiSDFTask *task, const Schedule *schedule, u32 *comRates) const;

            /**
             * @brief Compute the communication cost and the data size that would need to be send if a vertexTask is mapped
             *        on a given PE.
             * @param task      Pointer to the task.
             * @param mappedPE  PE on which the vertexTask is currently mapped.
             * @param schedule  Schedule to which the vertexTask is associated.
             * @return pair containing the communication cost as first and the total size of data to send as second.
             */
            static std::pair<ufast64, ufast64> computeCommunicationCost(Task *task,
                                                                        const PE *mappedPE,
                                                                        const Schedule *schedule,
                                                                        const u32 *comRates);

            void mapCommunications(MappingResult &mappingInfo, Task *task, Schedule *schedule) const;

            void mapCommunications(MappingResult &mappingInfo, PiSDFTask *task, Schedule *schedule) const;

            void mapCommunications(MappingResult &mappingInfo,
                                   Task *task,
                                   Task *srcTask,
                                   size_t depIx,
                                   Schedule *schedule) const;

        };
    }
}
#endif //SPIDER2_MAPPER_H
