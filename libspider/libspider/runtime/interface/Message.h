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
#ifndef SPIDER2_MESSAGE_H
#define SPIDER2_MESSAGE_H

/* === Include(s) === */

#include <containers/containers.h>
#include <containers/array.h>

namespace spider {

    /* === Type(s) definition === */

    using JobConstraint = std::pair<uint32_t, int32_t>; /*!< Constraint of a job (Ix of the LRT running the job, Ix of the job). */

    /* === Structure(s) definition === */

    /**
    * @brief Information message about an LRT job to run.
    */
    struct JobMessage {

        JobMessage() = default;

        JobMessage(const JobMessage &) = default;

        JobMessage(JobMessage &&) noexcept = default;

        JobMessage &operator=(const JobMessage &) = default;

        JobMessage &operator=(JobMessage &&) noexcept = default;

        ~JobMessage() = default;

        /* === Struct member(s) === */

        spider::array<bool> LRTs2Notify_;         /*!< Vector of LRT to notify after job completion (size IS equal to the number of LRT) */
        spider::array<JobConstraint> jobs2Wait_;  /*!< Vector of jobs this job has to wait before running (size is inferior or equal to the number of LRT) */
        spider::array<int64_t> inputParams_;
        // TODO: add other job description.
    };

    /**
     * @brief Message containing dynamic parameters values set by a job.
     */
    struct ParameterMessage {

        ParameterMessage() = default;

        ParameterMessage(const ParameterMessage &) = default;

        ParameterMessage(ParameterMessage &&) noexcept = default;

        ParameterMessage(uint32_t vertexIx, spider::vector<int64_t> params) : vertexIx_{ vertexIx },
                                                                              params_{ std::move(params) } {

        }

        ParameterMessage &operator=(const ParameterMessage &) = default;

        ParameterMessage &operator=(ParameterMessage &&) noexcept = default;

        ~ParameterMessage() = default;

        /* === Struct member(s) === */

        uint32_t vertexIx_ = UINT32_MAX; /*!< Ix of the vertex setting the parameter(s) */
        spider::vector<int64_t> params_; /*!< Vector of parameter(s) value */
        // TODO: Using raw pointer (int64_t*) + uint32_t, size drop to 16 bytes instead of 40.
        // TODO: HOWEVER, vector are safer and easier to manipulate, maybe extra cost is worth?
    };

    /**
     * @brief Message containing trace information.
     */
    struct TraceMessage {

        TraceMessage() = default;

        TraceMessage(const TraceMessage &) = default;

        TraceMessage(TraceMessage &&) noexcept = default;

        TraceMessage &operator=(const TraceMessage &) = default;

        TraceMessage &operator=(TraceMessage &&) noexcept = default;

        ~TraceMessage() = default;

        /* === Struct member(s) === */

        uint32_t vertexIx_ = UINT32_MAX;   /*!< Ix of the vertex */
        uint64_t startTime_ = UINT64_MAX;  /*!< Start time of the job */
        uint64_t endTime_ = 0;             /*!< End time of the job */
    };

}

#endif //SPIDER2_MESSAGE_H
