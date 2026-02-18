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

#include <scheduling/memory/JobFifos.h>
#include <containers/vector.h>

/* === Method(s) implementation === */

spider::JobFifos::JobFifos(u32 inputFifoCount, u32 outputFifoCount) :
        inputFifos_{ spider::allocate<Fifo, StackID::RUNTIME>(inputFifoCount) },
        outputFifos_{ spider::allocate<Fifo, StackID::RUNTIME>(outputFifoCount) },
        inputFifoCount_{ inputFifoCount },
        outputFifoCount_{ outputFifoCount } {

}

spider::array_view<spider::Fifo> spider::JobFifos::inputFifos() const {
    return make_view(inputFifos_.get(), inputFifoCount_);
}

spider::array_view<spider::Fifo> spider::JobFifos::outputFifos() const {
    return make_view(outputFifos_.get(), outputFifoCount_);
}

size_t spider::JobFifos::inputFifoCount() const {
    return inputFifoCount_;
}

size_t spider::JobFifos::outputFifoCount() const {
    return outputFifoCount_;
}

spider::Fifo spider::JobFifos::inputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= inputFifoCount_) {
        throwSpiderException("accessing out_of_range input fifo");
    }
    return inputFifos_[ix];
#else
    return inputFifos_[ix];
#endif
}

spider::Fifo spider::JobFifos::outputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= outputFifoCount_) {
        throwSpiderException("accessing out_of_range output fifo");
    }
    return outputFifos_[ix];
#else
    return outputFifos_[ix];
#endif
}

void spider::JobFifos::setInputFifo(size_t ix, spider::Fifo fifo) {
    if (inputFifos_ && (ix < inputFifoCount_)) {
        inputFifos_[ix] = fifo;
    }
}

void spider::JobFifos::setOutputFifo(size_t ix, spider::Fifo fifo) {
    if (outputFifos_ && (ix < outputFifoCount_)) {
        outputFifos_[ix] = fifo;
    }
}
