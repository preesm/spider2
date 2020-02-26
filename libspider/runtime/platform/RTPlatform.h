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
#ifndef SPIDER2_RTPLATFORM_H
#define SPIDER2_RTPLATFORM_H

/* === Include(s) === */

#include <containers/array.h>
#include <containers/vector.h>
#include <runtime/common/RTKernel.h>
#include <thread/Thread.h>
#include <algorithm>

namespace spider {

    /* === Forward declaration(s) === */

    class RTRunner;

    class RTCommunicator;

    /* === Class definition === */

    class RTPlatform {
    public:

        explicit RTPlatform(size_t runnerCount = 0) : runnerArray_{ runnerCount, nullptr, StackID::RUNTIME } { }

        virtual ~RTPlatform();

        /* === Method(s) === */

        /**
         * @brief Adds an @refitem RTRunner to the RTPlatform.
         * @remark if runner is nullptr, nothing happens.
         * @param runner Pointer to the runner to add.
         * @throws std::out_of_range index of the runner is not valid.
         */
        void addRunner(RTRunner *runner);

        /**
         * @brief Add a @refitem RTKernel to the RTPlatform.
         * @remark If kernel already exist, this return the current ix of the kernel.
         * @param kernel  Kernel to add.
         * @return index of the kernel inside the platform, SIZE_MAX if failure.
         */
        inline size_t addKernel(RTKernel *kernel) {
            if (!kernel) {
                return SIZE_MAX;
            }
            /* == Search if seach Kernel does not already exists == */
            auto res = std::find(runtimeKernelVector_.begin(), runtimeKernelVector_.end(), kernel);
            if (res == runtimeKernelVector_.end()) {
                kernel->setIx(runtimeKernelVector_.size());
                runtimeKernelVector_.emplace_back(kernel);
                res = runtimeKernelVector_.end() - 1;
            }
            return (*res)->ix();
        }

        /**
        * @brief Send the LRT_START_ITERATION notification to every runners.
        */
        void sendStartIteration() const;

        /**
         * @brief Send the LRT_END_ITERATION notification to every runners.
         */
        void sendEndIteration() const;

        /**
         * @brief Wait for every runners to send the LRT_FINISHED_ITERATION notification.
         */
        void waitForRunnersToFinish() const;

        /**
         * @brief send JOB_CLEAR_QUEUE notification to every runners.
         */
        void sendClearToRunners() const;

        virtual void createRunnerRessource(RTRunner *) = 0;

        virtual void waitForRunnerToBeReady() = 0;

        /* === Getter(s) === */

        /**
         * @brief Returns pointer to @refitem RTRunner of index ix.
         * @param ix Index of the RTRunner to fetch
         * @return pointer to the corresponding @refitem RTRunner.
         * @throws std::out_of_range if index is not valid.
         */
        inline RTRunner *runner(size_t ix) const {
            return runnerArray_.at(ix);
        }

        /**
         * @brief Returns runtime communicator of the platform.
         * @return pointer to the @refitem RTCommunicator.
         */
        inline RTCommunicator *communicator() const {
            return communicator_;
        }

        /**
         * @brief Returns the vector of runtime kernels of the platform.
         * @return const reference to a spider::vector of pointer of @refitem RTKernel.
         */
        inline const spider::vector<RTKernel *> &runtimeKernelVector() const {
            return runtimeKernelVector_;
        }

        /**
         * @brief Returns the runtime kernel associated with given index if possible.
         * @param ix Index of the kernel to fetch.
         * @return pointer to the @refitem RTKernel if it exists, nullptr else.
         */
        inline RTKernel *getKernel(size_t ix) const {
            if (ix >= runtimeKernelVector_.size()) {
                return nullptr;
            }
            return runtimeKernelVector_[ix];
        }

        /* === Setter(s) === */

        /**
         * @brief Set the @refitem RTCommunicator of the platform.
         * @remark If communicator is nullptr, nothing happens.
         * @param communicator Pointer to the communicator to set.
         * @throws spider::Exception if platform already has a communicator.
         */
        inline void setCommunicator(RTCommunicator *communicator) {
            if (!communicator) {
                return;
            }
            if (communicator_) {
                throwSpiderException("already existing runtime communicator.");
            }
            communicator_ = communicator;
        }

    protected:
        array<RTRunner *> runnerArray_;                                /* = Array of RTRunner = */
        sbc::vector<RTKernel*, StackID::RUNTIME> runtimeKernelVector_; /* = Vector of RTKernel = */
        RTCommunicator *communicator_ = nullptr;                               /* = Communicator of the RTPlatform = */
    };
}

#endif //SPIDER2_RTPLATFORM_H
