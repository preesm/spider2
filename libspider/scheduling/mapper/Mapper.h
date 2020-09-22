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
#ifndef SPIDER2_MAPPER_H
#define SPIDER2_MAPPER_H

/* === Include(s) === */

#include <common/Types.h>

namespace spider {

    namespace pisdf {
        class Vertex;
    }

    namespace sched {

        class Task;

        class TaskVertex;

        class Schedule;

        /* === Class definition === */

        class Mapper {
        public:
            Mapper() = default;

            virtual ~Mapper() noexcept = default;

            /* === Method(s) === */

            /**
             * @brief Map a task onto available resources.
             * @param task     pointer to the task to map.
             * @param schedule pointer to the schedule to update.
             * @throw @refitem spider::Exception if the mapper was unable to find any processing elements for the task.
             */
            virtual void map(TaskVertex *task, Schedule *schedule) = 0;

            /* === Getter(s) === */

            /* === Setter(s) === */

            inline void setStartTime(ufast64 time) { startTime_ = time; }

        protected:
            ufast64 startTime_{ 0U };

            /* === Protected method(s) === */

            /**
             * @brief Compute the minimum start time possible for a given task.
             * @param task    Pointer to the task.
             * @return value of the minimum start time possible
             */
            ufast64 computeStartTime(const Task *task) const;

        };
    }
}
#endif //SPIDER2_MAPPER_H
