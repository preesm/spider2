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
#ifndef SPIDER2_SCHEDULEJOB_H
#define SPIDER2_SCHEDULEJOB_H

/* === Include(s) === */

#include <cstdint>
#include <containers/containers.h>
#include <algorithm>
#include <runtime/interface/Message.h>

/* === Forward declarations === */

namespace spider {

    namespace sched {

        /* === Enumeration === */

        /**
         * @brief State a scheduled job can take.
         */
        enum class JobState {
            NON_EXEC,  /*!< Job is currently non-executable */
            RUNNING,   /*!< Job is currently running */
            PENDING,   /*!< Job is waiting to be run */
        };

        /**
         * @brief Mapping information of the TransfoJob.
         */
        struct JobMappingInfo {
            size_t PEIx = SIZE_MAX;          /*!< ix of the mapped PE in its cluster */
            size_t LRTIx = SIZE_MAX;         /*!< ix of the LRT handling the job */
            uint64_t startTime = UINT64_MAX; /*!< mapping start time */
            uint64_t endTime = UINT64_MAX;   /*!< mapping end time */
        };

        /* === Class definition === */

        class Job {
        public:

            Job() = delete;

            explicit Job(size_t ix);

            Job(size_t ix,
                size_t vertexIx,
                size_t PEIx,
                size_t LRTIx);

            Job(const Job &) = default;

            Job(Job &&) noexcept = default;

            Job &operator=(const Job &) = default;

            Job &operator=(Job &&) = default;

            ~Job() = default;

            /* === Method(s) === */

            /**
             * @brief Add a constraint on another @refitem ScheduleJob to current job.
             * @param ix    Ix of the job we're constrained on.
             * @param lrtIx Ix of the LRT the constraint belong.
             */
            inline void setJobConstraint(size_t ix, size_t lrtIx) {
                if (ix != SIZE_MAX && ix != ix_) {
                    jobConstraintVector_.at(lrtIx) = ix;
                }
            }

            /* === Getter(s) === */

            /**
             * @brief Return the job constraint associated to the LRT ix.
             * @param lrtIx  Ix of the LRT.
             * @return
             */
            inline size_t jobConstraintIxOnLRT(size_t lrtIx) const {
                return jobConstraintVector_.at(lrtIx);
            }

            inline size_t numberOfConstraints() const {
                return jobConstraintVector_.size() -
                       static_cast<size_t>(std::count(jobConstraintVector_.begin(),
                                                      jobConstraintVector_.end(),
                                                      SIZE_MAX));
            }

            inline const spider::vector<size_t> &jobConstraintVector() const {
                return jobConstraintVector_;
            }

            inline JobMessage &message() {
                return message_;
            }

            /**
             * @brief Get the ix of the job.
             * @return job ix.
             */
            inline size_t vertexIx() const {
                return vertexIx_;
            }

            /**
             * @brief Get the ix of the job.
             * @return job ix.
             */
            inline size_t ix() const {
                return ix_;
            }

            /**
             * @brief Get the state of the job.
             * @return @refitem Spider::JobState
             */
            inline JobState state() const {
                return state_;
            }

            /**
             * @brief Get the mapping information of the job.
             * @return const reference to the @refitem Spider::JobMappingInfo of the job.
             */
            inline const JobMappingInfo &mappingInfo() const {
                return mappingInfo_;
            }

            /* === Setter(s) === */

            /**
             * @brief Set the ix of the vertex of the job.
             * @remark This method will overwrite current value.
             * @param ix Ix to set.
             */
            inline void setVertexIx(size_t ix) {
                vertexIx_ = ix;
            }

            /**
             * @brief Set the ix of the job.
             * @remark This method will overwrite current value.
             * @param ix Ix to set.
             */
            inline void setIx(size_t ix) {
                ix_ = ix;
                message_.ix_ = ix;
            }

            /**
             * @brief Set the state of the job.
             * @remark This method will overwrite current value.
             * @param state State to set.
             */
            inline void setState(JobState state) {
                state_ = state;
            }

            /**
            * @brief Set the processing element of the job.
            * @remark This method will overwrite current values.
            * @param PEIx  PE ix inside spider.
            */
            inline void setMappingPE(size_t PEIx) {
                mappingInfo_.PEIx = PEIx;
            }

            /**
            * @brief Set the LRT ix of the LRT that will handle the job.
            * @remark This method will overwrite current values.
            * @param LRTIx  LRT ix.
            */
            inline void setMappingLRT(size_t LRTIx) {
                mappingInfo_.LRTIx = LRTIx;
            }

            /**
             * @brief Set the start time of the job.
             * @remark This method will overwrite current value.
             * @param time  Value to set.
             */
            inline void setMappingStartTime(uint64_t time) {
                mappingInfo_.startTime = time;

            }

            /**
             * @brief Set the end time of the job.
             * @remark This method will overwrite current value.
             * @param time  Value to set.
             */
            inline void setMappingEndTime(uint64_t time) {
                mappingInfo_.endTime = time;
            }

        private:
            stack_vector(jobConstraintVector_, size_t, StackID::SCHEDULE);
            size_t vertexIx_ = SIZE_MAX;
            size_t ix_ = SIZE_MAX;
            JobState state_ = JobState::NON_EXEC;
            JobMappingInfo mappingInfo_;
            JobMessage message_;
        };
    }
}

#endif //SPIDER2_SCHEDULEJOB_H
