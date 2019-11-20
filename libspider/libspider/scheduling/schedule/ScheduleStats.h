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
#include <containers/StlContainers.h>

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
            inline std::uint64_t makespan() const;

            /**
             * @brief Compute the utilization factor of a PE.
             * @remark utilization factor is defined as (load(PE) / makespan())
             * @param PE PE to evaluate.
             * @return utilization factor of the PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline double utilizationFactor(std::uint32_t PE) const;

            /* === Getter(s) === */

            /**
             * @brief Return the scheduled start time of a given PE.
             * @param PE  PE to check.
             * @return start time of given PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline std::uint64_t startTime(std::uint32_t PE) const;

            /**
             * @brief Return the scheduled end time of a given PE.
             * @param PE  PE to check.
             * @return end time of given PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline std::uint64_t endTime(std::uint32_t PE) const;

            /**
             * @brief Return the load time of a given PE.
             * @param PE  PE to check.
             * @return load time of given PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline std::uint32_t loadTime(std::uint32_t PE) const;

            /**
             * @brief Return the idle time of a given PE.
             * @param PE  PE to check.
             * @return idle time of given PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline std::uint32_t idleTime(std::uint32_t PE) const;

            /**
             * @brief Return the schedule span of a given PE.
             * @param PE  PE to check.
             * @return makespan of given PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline std::uint64_t makespan(std::uint32_t PE) const;

            /**
             * @brief Return the number job mapped on given PE.
             * @param PE PE to evaluate.
             * @return job count of the PE.
             * @throws @refitem std::out_of_range if PE out of range.
             */
            inline std::uint32_t jobCount(std::uint32_t PE) const;

            /**
             * @brief Return the minimum start time among the different PE.
             * @return min start time
             */
            inline std::uint64_t minStartTime() const;

            /**
             * @brief Return the maximum end time among the different PE.
             * @return max end time
             */
            inline std::uint64_t maxEndTime() const;

            /* === Setter(s) === */

            inline void updateStartTime(std::uint32_t PE, std::uint64_t time);

            inline void updateEndTime(std::uint32_t PE, std::uint64_t time);

            inline void updateLoadTime(std::uint32_t PE, std::uint64_t time);

            inline void updateIDLETime(std::uint32_t PE, std::uint64_t time);

            inline void updateJobCount(std::uint32_t PE, std::uint32_t incValue = 1);

        private:
            spider::vector<std::uint64_t> startTimeVector_;
            spider::vector<std::uint64_t> endTimeVector_;
            spider::vector<std::uint64_t> loadTimeVector_;
            spider::vector<std::uint64_t> idleTimeVector_;
            spider::vector<std::uint32_t> jobCountVector_;
            std::uint64_t minStartTime_ = UINT64_MAX;
            std::uint64_t maxEndTime_ = 0;

            /* === Private method(s) === */
        };

        /* === Inline method(s) === */

        std::uint64_t Stats::makespan() const {
            return maxEndTime_ - minStartTime_;
        }

        double Stats::utilizationFactor(std::uint32_t PE) const {
            const double &load = loadTime(PE);
            return load / makespan();
        }

        std::uint64_t Stats::startTime(std::uint32_t PE) const {
            return startTimeVector_.at(PE);
        }

        std::uint64_t Stats::endTime(std::uint32_t PE) const {
            return endTimeVector_.at(PE);
        }

        std::uint32_t Stats::loadTime(std::uint32_t PE) const {
            return loadTimeVector_.at(PE);
        }

        std::uint32_t Stats::idleTime(std::uint32_t PE) const {
            return idleTimeVector_.at(PE);
        }

        std::uint64_t Stats::makespan(std::uint32_t PE) const {
            /* == Here, we use the at method on the first vector to check the validity of PE value then we use
             * random access operator on second vector due to it being faster. == */
            return startTimeVector_.at(PE) - endTimeVector_[PE];
        }

        std::uint32_t Stats::jobCount(std::uint32_t PE) const {
            return jobCountVector_.at(PE);
        }

        std::uint64_t Stats::minStartTime() const {
            return minStartTime_;
        }

        std::uint64_t Stats::maxEndTime() const {
            return maxEndTime_;
        }

        void Stats::updateStartTime(std::uint32_t PE, std::uint64_t time) {
            auto &startTime = startTimeVector_.at(PE);
            startTime = time;
            minStartTime_ = std::min(startTime, minStartTime_);
        }

        void Stats::updateEndTime(std::uint32_t PE, std::uint64_t time) {
            auto &endTime = endTimeVector_.at(PE);
            endTime = time;
            maxEndTime_ = std::max(endTime, maxEndTime_);
        }

        void Stats::updateLoadTime(std::uint32_t PE, std::uint64_t time) {
            auto &loadTime = loadTimeVector_.at(PE);
            loadTime += time;
        }

        void Stats::updateIDLETime(std::uint32_t PE, std::uint64_t time) {
            auto &idleTime = idleTimeVector_.at(PE);
            idleTime += time;
        }

        void Stats::updateJobCount(std::uint32_t PE, std::uint32_t incValue) {
            auto &jobCount = jobCountVector_.at(PE);
            jobCount += incValue;
        }
    }
}


#endif //SPIDER2_SCHEDULESTATS_H
