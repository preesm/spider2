/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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

#include <cstring>
#include <runtime/special-kernels/specialKernels.h>
#include <common/Types.h>
#include <common/Exception.h>
#include <common/Logger.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/PE.h>
#include <archi/MemoryInterface.h>
#include <api/archi-api.h>

/* === Function(s) definition === */

void spider::rt::fork(const int64_t *paramsIn, int64_t *, void **in, void **out) {
    const auto inputRate = paramsIn[0];   /* = Rate of the input port (used for sanity check) = */
    const auto outputCount = paramsIn[1]; /* = Number of output = */
    size_t offset = 0;
    for (int64_t i = 0; i < outputCount; ++i) {
        /* == Size of the current output to copy == */
        const auto outputSize = static_cast<size_t>(paramsIn[i + 2]);
        auto *input = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(in[0]) + offset);
        if (input != out[i]) {
            std::memcpy(out[i], input, outputSize);
        }
        offset += outputSize;
    }
    if (offset != static_cast<size_t>(inputRate)) {
        throwSpiderException("Fork has different rates: input[%"
                                     PRId64
                                     "] | output[%"
                                     PRId64
                                     "]", inputRate, offset);
    }
}

void spider::rt::join(const int64_t *paramsIn, int64_t *, void **in, void **out) {
    const auto outputRate = paramsIn[0]; /* = Rate of the output port (used for sanity check) = */
    const auto inputCount = paramsIn[1]; /* = Number of input = */
    size_t offset = 0;
    for (int64_t i = 0; i < inputCount; ++i) {
        /* == Size to copy for current input == */
        const auto inputSize = static_cast<size_t>(paramsIn[i + 2]);
        auto *output = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(out[0]) + offset);
        if (output != in[i]) {
            std::memcpy(output, in[i], inputSize);
        }
        offset += inputSize;
    }
    if (offset != static_cast<size_t>(outputRate)) {
        throwSpiderException("Join has different rates: input[%"
                                     PRId64
                                     "] | output[%"
                                     PRId64
                                     "]", offset, outputRate);
    }
}

void spider::rt::head(const int64_t *paramsIn, int64_t *, void **in, void **out) {
    const auto inputEnd = paramsIn[0]; /* = Number of inputs to consider = */
    size_t offset = 0;
    for (int64_t i = 0; i < inputEnd; ++i) {
        /* == Size to copy for current input == */
        const auto inputSize = static_cast<size_t>(paramsIn[i + 1]);
        auto *output = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(out[i]) + offset);
        if (output != in[i]) {
            std::memcpy(output, in[i], inputSize);
        }
        offset += inputSize;
    }
}

void spider::rt::tail(const int64_t *paramsIn, int64_t *, void **in, void **out) {
    const auto inputCount = static_cast<size_t>(paramsIn[0]);     /* = Number of input = */
    const auto inputStart = static_cast<size_t>(paramsIn[1]);     /* = First input to be considered = */
    const auto inputOffset = static_cast<size_t>(paramsIn[2]);    /* = Offset in the first buffer if any = */
    const auto sizeFirstInput = static_cast<size_t>(paramsIn[3]); /* = Effective size to copy of the first input = */

    /* == Copy the first input with the offset == */
    auto *input = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(in[inputStart]) + inputOffset);
    std::memcpy(out[0], input, sizeFirstInput);

    /* == Do the general case == */
    size_t offset = inputOffset + sizeFirstInput;
    for (size_t i = inputStart + 1; i < inputCount; ++i) {
        const auto inputSize = static_cast<size_t>(paramsIn[i + 4]);
        auto *output = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(out[0]) + offset);
        if (output != in[i]) {
            std::memcpy(output, in[i], inputSize);
        }
        offset += inputSize;
    }
}

void spider::rt::repeat(const int64_t *paramsIn, int64_t *, void **in, void **out) {
    if (in[0] == out[0]) {
        return;
    }
    const auto inputSize = static_cast<size_t>(paramsIn[0]);  /* = Rate of the input port = */
    const auto outputSize = static_cast<size_t>(paramsIn[1]); /* = Rate of the output port = */
    if (inputSize >= outputSize) {
        std::memcpy(out[0], in[0], outputSize);
    } else {
        const auto repeatCount = outputSize / inputSize;
        const auto rest = outputSize % inputSize;
        for (size_t i = 0; i < repeatCount; ++i) {
            auto *output = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(out[0]) + i * inputSize);
            std::memcpy(output, in[0], inputSize);
        }
        if (rest) {
            auto *output = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(out[0]) +
                                                    inputSize * repeatCount);
            std::memcpy(output, in[0], rest);
        }
    }
}

void spider::rt::duplicate(const int64_t *paramsIn, int64_t *, void **in, void **out) {
    const auto outputCount = paramsIn[0]; /* = Number of output = */
    const auto inputSize = paramsIn[1];   /* = Rate of the input port = */
    const auto *input = in[0];            /* = Input buffer = */
    for (int64_t i = 0; i < outputCount; ++i) {
        if (input != out[i]) {
            std::memcpy(out[i], input, static_cast<size_t>(inputSize));
        }
    }
}

void spider::rt::init(const int64_t *paramsIn, int64_t *, void **, void **out) {
    const auto isPersistent = paramsIn[0];
    const auto size = paramsIn[1];
    if (!isPersistent) {
        std::memset(out[0], 0, static_cast<size_t>(size));
    } else {
        const auto address = paramsIn[2];
        const auto *grt = archi::platform()->spiderGRTPE();
        auto *memInterface = grt->cluster()->memoryInterface();
        auto *buffer = memInterface->read(static_cast<u64>(address), 1);
        if (out[0] != buffer) {
            memcpy(out[0], buffer, static_cast<size_t>(size));
        }
        if (log::enabled<log::MEMORY>()) {
            log::info<log::MEMORY>("INIT for address %p and size %ld.\n", out[0], size);
        }
    }
}

void spider::rt::end(const int64_t *paramsIn, int64_t *, void **in, void **) {
    if (paramsIn[0]) {
        const auto size = paramsIn[1];
        const auto address = paramsIn[2];
        const auto *grt = archi::platform()->spiderGRTPE();
        auto *memInterface = grt->cluster()->memoryInterface();
        auto *buffer = memInterface->read(static_cast<u64>(address), 1);
        if (in[0] != buffer) {
            memcpy(buffer, in[0], static_cast<size_t>(size));
        }
        if (log::enabled<log::MEMORY>()) {
            log::info<log::MEMORY>("END for address %p and size %ld.\n", in[0], size);
        }
    }
}

void spider::rt::externIn(const int64_t *, int64_t *, void **, void **) { }

void spider::rt::externOut(const int64_t *paramsIn, int64_t *, void **in, void **) {
    const auto bufferIndex = paramsIn[0];
    const auto size = paramsIn[1];
    auto *buffer = archi::platform()->getExternalBuffer(static_cast<size_t>(bufferIndex));
    if (in[0] != buffer) {
        memcpy(buffer, in[0], static_cast<size_t>(size));
    }
}
