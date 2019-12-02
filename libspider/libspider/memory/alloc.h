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
        FREELIST,             /*!< (Dynamic) FreeList type allocator */
        GENERIC,              /*!< (Dynamic) Generic type allocator (=malloc) */
        LIFO_STATIC,          /*!< (Static) LIFO type allocator */
        LINEAR_STATIC,        /*!< (Static) Linear type allocator */
        First = FREELIST,     /*!< Sentry for EnumIterator::begin */
        Last = LINEAR_STATIC, /*!< Sentry for EnumIterator::end */
    };

    constexpr size_t ALLOCATOR_COUNT = static_cast<size_t>(AllocatorType::Last) + 1;

    /* == Functions used for creating / destroying allocators == */

    /**
     * @brief Trick used to do function partial template specialization.
     * @remark This trick is originated from https://www.fluentcpp.com/2017/08/15/function-templates-partial-specialization-cpp/
     * @tparam T  Type of the partial specialization
     */
    template<AllocatorType T>
    struct type {
    };

    inline std::array<AbstractAllocator *, STACK_COUNT> &stackAllocatorArray() {
        static std::array<AbstractAllocator *, STACK_COUNT> stackAllocatorArray = { nullptr };
        return stackAllocatorArray;
    }

    inline AbstractAllocator *&getStackAllocator(StackID stack) {
        return stackAllocatorArray()[static_cast<size_t>(stack)];
    }

    template<StackID stack>
    constexpr inline AbstractAllocator *&getStackAllocator() {
        return stackAllocatorArray()[static_cast<int32_t>(stack)];
    }

    template<AllocatorType Type, class ...Args>
    inline StackID createAllocator(type<Type>, StackID, Args &&...) {
        throwSpiderException("unsupported allocator type.");
    }

    template<class ...Args>
    inline void createAllocator(type<AllocatorType::GENERIC>, StackID stack, Args &&... args) {
        if (!getStackAllocator(stack)) {
            getStackAllocator(stack) = new GenericAllocator(std::forward<Args>(args)...);
        }
    }

    template<class ...Args>
    inline void createAllocator(type<AllocatorType::FREELIST>, StackID stack, Args &&... args) {
        if (!getStackAllocator(stack)) {
            getStackAllocator(stack) = new FreeListAllocator(std::forward<Args>(args)...);
        }
    }

    template<class ...Args>
    inline void createAllocator(type<AllocatorType::LINEAR_STATIC>, StackID stack, Args &&... args) {
        if (!getStackAllocator(stack)) {
            getStackAllocator(stack) = new LinearStaticAllocator(std::forward<Args>(args)...);
        }
    }

    template<class ...Args>
    inline void createAllocator(type<AllocatorType::LIFO_STATIC>, StackID stack, Args &&... args) {
        if (!getStackAllocator(stack)) {
            getStackAllocator(stack) = new LIFOStaticAllocator(std::forward<Args>(args)...);
        }
    }

    inline void freeAllocators() {
        for (auto stack : spider::EnumIterator<StackID>()) {
            delete getStackAllocator(stack);
            getStackAllocator(stack) = nullptr;
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
    template<typename T>
    inline T *allocate(StackID stack, size_t size = 1) {
#ifdef _SPIDER_CHECK_ALLOCATOR
        /* == Allocate buffer with (size + 1) to store stack identifier == */
        auto *allocator = getStackAllocator(stack);
        if (!alloc) {
            throwSpiderException("Trying to allocate memory with un-initialized allocator: %ld", static_cast<long>(stack));
        }
        auto buffer = reinterpret_cast<uintptr_t>(allocator->allocate(size * sizeof(T) + sizeof(uint64_t)));
#else
        /* == Allocate buffer with (size + 1) to store stack identifier == */
        auto buffer = reinterpret_cast<uintptr_t>(getStackAllocator(stack)->allocate(size * sizeof(T) + sizeof(uint64_t)));
#endif
        /* == Return allocated buffer == */
        if (buffer) {
            reinterpret_cast<uint64_t *>(buffer)[0] = static_cast<uint64_t>(stack);
            buffer = buffer + sizeof(uint64_t);
            return reinterpret_cast<T *>(buffer);
        }
        return nullptr;
    }

    template<typename T, StackID stack = StackID::GENERAL>
    inline T *allocate(size_t size = 1) {
#ifdef _SPIDER_CHECK_ALLOCATOR
        /* == Allocate buffer with (size + 1) to store stack identifier == */
        auto *allocator = getStackAllocator<stack>();
        if (!alloc) {
            throwSpiderException("Trying to allocate memory with un-initialized allocator: %ld", static_cast<long>(stack));
        }
        auto buffer = reinterpret_cast<uintptr_t>(allocator->allocate(size * sizeof(T) + sizeof(uint64_t)));
#else
        /* == Allocate buffer with (size + 1) to store stack identifier == */
        auto buffer = reinterpret_cast<uintptr_t>(getStackAllocator<stack>()->allocate(size * sizeof(T) + sizeof(uint64_t)));
#endif

        /* == Return allocated buffer == */
        if (buffer) {
            reinterpret_cast<uint64_t *>(buffer)[0] = static_cast<uint64_t>(stack);
            buffer = buffer + sizeof(uint64_t);
            return reinterpret_cast<T *>(buffer);
        }
        return nullptr;
    }

    /**
     * @brief  Construct a previously allocated object
     * @attention This method does not allocate memory, use @refitem spider::allocate first
     * @tparam T     Type of the object to construct
     * @tparam Args  Packed arguments list for construction
     * @param ptr    Reference pointer of the object to be constructed
     * @param args   Arguments used by the constructor of the object
     */
    template<typename T, class... Args>
    inline void construct(T *ptr, Args &&... args) {
        new(ptr) T(std::forward<Args>(args)...);
    }

    /**
     * @brief Destroy an object
     * @attention This method does not deallocate memory of the pointer, use @refitem spider::deallocate
     * @tparam T  Type of the object to destroy
     * @param ptr Reference pointer to the object to destroy
     */
    template<typename T>
    inline void destruct(T *ptr) {
        if (ptr) {
            ptr->~T();
        }
    }

    /**
     * @brief Deallocate raw memory pointer
     * @attention This method does not destroy the object, use @refitem spider::destroy
     * @param ptr Raw pointer to deallocate
     */
    inline void deallocate(void *ptr) {
        if (!ptr) {
            return;
        }
        /* == Retrieve stack id == */
        auto *originalPtr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) - sizeof(uint64_t));
        auto stackId = static_cast<StackID>(reinterpret_cast<uint64_t *>(originalPtr)[0]);

        /* == Deallocate the pointer == */
        getStackAllocator(stackId)->deallocate(originalPtr);
    }

    /**
     * @brief Allocate an object on a given stack and construct it.
     * @remark this function is a shortcut to allocate<T> + construct().
     * @tparam T      Type of the object.
     * @tparam stack  Stack to which the object is to be allocated on (default is GENERAL).
     * @tparam Args   Variadic templates for the ctor arguments.
     * @param args    Ctor arguments.
     * @return pointer to the newly constructed object.
     */
    template<class T, StackID stack = StackID::GENERAL, class ...Args>
    inline T *make(Args &&... args) {
        auto *ptr = spider::allocate<T, stack>();
        spider::construct(ptr, std::forward<Args>(args)...);
        return ptr;
    }

    template<class T, class ...Args>
    inline T *make(StackID stack, Args &&... args) {
        auto *ptr = spider::allocate<T>(stack);
        spider::construct(ptr, std::forward<Args>(args)...);
        return ptr;
    }

    /**
     * @brief Create a c-style array of type T with default value.
     * @tparam T      Type of the array.
     * @tparam stack  Stack on which the array is to be allocated on (default is GENERAL).
     * @param count   Size of the array.
     * @param value   Value to be set in the array.
     * @return pointer to the array.
     */
    template<class T, StackID stack = StackID::GENERAL>
    inline T *make_n(size_t count, const T &value = T()) {
        auto *ptr = spider::allocate<T, stack>(count);
        for (size_t i = 0; i < count; ++i) {
            spider::construct(&ptr[i], value);
        }
        return ptr;
    }

    template<class T>
    inline T *make_n(StackID stack, size_t count, const T &value = T()) {
        auto *ptr = spider::allocate<T>(stack, count);
        for (size_t i = 0; i < count; ++i) {
            spider::construct(&ptr[i], value);
        }
        return ptr;
    }

    /**
     * @brief Wrapper to de-construct and deallocate an object
     * @remark If ptr is nullptr, nothing happen. This function does not reset the value of ptr to nullptr.
     * @tparam T    Type of the object (inferred by the call in most case)
     * @param ptr   Pointer to the object.
     */
    template<class T>
    inline void destroy(T *ptr) {
        if (ptr) {
            /* == Destruct the object pointed by ptr == */
            ptr->~T();

            /* == Retrieve stack id == */
            auto *originalPtr = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(ptr) - sizeof(uint64_t));
            auto stackId = static_cast<StackID>(reinterpret_cast<uint64_t *>(originalPtr)[0]);

            /* == Deallocate the pointer == */
            getStackAllocator(stackId)->deallocate(originalPtr);
        }
    }
}

#endif /* SPIDER_STACKALLOCATOR_H */

