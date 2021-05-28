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
#ifndef SPIDER2_JOBFIFOS_H
#define SPIDER2_JOBFIFOS_H

/* === Include(s) === */

#include <runtime/common/Fifo.h>
#include <containers/array_view.h>
#include <memory/unique_ptr.h>

namespace spider {

    /* === Class definition === */

    class JobFifos {
    public:
        JobFifos(u32 inputFifoCount, u32 outputFifoCount);

        ~JobFifos() = default;

        JobFifos(JobFifos &&) = default;

        JobFifos &operator=(JobFifos &&) = default;

        JobFifos(const JobFifos &) = delete;

        JobFifos &operator=(const JobFifos &) = delete;

        /* === Method(s) === */

        /* === Getter(s) === */

        /**
         * @brief Return an array handle on the input @refitem RTFifo.
         * @return @refitem spider::array_handle of @refitem RTFifo.
         */
        array_view<Fifo> inputFifos() const;

        /**
         * @brief Return an array handle on the output @refitem RTFifo.
         * @return @refitem spider::array_handle of @refitem RTFifo.
         */
        array_view<Fifo> outputFifos() const;

        /**
         * @brief Get the input fifo at index ix.
         * @remark no bound check is performed (use @refitem TaskMemory::inputFifos for safer access).
         * @param ix Index of the fifo.
         * @return corresponding input fifo else.
         */
        Fifo inputFifo(size_t ix) const;

        /**
         * @brief Get the output fifo at index ix.
         * @remark no bound check is performed (use @refitem TaskMemory::outputFifos for safer access).
         * @param ix Index of the fifo.
         * @return corresponding output fifo else.
         */
        Fifo outputFifo(size_t ix) const;

        /**
         * @brief Returns the number of input fifos.
         * @return number of input fifos.
         */
        size_t inputFifoCount() const;

        /**
         * @brief Returns the number of output fifos.
         * @return number of input fifos.
         */
        size_t outputFifoCount() const;

        /* === Setter(s) === */

        /**
         * @brief Sets the input fifo at position ix.
         * @param ix    Position to set the fifo.
         * @param fifo  Fifo to set.
         */
        void setInputFifo(size_t ix, Fifo fifo);

        /**
         * @brief Sets the output fifo at position ix.
         * @param ix    Position to set the fifo.
         * @param fifo  Fifo to set.
         */
        void setOutputFifo(size_t ix, Fifo fifo);

    private:
        spider::unique_ptr<Fifo> inputFifos_;
        spider::unique_ptr<Fifo> outputFifos_;
        u32 inputFifoCount_ = 0;
        u32 outputFifoCount_ = 0;
    };
}
#endif //SPIDER2_JOBFIFOS_H
