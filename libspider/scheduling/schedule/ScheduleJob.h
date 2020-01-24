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
#include <algorithm>
#include <containers/containers.h>
#include <runtime/interface/Message.h>
#include <scheduling/allocator/FifoAllocator.h>

/* === Forward declarations === */

namespace spider {

    namespace pisdf {
        class Vertex;
    }

    namespace sched {

        class Schedule;

        /* === Enumeration === */

        /**
         * @brief State a scheduled job can take.
         */
        enum class JobState {
            PENDING,   /*!< Job is waiting to be scheduled */
            READY,     /*!< Job is ready to be run */
            RUNNING,   /*!< Job is currently running */
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

            Job();

            explicit Job(pisdf::Vertex *vertex);

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
            inline void setScheduleConstraint(size_t ix, size_t lrtIx) {
                if (ix != SIZE_MAX && ix != ix_) {
                    scheduleConstraintsArray_[lrtIx] = ix;
                }
            }

            /**
             * @brief Creates a JobMessage out of the schedule job information.
             * @param schedule Pointer to the schedule.
             * @return @refitem JobMessage
             */
            JobMessage createJobMessage(const Schedule *schedule);

            inline void addOutputFIFO(RTFifo fifo) {
                outputFifoVector_.emplace_back(fifo);
            }

            /* === Getter(s) === */

            /**
             * @brief Return the job constraint associated to the LRT ix.
             * @param lrtIx  Ix of the LRT.
             * @return ix of the @refitem sched::Job this job is constrained on given LRT.
             */
            inline size_t scheduleJobConstraintOnLRT(size_t lrtIx) const {
                return scheduleConstraintsArray_[lrtIx];
            }

            inline size_t numberOfConstraints() const {
                return scheduleConstraintsArray_.size() -
                       static_cast<size_t>(std::count(scheduleConstraintsArray_.begin(),
                                                      scheduleConstraintsArray_.end(),
                                                      SIZE_MAX));
            }

            inline const spider::array<size_t> &scheduleConstraintsArray() const {
                return scheduleConstraintsArray_;
            }

            /**
             * @brief Get the vertex of the job.
             * @return pointer to the @refitem pisdf::Vertex of the job.
             */
            inline const pisdf::Vertex *vertex() const {
                return vertex_;
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

            inline RTFifo outputFIFO(size_t ix) const {
                return outputFifoVector_.at(ix);
            }

            /* === Setter(s) === */

            /**
             * @brief Sets the flag of notification for a given LRT.
             * @warning no bound checking is performed.
             * @param lrtIx  Ix of the LRT to notify (or not).
             * @param value  Value to set to the flag.
             */
            inline void setRunnerToNotify(size_t lrtIx, bool value) {
                runnerToNotifyArray_[lrtIx] = value;
            }

            /**
             * @brief Set the vertex of the job.
             * @remark This method will overwrite current value.
             * @param vertex Pointer to the vertex to set.
             */
            void setVertex(const pisdf::Vertex *vertex);

            /**
             * @brief Set the ix of the job.
             * @remark This method will overwrite current value.
             * @param ix Ix to set.
             */
            inline void setIx(size_t ix) {
                ix_ = ix;
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
            stack_vector(outputFifoVector_, RTFifo, StackID::SCHEDULE);
            spider::array<size_t> scheduleConstraintsArray_;
            spider::array<bool> runnerToNotifyArray_;
            JobMappingInfo mappingInfo_;
            const pisdf::Vertex *vertex_ = nullptr;
            size_t ix_ = SIZE_MAX;
            JobState state_ = JobState::PENDING;
        };
    }
}

#endif //SPIDER2_SCHEDULEJOB_H
