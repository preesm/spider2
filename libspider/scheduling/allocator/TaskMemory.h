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
#ifndef SPIDER2_TASKMEMORY_H
#define SPIDER2_TASKMEMORY_H

/* === Include(s) === */

#include <runtime/common/RTFifo.h>
#include <containers/array_handle.h>
#include <memory/unique_ptr.h>

namespace spider {

    /* === Class definition === */

    class TaskMemory {
    public:
        TaskMemory(size_t inputFifoCount, size_t outputFifoCount);

        ~TaskMemory() = default;

        TaskMemory(TaskMemory &&) = default;

        TaskMemory &operator=(TaskMemory &&) = default;

        TaskMemory(const TaskMemory &) = delete;

        TaskMemory &operator=(const TaskMemory &) = delete;

        /* === Method(s) === */

        /* === Getter(s) === */

        /**
         * @brief Return an array handle on the input @refitem RTFifo.
         * @return @refitem spider::array_handle of @refitem RTFifo.
         */
        array_handle<RTFifo> inputFifos() const;

        /**
         * @brief Return an array handle on the output @refitem RTFifo.
         * @return @refitem spider::array_handle of @refitem RTFifo.
         */
        array_handle<RTFifo> outputFifos() const;

        /**
         * @brief Get the input fifo at index ix.
         * @remark no bound check is performed (use @refitem TaskMemory::inputFifos for safer access).
         * @param ix Index of the fifo.
         * @return corresponding input fifo else.
         */
        RTFifo inputFifo(size_t ix) const;

        /**
         * @brief Get the output fifo at index ix.
         * @remark no bound check is performed (use @refitem TaskMemory::outputFifos for safer access).
         * @param ix Index of the fifo.
         * @return corresponding output fifo else.
         */
        RTFifo outputFifo(size_t ix) const;

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
        void setInputFifo(size_t ix, RTFifo fifo);

        /**
         * @brief Sets the output fifo at position ix.
         * @param ix    Position to set the fifo.
         * @param fifo  Fifo to set.
         */
        void setOutputFifo(size_t ix, RTFifo fifo);

    private:
        unique_ptr<RTFifo> inputFifos_;
        unique_ptr<RTFifo> outputFifos_;
        size_t inputFifoCount_ = 0;
        size_t outputFifoCount_ = 0;
    };
}
#endif //SPIDER2_TASKMEMORY_H