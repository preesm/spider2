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
#ifndef SPIDER_STACKALLOCATOR_H
#define SPIDER_STACKALLOCATOR_H

/* === Includes === */

#include <cstdint>
#include <limits>
#include <memory/abstract-allocators/AbstractAllocator.h>
#include <memory/static-allocators/FreeListStaticAllocator.h>
#include <memory/static-allocators/LIFOStaticAllocator.h>
#include <memory/static-allocators/LinearStaticAllocator.h>
#include <memory/dynamic-allocators/FreeListAllocator.h>
#include <memory/dynamic-allocators/GenericAllocator.h>
#include <spider-api/config.h>
#include <common/EnumIterator.h>

/* === Define(s) === */

/* === Structure(s) === */

/* === Namespace === */

namespace spider {

    /* === Enumeration(s) === */

    /**
     * @brief Allocator types
     */
    enum class AllocatorType {
        FREELIST,        /*!< (Dynamic) FreeList type allocator */
        GENERIC,         /*!< (Dynamic) Generic type allocator (=malloc) */
        LIFO_STATIC,     /*!< (Static) LIFO type allocator */
        FREELIST_STATIC, /*!< (Static) FreeList type allocator */
        LINEAR_STATIC    /*!< (Static) Linear type allocator */
    };

    /* == Functions used for creating / destroying allocators == */

    /**
     * @brief Trick used to do function partial template specialization.
     * @remark This trick is originated from https://www.fluentcpp.com/2017/08/15/function-templates-partial-specialization-cpp/
     * @tparam T  Type of the partial specialization
     */
    template<AllocatorType T>
    struct type {
    };

    inline AbstractAllocator *&allocator(StackID stack) {
        static std::array<AbstractAllocator *, ALLOCATOR_COUNT> allocatorArray = { nullptr };
        return allocatorArray[static_cast<std::int32_t>(stack)];
    }

    template<AllocatorType Type, class ...Args>
    inline StackID createAllocator(type<Type>, StackID, Args &&...) {
        throwSpiderException("unsupported allocator type.");
    }

    template<class ...Args>
    inline void createAllocator(type<AllocatorType::GENERIC>, StackID stack, Args &&... args) {
        if (!allocator(stack)) {
            allocator(stack) = new GenericAllocator(std::forward<Args>(args)...);
        }
    }

    template<class ...Args>
    inline void createAllocator(type<AllocatorType::FREELIST>, StackID stack, Args &&... args) {
        if (!allocator(stack)) {
            allocator(stack) = new FreeListAllocator(std::forward<Args>(args)...);
        }
    }

    template<class ...Args>
    inline void createAllocator(type<AllocatorType::FREELIST_STATIC>, StackID stack, Args &&... args) {
        if (!allocator(stack)) {
            allocator(stack) = new FreeListStaticAllocator(std::forward<Args>(args)...);
        }
    }

    template<class ...Args>
    inline void createAllocator(type<AllocatorType::LINEAR_STATIC>, StackID stack, Args &&... args) {
        if (!allocator(stack)) {
            allocator(stack) = new LinearStaticAllocator(std::forward<Args>(args)...);
        }
    }

    template<class ...Args>
    inline void createAllocator(type<AllocatorType::LIFO_STATIC>, StackID stack, Args &&... args) {
        if (!allocator(stack)) {
            allocator(stack) = new LIFOStaticAllocator(std::forward<Args>(args)...);
        }
    }

    inline void freeAllocators() {
        for (auto stack : spider::EnumIterator<StackID>()) {
            delete allocator(stack);
            allocator(stack) = nullptr;
        }
    }

    /* == Functions used for allocating (constructing) / deallocating(destroying) == */

    /**
     * @brief Allocate raw memory buffer on given stack.
     * @tparam T    Type of the pointer to allocate.
     * @param size  Size of the buffer to allocate.
     * @param stack Stack on which the buffer should be allocated, see #SpiderStack
     * @return pointer to allocated buffer, nullptr if size is 0.
     */
    // TODO: merge this with Allocator
    template<typename T>
    inline T *allocate(StackID stack, std::uint64_t size = 1) {
        /* == Allocate buffer with (size + 1) to store stack identifier == */
        size = size * sizeof(T);
        auto *&worker = allocator(stack);
        if (!worker) {
            throwSpiderException("Allocating memory with non-initialized allocator.");
        }
        auto buffer = reinterpret_cast<uintptr_t>(worker->allocate(size + sizeof(std::uint64_t)));

        /* == Return allocated buffer == */
        if (buffer) {
            reinterpret_cast<std::uint64_t *>(buffer)[0] = static_cast<std::uint64_t>(stack);
            buffer = buffer + sizeof(std::uint64_t);
        }
        return reinterpret_cast<T *>(buffer);
    }

    /**
     * @brief  Construct a previously allocated object
     * @attention This method does not allocate memory, use @refitem Allocator::allocate first
     * @tparam T     Type of the object to construct
     * @tparam Args  Packed arguments list for construction
     * @param ptr    Reference pointer of the object to be constructed
     * @param args   Arguments use for by the constructor of the object
     */
    template<typename T, class... Args>
    inline void construct(T *ptr, Args &&... args) {
        new(ptr) T(std::forward<Args>(args)...);
    }

    /**
     * @brief Destroy an object
     * @attention This method does not deallocate memory of the pointer, use @refitem Allocator::deallocate
     * @tparam T  Type of the object to destroy
     * @param ptr Reference pointer to the object to destroy
     */
    template<typename T>
    inline void destroy(T *ptr) {
        if (ptr) {
            ptr->~T();
        }
    }

    /**
     * @brief Deallocate raw memory pointer
     * @attention This method does not destroy the object, use @refitem Allocator::destroy
     * @param ptr Raw pointer to deallocate
     */
    inline void deallocate(void *ptr) {
        if (!ptr) {
            return;
        }
        /* == Retrieve stack id == */
        auto *originalPtr = (reinterpret_cast<char *>(ptr) - sizeof(std::uint64_t));
        auto stackId = static_cast<StackID>(((std::uint64_t *) (originalPtr))[0]);

        /* == Deallocate the pointer == */
        allocator(stackId)->deallocate(originalPtr);
    }
}

#endif /* SPIDER_STACKALLOCATOR_H */

