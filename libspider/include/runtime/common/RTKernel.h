/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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
#ifndef SPIDER2_RTKERNEL_H
#define SPIDER2_RTKERNEL_H

/* === Include(s) === */

#include <cstdint>
#include <cstddef>

namespace spider {

    /* === Class definition === */

    class RTKernel {
    public:

        explicit RTKernel(Kernel kernel) : kernel_{ kernel } { };

        RTKernel() = default;

        RTKernel(const RTKernel &) = default;

        RTKernel(RTKernel &&) noexcept = default;

        RTKernel &operator=(const RTKernel &) = default;

        RTKernel &operator=(RTKernel &&) noexcept = default;

        ~RTKernel() = default;

        /* === Method(s) === */

        /* === Operator === */

        void operator()(const int64_t *paramIN, int64_t *paramOUT, void *buffersIN[], void *buffersOUT[]) {
            kernel_(paramIN, paramOUT, buffersIN, buffersOUT);
        }

        /* === Getter(s) === */


        /**
         * @brief Get the ix of the kernel.
         * @return ix of the kernel, if not set return SIZE_MAX.
         */
        inline size_t ix() const {
            return ix_;
        }

        /* === Setter(s) === */

        /**
         * @brief Set the ix of the kernel.
         * @param ix Ix to set.
         */
        inline void setIx(size_t ix) {
            ix_ = ix;
        }

    private:
        Kernel kernel_;         /* = Kernel function to be called when executing the associated vertex = */
        size_t ix_ = SIZE_MAX;  /* = Index of the kernel in the @refitem RTPlatform = */
    };

}

#endif //SPIDER2_RTKERNEL_H
