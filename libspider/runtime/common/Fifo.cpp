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

#include <runtime/common/Fifo.h>
#include <archi/Platform.h>
#include <archi/MemoryInterface.h>

#define cast_buffer_woffset(buffer, offset) (reinterpret_cast<void *>(reinterpret_cast<uintptr_t>((buffer)) + (offset)))

/* === Static function(s) === */

namespace spider {

    /* === Static read functions declaration === */

    static void *readDummy(array_view<Fifo>::iterator &it, MemoryInterface *) {
        it++;
        return nullptr;
    }

    static void *readExternBuffer(array_view<Fifo>::iterator &it, MemoryInterface *) {
        const auto fifo = *(it++);
        if (!fifo.size_) {
            return nullptr;
        }
        return cast_buffer_woffset(archi::platform()->getExternalBuffer(fifo.address_), fifo.offset_);
    }

    static void *readBuffer(array_view<Fifo>::iterator &it, MemoryInterface *memoryInterface) {
        const auto fifo = *(it++);
        if (!fifo.size_) {
            return nullptr;
        }
        return cast_buffer_woffset(memoryInterface->read(fifo.address_, fifo.count_), fifo.offset_);
    }

    static void *readRepeatBuffer(array_view<Fifo>::iterator &, MemoryInterface *);

    static void *readMergedBuffer(array_view<Fifo>::iterator &, MemoryInterface *);

    /* === Static array of read functions === */

    using fifo_fun_t = void *(*)(array_view<Fifo>::iterator &it, MemoryInterface *);

    static std::array<fifo_fun_t, FIFO_ATTR_COUNT> readFunctions = {{ readBuffer,             /*!< RW_ONLY  */
                                                                            readBuffer,       /*!< RW_OWN   */
                                                                            readExternBuffer, /*!< RW_EXT   */
                                                                            readMergedBuffer, /*!< R_MERGE  */
                                                                            readRepeatBuffer, /*!< R_REPEAT */
                                                                            readDummy,        /*!< W_SINK   */
                                                                            readBuffer,       /*!< RW_AUTO  */
                                                                            readDummy         /*!< DUMMY    */ }};

    /* === Static read functions definition === */

    static void *readMergedBuffer(array_view<Fifo>::iterator &it, MemoryInterface *memoryInterface) {
        const auto mergedFifo = *(it++);
        auto *mergedBuffer = memoryInterface->allocate(mergedFifo.address_, mergedFifo.size_, mergedFifo.count_);
#ifndef NDEBUG
        if (!mergedBuffer) {
            throwNullptrException();
        }
#endif
        const auto lastIt = std::next(it, mergedFifo.offset_);
        auto *destBuffer = reinterpret_cast<char *>(mergedBuffer);
        while (it != lastIt) {
            const auto fifo = *it;
            auto *buffer = readFunctions[static_cast<u8>(fifo.attribute_)](it, memoryInterface);
            std::memcpy(destBuffer, buffer, fifo.size_);
            destBuffer += fifo.size_;
        }
        return mergedBuffer;
    }

    static void *readRepeatBuffer(array_view<Fifo>::iterator &it, MemoryInterface *memoryInterface) {
        const auto repeatFifo = *(it++);
        auto *repeatBuffer = memoryInterface->allocate(repeatFifo.address_, repeatFifo.size_, repeatFifo.count_);
#ifndef NDEBUG
        if (!repeatBuffer) {
            throwNullptrException();
        }
#endif
        const auto inputFifo = *it;
        auto *inputBuffer = readFunctions[static_cast<u8>(inputFifo.attribute_)](it, memoryInterface);
        if (inputBuffer && (inputFifo.size_ >= repeatFifo.size_)) {
            std::memcpy(repeatBuffer, inputBuffer, repeatFifo.size_);
        } else if (inputBuffer) {
            const auto repeatCount = repeatFifo.size_ / inputFifo.size_;
            const auto rest = repeatFifo.size_ % inputFifo.size_;
            for (size_t i = 0; i < repeatCount; ++i) {
                auto *output = cast_buffer_woffset(repeatBuffer, i * inputFifo.size_);
                std::memcpy(output, inputBuffer, inputFifo.size_);
            }
            if (rest) {
                auto *output = cast_buffer_woffset(repeatBuffer, repeatCount * inputFifo.size_);
                std::memcpy(output, inputBuffer, rest);
            }
        }
        return repeatBuffer;
    }

    /* === Static allocate functions === */

    static void *allocBuffer(array_view<Fifo>::iterator &it, MemoryInterface *memoryInterface) {
        const auto fifo = *(it++);
        return memoryInterface->allocate(fifo.address_, fifo.size_, fifo.count_);
    }

    /* === Static array of allocate functions === */

    static std::array<fifo_fun_t, FIFO_ATTR_COUNT> allocFunctions = {{ readBuffer,             /*!< RW_ONLY  */
                                                                             allocBuffer,      /*!< RW_OWN   */
                                                                             readExternBuffer, /*!< RW_EXT   */
                                                                             readDummy,        /*!< R_MERGE  */
                                                                             readDummy,        /*!< R_REPEAT */
                                                                             allocBuffer,      /*!< W_SINK   */
                                                                             allocBuffer,      /*!< RW_AUTO  */
                                                                             readDummy         /*!< DUMMY    */}};
}

/* === Function(s) definition === */

spider::array<void *>
spider::getInputBuffers(const array_view<Fifo> &fifos, MemoryInterface *memoryInterface) {
    size_t count = 0u;
    auto it = std::begin(fifos);
    while (it != std::end(fifos)) {
        count += it->attribute_ != FifoAttribute::DUMMY;
        it = it->attribute_ == FifoAttribute::R_MERGE ? it + it->offset_ + 1 : it + 1;
    }
    auto result = spider::array<void *>{ count, nullptr, StackID::RUNTIME };
    /* = yeah it is ugly, but avoids changing everything else and keeps const at high level = */
    auto fifoIt = const_cast<array_view<Fifo>::iterator>(std::begin(fifos));
    auto resIt = std::begin(result);
    while (resIt != std::end(result)) {
        if (fifoIt->attribute_ == FifoAttribute::DUMMY) {
            fifoIt++;
        } else {
            (*resIt) = readFunctions[static_cast<u8>(fifoIt->attribute_)](fifoIt, memoryInterface);
            resIt++;
        }
    }
    return result;
}

spider::array<void *>
spider::getOutputBuffers(const array_view<Fifo> &fifos, MemoryInterface *memoryInterface) {
    auto result = spider::array<void *>{ fifos.size(), nullptr, StackID::RUNTIME };
    /* = yeah it is ugly, but avoids changing everything else and keeps const at high level = */
    auto fifoIt = const_cast<array_view<Fifo>::iterator>(std::begin(fifos));
    for (auto it = std::begin(result); it != std::end(result); ++it) {
        (*it) = allocFunctions[static_cast<u8>((*fifoIt).attribute_)](fifoIt, memoryInterface);
    }
    return result;
}
