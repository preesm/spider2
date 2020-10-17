/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_JOBMESSAGE_H
#define SPIDER2_JOBMESSAGE_H

/* === Include(s) === */

#include <scheduling/memory/JobFifos.h>
#include <common/Types.h>

namespace spider {

    class RTKernel;

    /* === Type(s) definition === */

    struct SyncInfo {
        size_t lrtToWait_ = SIZE_MAX;
        size_t jobToWait_ = SIZE_MAX;
    };

    /**
     * @brief Information message about an LRT job to run.
     */
    struct JobMessage {
        std::shared_ptr<JobFifos> fifos_;              /*!< Fifos of the task */
        spider::array<SyncInfo> execConstraints_;       /*!< Array of jobs this job has to wait before running (size is inferior or equal to the number of LRT) */
        spider::unique_ptr<i64> inputParams_;           /*!< Array of static input parameters */
        spider::unique_ptr<bool> synchronizationFlags_; /*!< Array of LRT to notify after job completion (size IS equal to the number of LRT) */
        u32 kernelIx_;                                  /*!< Kernel used for executing the task */
        u32 ix_;                                        /*!< Index of the job */
        u32 taskIx_;                                    /*!< Index of the task associated with the job */
        u32 nParamsOut_;                                /*!< Number of output parameters to be set by this job. */
    };
}

#endif //SPIDER2_JOBMESSAGE_H
