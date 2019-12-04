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
#ifndef SPIDER2_SCHEDULE_H
#define SPIDER2_SCHEDULE_H

/* === Include(s) === */

#include <containers/containers.h>
#include <scheduling/schedule/ScheduleJob.h>
#include <scheduling/schedule/ScheduleStats.h>
#include <functional>

namespace spider {

    namespace sched {

        /* === Class definition === */

        class Schedule {
        public:

            Schedule() = default;

            ~Schedule() = default;

            /* === Method(s) === */

            /**
             * @brief Update schedule stats based on given TransfoJob.
             * @param job  TransfoJob to evaluate.
             */
            void update(sched::Job &job);

            /**
             * @brief Clear schedule jobs.
             */
            void clear();

            /**
             * @brief Reset schedule jobs.
             * @remark Set all job state to @refitem Spider::JobState::PENDING.
             * @remark Statistics of the platform are not modified.
             */
            void reset();

            /**
             * @brief Clear the job vector and initialize it with a given count of jobs.
             * @remark This method is intended to be used with scheduler using SR-DAG representation.
             * @param count Number of jobs to initialize.
             */
            void setJobCount(size_t count);

            /* === Getter(s) === */

            inline size_t jobCount() const;

            /**
             * @brief Get the job vector of the schedule.
             * @return const reference to the job vector
             */
            inline const spider::vector<sched::Job> &jobs() const;

            /**
             * @brief Get a job from its ix.
             * @param ix  Ix of the job to fetch.
             * @return const reference to the job.
             * @throws @refitem std::out_of_range if ix is out of range.
             */
            inline sched::Job &job(size_t ix);

            /**
             * @brief Get a job from its ix.
             * @param ix  Ix of the job to fetch.
             * @return const reference to the job.
             * @throws @refitem std::out_of_range if ix is out of range.
             */
            inline const sched::Job &job(size_t ix) const;

            /**
             * @brief Get the different statistics of the platform.
             * @return const reference to @refitem Stats
             */
            inline const sched::Stats &stats() const;

            /* === Setter(s) === */

        private:
            stack_vector(jobs_, sched::Job, StackID::SCHEDULE);
            sched::Stats stats_;

            /* === Private method(s) === */
        };

        /* === Inline method(s) === */

        size_t Schedule::jobCount() const {
            return jobs_.size();
        }

        const spider::vector<sched::Job> &Schedule::jobs() const {
            return jobs_;
        }

        sched::Job &Schedule::job(size_t ix) {
            return jobs_.at(ix);
        }

        const sched::Job &Schedule::job(size_t ix) const {
            return jobs_.at(ix);
        }

        const sched::Stats &Schedule::stats() const {
            return stats_;
        }
    }
}

#endif //SPIDER2_SCHEDULE_H