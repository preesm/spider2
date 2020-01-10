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
#ifndef SPIDER2_JITMSRTRUNNER_H
#define SPIDER2_JITMSRTRUNNER_H

/* === Include(s) === */

#include <runtime/runner/RTRunner.h>
#include <runtime/interface/Notification.h>

namespace spider {

    /* === Class definition === */

    class JITMSRTRunner final : public RTRunner {
    public:

        JITMSRTRunner(PE *attachedPE, size_t runnerIx, int32_t affinity = -1) : RTRunner(attachedPE,
                                                                                         runnerIx,
                                                                                         affinity) { }

        ~JITMSRTRunner() override = default;

        /* === Method(s) === */

        void run(bool infiniteLoop) override;

        void begin() override;

        /* === Getter(s) === */

        /* === Setter(s) === */

    private:
        size_t jobCount_ = SIZE_MAX;
        bool shouldBroadcast_ = false;

        /**
         * @brief Run a given job.
         * @param job  Job to run.
         */
        void runJob(const JobMessage &job);

        /**
         * @brief Checks if a given job is runnable given its dependencies.
         * @param message  Job to evaluate.
         * @return true if the job is runnable, false else.
         */
        bool isJobRunnable(const JobMessage &message) const;

        /**
         * @brief Checks if notification is available and read it.
         * @param blocking  Flag indicating if check should be blocking or not.
         * @return true if notification was read, false else.
         */
        bool readNotification(bool blocking);

        /**
         * @brief Update the local job stamp value of a given runner.
         * @param lrtIx          Ix of the local runtime to update.
         * @param jobStampValue  Value of the job stamp to update.
         */
        void updateJobStamp(size_t lrtIx, size_t jobStampValue);
    };
}


#endif //SPIDER2_JITMSRTRUNNER_H
