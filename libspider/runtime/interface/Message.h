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

#include <containers/array.h>
#include <memory/unique_ptr.h>
#include <runtime/common/RTFifo.h>

namespace spider {

    /* === Type(s) definition === */

    struct JobConstraint {
        size_t lrtToWait_ = SIZE_MAX;
        size_t jobToWait_ = SIZE_MAX;
    };

    /* === Structure(s) definition === */

    /**
    * @brief Information message about an LRT job to run.
    */
    struct JobMessage {

        JobMessage() = default;

        JobMessage(const JobMessage &) = delete;

        JobMessage(JobMessage &&) noexcept = default;

        JobMessage &operator=(const JobMessage &) = delete;

        JobMessage &operator=(JobMessage &&) noexcept = default;

        ~JobMessage() = default;

        /* === Struct member(s) === */

        unique_ptr<bool> notificationFlagsArray_; /*!< Array of LRT to notify after job completion (size IS equal to the number of LRT) */
        array<JobConstraint> jobs2Wait_;          /*!< Array of jobs this job has to wait before running (size is inferior or equal to the number of LRT) */
        array<int64_t> inputParams_;              /*!< Array of static input parameters */
        array<RTFifo> inputFifoArray_;            /*!< Array of input FIFO for the job */
        array<RTFifo> outputFifoArray_;           /*!< Array of output FIFO for the job */
        i32 outputParamCount_ = 0;                /*!< Number of output parameters to be set by this job. */
        size_t kernelIx_ = SIZE_MAX;              /*!< Index of the kernel to use to run this job */
        size_t vertexIx_ = SIZE_MAX;              /*!< Index of the vertex associated with the job */
        size_t ix_ = SIZE_MAX;                    /*!< Index of the job */
    };

    /**
     * @brief Message containing dynamic parameters values set by a job.
     */
    struct ParameterMessage {

        ParameterMessage() = default;

        ParameterMessage(const ParameterMessage &) = default;

        ParameterMessage(ParameterMessage &&) noexcept = default;

        ParameterMessage(size_t kernelIx, spider::array<int64_t> params) : vertexIx_{ kernelIx },
                                                                           params_{ std::move(params) } { };

        ParameterMessage &operator=(const ParameterMessage &) = default;

        ParameterMessage &operator=(ParameterMessage &&) noexcept = default;

        ~ParameterMessage() = default;

        /* === Struct member(s) === */

        size_t vertexIx_ = SIZE_MAX;     /*!< Ix of the kernel setting the parameter(s) */
        spider::array<int64_t> params_;  /*!< Array of parameter(s) value */
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
