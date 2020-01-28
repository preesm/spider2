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
#ifndef SPIDER2_SCHEDULESTATS_H
#define SPIDER2_SCHEDULESTATS_H

/* === Include(s) === */

#include <cstdint>
#include <containers/vector.h>

namespace spider {

    namespace sched {

        /* === Class definition === */

        class Stats {
        public:

            Stats();

            ~Stats() = default;

            /* === Method(s) === */

            /**
             * @brief Reset all stats values to 0.
             */
            void reset();

            /**
             * @brief Return the maximum span of all PE of the platform.
             * @return max span of all PE
             */
            inline uint64_t makespan() const;

            /**
             * @brief Compute the utilization factor of a PE.
             * @remark utilization factor is defined as (load(PE) / makespan())
             * @param ix PE to evaluate.
             * @return utilization factor of the PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline double utilizationFactor(size_t ix) const;

            /* === Getter(s) === */

            /**
             * @brief Return the scheduled start time of a given PE.
             * @param ix  PE to check.
             * @return start time of given PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline uint64_t startTime(size_t ix) const;

            /**
             * @brief Return the scheduled end time of a given PE.
             * @param ix  PE to check.
             * @return end time of given PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline uint64_t endTime(size_t ix) const;

            /**
             * @brief Return the load time of a given PE.
             * @param ix  PE to check.
             * @return load time of given PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline uint64_t loadTime(size_t ix) const;

            /**
             * @brief Return the idle time of a given PE.
             * @param ix  PE to check.
             * @return idle time of given PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline uint64_t idleTime(size_t ix) const;

            /**
             * @brief Return the schedule span of a given PE.
             * @param ix  PE to check.
             * @return makespan of given PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline uint64_t makespan(size_t ix) const;

            /**
             * @brief Return the number job mapped on given PE.
             * @param ix PE to evaluate.
             * @return job count of the PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline size_t jobCount(size_t ix) const;

            /**
             * @brief Return the minimum start time among the different PE.
             * @return min start time
             */
            inline uint64_t minStartTime() const;

            /**
             * @brief Return the maximum end time among the different PE.
             * @return max end time
             */
            inline uint64_t maxEndTime() const;

            /* === Setter(s) === */

            inline void updateStartTime(size_t ix, uint64_t time);

            inline void updateEndTime(size_t ix, uint64_t time);

            inline void updateLoadTime(size_t ix, uint64_t time);

            inline void updateIDLETime(size_t ix, uint64_t time);

            inline void updateJobCount(size_t ix, uint32_t incValue = 1);

        private:
            spider::sbc::vector<uint64_t, StackID::SCHEDULE> startTimeVector_;
            spider::sbc::vector<uint64_t, StackID::SCHEDULE> endTimeVector_;
            spider::sbc::vector<uint64_t, StackID::SCHEDULE> loadTimeVector_;
            spider::sbc::vector<uint64_t, StackID::SCHEDULE> idleTimeVector_;
            spider::sbc::vector<size_t, StackID::SCHEDULE> jobCountVector_;
            uint64_t minStartTime_ = UINT64_MAX;
            uint64_t maxEndTime_ = 0;

            /* === Private method(s) === */
        };

        /* === Inline method(s) === */

        uint64_t Stats::makespan() const {
            return maxEndTime_ - minStartTime_;
        }

        double Stats::utilizationFactor(size_t ix) const {
            const auto &load = static_cast<double>(loadTime(ix));
            const auto &span = static_cast<double>(makespan());
            return load / span;
        }

        uint64_t Stats::startTime(size_t ix) const {
            return startTimeVector_.at(ix);
        }

        uint64_t Stats::endTime(size_t ix) const {
            return endTimeVector_.at(ix);
        }

        uint64_t Stats::loadTime(size_t ix) const {
            return loadTimeVector_.at(ix);
        }

        uint64_t Stats::idleTime(size_t ix) const {
            return idleTimeVector_.at(ix);
        }

        uint64_t Stats::makespan(size_t ix) const {
            /* == Here, we use the at method on the first vector to check the validity of PE value then we use
             * random access operator on second vector due to it being faster. == */
            return startTimeVector_.at(ix) - endTimeVector_[ix];
        }

        size_t Stats::jobCount(size_t ix) const {
            return jobCountVector_.at(ix);
        }

        uint64_t Stats::minStartTime() const {
            return minStartTime_;
        }

        uint64_t Stats::maxEndTime() const {
            return maxEndTime_;
        }

        void Stats::updateStartTime(size_t ix, uint64_t time) {
            auto &startTime = startTimeVector_.at(ix);
            startTime = time;
            minStartTime_ = std::min(startTime, minStartTime_);
        }

        void Stats::updateEndTime(size_t ix, uint64_t time) {
            auto &endTime = endTimeVector_.at(ix);
            endTime = time;
            maxEndTime_ = std::max(endTime, maxEndTime_);
        }

        void Stats::updateLoadTime(size_t ix, uint64_t time) {
            auto &loadTime = loadTimeVector_.at(ix);
            loadTime += time;
        }

        void Stats::updateIDLETime(size_t ix, uint64_t time) {
            auto &idleTime = idleTimeVector_.at(ix);
            idleTime += time;
        }

        void Stats::updateJobCount(size_t ix, uint32_t incValue) {
            auto &jobCount = jobCountVector_.at(ix);
            jobCount += incValue;
        }
    }
}


#endif //SPIDER2_SCHEDULESTATS_H
