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
/* === Include(s) === */

#include <scheduling/task/TaskFifos.h>
#include <containers/vector.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::TaskFifos::TaskFifos(size_t inputFifoCount, size_t outputFifoCount) :
        inputFifos_{ spider::allocate<Fifo, StackID::SCHEDULE>(inputFifoCount) },
        outputFifos_{ spider::allocate<Fifo, StackID::SCHEDULE>(outputFifoCount) },
        inputFifoCount_{ inputFifoCount },
        outputFifoCount_{ outputFifoCount } {

}

spider::array_handle<spider::Fifo> spider::TaskFifos::inputFifos() const {
    return make_handle(inputFifos_.get(), inputFifoCount_);
}

spider::array_handle<spider::Fifo> spider::TaskFifos::outputFifos() const {
    return make_handle(outputFifos_.get(), outputFifoCount_);
}

size_t spider::TaskFifos::inputFifoCount() const {
    return inputFifoCount_;
}

size_t spider::TaskFifos::outputFifoCount() const {
    return outputFifoCount_;
}

spider::Fifo spider::TaskFifos::inputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= inputFifoCount_) {
        throwSpiderException("accessing out_of_range input fifo");
    }
    return inputFifos_.get()[ix];
#else
    return inputFifos_.get()[ix];
#endif
}

spider::Fifo spider::TaskFifos::outputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= outputFifoCount_) {
        throwSpiderException("accessing out_of_range output fifo");
    }
    return outputFifos_.get()[ix];
#else
    return outputFifos_.get()[ix];
#endif
}

void spider::TaskFifos::setInputFifo(size_t ix, spider::Fifo fifo) {
    if (inputFifos_ && (ix < inputFifoCount_)) {
        inputFifos_.get()[ix] = fifo;
    }
}

void spider::TaskFifos::setOutputFifo(size_t ix, spider::Fifo fifo) {
    if (outputFifos_ && (ix < outputFifoCount_)) {
        outputFifos_.get()[ix] = fifo;
    }
}
