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
#include <containers/StlContainers.h>

/* === Forward declarations === */

namespace spider {

    /* === Enumeration === */

    /**
     * @brief State a scheduled job can take.
     */
    enum class JobState {
        RUNNING,   /*!< Job is currently running */
        PENDING,   /*!< Job is waiting to be run */
        FINISHED   /*!< Job has finished its execution */
    };

    /**
     * @brief Mapping information of the Job.
     */
    struct JobMappingInfo {
        std::uint32_t PEIx = UINT32_MAX;        /*!< ix of the mapped PE in its cluster */
        std::uint32_t clusterIx = UINT32_MAX;   /*!< ix of the mapped cluster */
        std::uint32_t LRTIx = UINT32_MAX;       /*!< ix of the LRT handling the job */
        std::uint64_t startTime = UINT64_MAX;   /*!< mapping start time */
        std::uint64_t endTime = UINT64_MAX;     /*!< mapping end time */
    };

    /* === Class definition === */

    class ScheduleJob {
    public:

        ScheduleJob() = delete;

        explicit ScheduleJob(std::uint32_t ix);

        ScheduleJob(std::uint32_t ix,
                    std::uint32_t vertexIx,
                    std::uint32_t PEIx,
                    std::uint32_t clusterIx,
                    std::uint32_t LRTIx);

        ~ScheduleJob() = default;

        /* === Method(s) === */

        // TODO add construction of ScheduleJobMessage

        /**
         * @brief Add a constraint on another @refitem ScheduleJob to current job.
         * @param job Pointer to the job we are constrained on.
         */
        inline void setConstraint(ScheduleJob *job);

        /* === Getter(s) === */

        inline ScheduleJob *constraint(std::uint32_t lrtIx) const;

        /**
         * @brief Get the ix of the job.
         * @return job ix.
         */
        inline std::uint32_t vertexIx() const;

        /**
         * @brief Get the ix of the job.
         * @return job ix.
         */
        inline std::uint32_t ix() const;

        /**
         * @brief Get the state of the job.
         * @return @refitem Spider::JobState
         */
        inline JobState state() const;

        /**
         * @brief Get the mapping information of the job.
         * @return const reference to the @refitem Spider::JobMappingInfo of the job.
         */
        inline const JobMappingInfo &mappingInfo() const;

        /* === Setter(s) === */

        /**
         * @brief Set the ix of the vertex of the job.
         * @remark This method will overwrite current value.
         * @param ix Ix to set.
         */
        inline void setVertexIx(std::uint32_t ix);

        /**
         * @brief Set the ix of the job.
         * @remark This method will overwrite current value.
         * @param ix Ix to set.
         */
        inline void setIx(std::uint32_t ix);

        /**
         * @brief Set the state of the job.
         * @remark This method will overwrite current value.
         * @param state State to set.
         */
        inline void setState(JobState state);

        /**
        * @brief Set the processing element of the job.
        * @remark This method will overwrite current values.
        * @param PEIx      Processing element ix inside its cluster.
        * @param clusterIx Cluster ix of the PE.
        */
        inline void setMappingPE(std::uint32_t PEIx, std::uint32_t clusterIx);

        /**
        * @brief Set the LRT ix of the LRT that will handle the job.
        * @remark This method will overwrite current values.
        * @param LRTIx  LRT ix.
        */
        inline void setMappingLRT(std::uint32_t LRTIx);

        /**
         * @brief Set the start time of the job.
         * @remark This method will overwrite current value.
         * @param time  Value to set.
         */
        inline void setMappingStartTime(std::uint64_t time);

        /**
         * @brief Set the end time of the job.
         * @remark This method will overwrite current value.
         * @param time  Value to set.
         */
        inline void setMappingEndTime(std::uint64_t time);

    private:
        std::uint32_t vertexIx_ = UINT32_MAX;
        std::uint32_t ix_ = UINT32_MAX;
        JobState state_ = spider::JobState::PENDING;
        JobMappingInfo mappingInfo_;
        spider::vector<ScheduleJob *> constraints_;

        /* === Private method(s) === */
    };

    /* === Inline method(s) === */

    void ScheduleJob::setConstraint(ScheduleJob *job) {
        if (job && job != this) {
            constraints_.at(job->mappingInfo().LRTIx) = job;
        }
    }

    ScheduleJob *ScheduleJob::constraint(std::uint32_t lrtIx) const {
        return constraints_.at(lrtIx);
    }

    std::uint32_t spider::ScheduleJob::vertexIx() const {
        return vertexIx_;
    }

    std::uint32_t spider::ScheduleJob::ix() const {
        return ix_;
    }

    JobState ScheduleJob::state() const {
        return state_;
    }

    const JobMappingInfo &ScheduleJob::mappingInfo() const {
        return mappingInfo_;
    }

    void ScheduleJob::setVertexIx(std::uint32_t ix) {
        vertexIx_ = ix;
    }

    void ScheduleJob::setIx(std::uint32_t ix) {
        ix_ = ix;
    }

    void ScheduleJob::setState(JobState state) {
        state_ = state;
    }

    void ScheduleJob::setMappingPE(std::uint32_t PEIx, std::uint32_t clusterIx) {
        mappingInfo_.PEIx = PEIx;
        mappingInfo_.clusterIx = clusterIx;
    }

    void ScheduleJob::setMappingLRT(std::uint32_t LRTIx) {
        mappingInfo_.LRTIx = LRTIx;
    }

    void ScheduleJob::setMappingStartTime(std::uint64_t time) {
        mappingInfo_.startTime = time;

    }

    void ScheduleJob::setMappingEndTime(std::uint64_t time) {
        mappingInfo_.endTime = time;
    }
}

#endif //SPIDER2_SCHEDULEJOB_H
