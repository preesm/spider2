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
#ifndef SPIDER2_BESTFITMAPPER_H
#define SPIDER2_BESTFITMAPPER_H

/* === Include(s) === */

#include <scheduling/mapper/Mapper.h>
#include <scheduling/schedule/ScheduleStats.h>
#include <scheduling/task/SyncTask.h>

namespace spider {

    class PE;

    class RTInfo;

    namespace sched {

        /* === Class definition === */

        class BestFitMapper final : public Mapper {
        public:
            BestFitMapper() : Mapper() { };

            ~BestFitMapper() noexcept override = default;

            /* === Method(s) === */

            void map(Task *task, Schedule *schedule) override;

            /* === Getter(s) === */

            /* === Setter(s) === */

        private:

            struct MappingResult {
                const PE *mappingPE{ nullptr };
                ufast64 startTime{ UINT_FAST64_MAX };
                ufast64 endTime{ UINT_FAST64_MAX };
                ufast64 scheduleCost{ UINT_FAST64_MAX };
                bool needToAddCommunication{ false };
            };

            /* === Private method(s) === */

            /**
             * @brief Find which PE is the best fit inside a given cluster.
             * @param cluster       Cluster to go through.
             * @param stats         Schedule information about current usage of PEs.
             * @param minStartTime  Lower bound for start time.
             * @param task          Pointer to the task.
             * @return best fit PE found, nullptr if no fit was found.
             */
            const PE *
            findBestFitPE(const Cluster *cluster, const Stats &stats, ufast64 minStartTime, const Task *task) const;

            void mapCommunications(Task *task, const Cluster *cluster, Schedule *schedule);

            SyncTask *insertCommunicationTask(const Cluster *cluster,
                                              const Cluster *distCluster,
                                              ufast64 dataSize,
                                              Task *previousTask,
                                              SyncType type,
                                              Schedule *schedule);
        };
    }
}

#endif //SPIDER2_BESTFITMAPPER_H
