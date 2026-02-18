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
#ifndef SPIDER_STACKALLOCATOR_H
#define SPIDER_STACKALLOCATOR_H

/* === Includes === */

#include <memory>
#include <memory/abstract-policies/AbstractAllocatorPolicy.h>
#include <memory/static-policies/LinearStaticAllocator.h>
#include <memory/dynamic-policies/FreeListAllocatorPolicy.h>
#include <memory/dynamic-policies/GenericAllocatorPolicy.h>
#include <memory/allocator.h>
#include <api/global-api.h>
#include <common/EnumIterator.h>

/* === Namespace === */

namespace spider {

    template<StackID stack>
    struct stack_t { };

    /* == Functions used for allocating (constructing) / deallocating(destroying) == */

    /**
     * @brief Allocates data using given stack.
     * @remark This method allocates slightly more than asked for book keeping.
     * @param stack  Pointer to the stack to use.
     * @param size   Size of the buffer element to allocate (ex: sizeof(double).
     * @param n      Number of element of size "size" to allocate.
     * @return allocated buffer on success, nullptr else.
     */
    void *allocate(std::unique_ptr<Stack> &stack, size_t size, size_t n);

    /**
     * @brief Allocate raw memory buffer on given stack.
     * @tparam T    Type of the pointer to allocate.
     * @param size  Size of the buffer to allocate.
     * @param stackId Stack on which the buffer should be allocated, see #SpiderStack
     * @return pointer to allocated buffer, nullptr if size is 0.
     */
    template<typename T>
    inline T *allocate(StackID stackId, size_t n = 1) {
        /* == Allocate buffer with (size + 1) to store stack identifier == */
        return reinterpret_cast<T *>(allocate(stackArray()[static_cast<size_t>(stackId)], sizeof(T), n));
    }

    template<typename T, StackID stackId = StackID::GENERAL>
    inline T *allocate(size_t n = 1) {
        /* == Allocate buffer with (size + 1) to store stack identifier == */
        return reinterpret_cast<T *>(allocate(stackArray()[static_cast<size_t>(stackId)], sizeof(T), n));
    }

    /**
     * @brief Deallocate raw memory pointer
     * @attention This method does not destroy the object, use @refitem spider::destroy
     * @param ptr Raw pointer to deallocate
     */
    void deallocate(void *ptr);

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
        auto *ptr = allocate<T, stack>();
        new(ptr) T(std::forward<Args>(args)...);
        return ptr;
    }

    template<class T, class ...Args>
    inline T *make(StackID stack, Args &&... args) {
        auto *ptr = allocate<T>(stack);
        new(ptr) T(std::forward<Args>(args)...);
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
    inline T *make_n(size_t count, const T &value) {
        auto *ptr = allocate<T, stack>(count);
        for (size_t i = 0; i < count; ++i) {
            new(&ptr[i]) T(value);
        }
        return ptr;
    }

    template<class T, StackID stack = StackID::GENERAL>
    inline T *make_n(size_t count) {
        auto *ptr = allocate<T, stack>(count);
        for (size_t i = 0; i < count; ++i) {
            new(&ptr[i]) T();
        }
        return ptr;
    }

    template<class T>
    inline T *make_n(StackID stack, size_t count, const T &value = T()) {
        auto *ptr = allocate<T>(stack, count);
        for (size_t i = 0; i < count; ++i) {
            new(&ptr[i]) T(value);
        }
        return ptr;
    }

    /**
     * @brief Wrapper to de-construct and deallocate an object
     * @remark If ptr is nullptr, nothing happen. This function reset the value of ptr to nullptr.
     * @tparam T    Type of the object (inferred by the call in most case)
     * @param ptr   Pointer to the object.
     */
    template<class T>
    inline
    typename std::enable_if<std::is_polymorphic<T>::value>::type
    destroy(T *&ptr) {
        if (ptr) {
            /* == Retrieve valid address == */
            auto *fixedPtr = dynamic_cast<void *>(ptr);

            /* == Destruct the object pointed by ptr == */
            ptr->~T();

            /* == Deallocate the pointer == */
            deallocate(fixedPtr);

            /* == Reset pointer to nullptr == */
            ptr = nullptr;
        }
    }

    template<class T>
    inline
    typename std::enable_if<!std::is_polymorphic<T>::value>::type
    destroy(T *&ptr) {
        if (ptr) {
            /* == Destruct the object pointed by ptr == */
            ptr->~T();

            /* == Deallocate the pointer == */
            deallocate(ptr);

            /* == Reset pointer to nullptr == */
            ptr = nullptr;
        }
    }

    template<class T, class ...Args>
    std::shared_ptr<T> make_shared(StackID stack, Args &&... args) {
        return std::allocate_shared<T>(allocator<T>(stack), std::forward<Args>(args)...);
    }

    template<class T, StackID stack = StackID::GENERAL, class ...Args>
    std::shared_ptr<T> make_shared(Args &&... args) {
        return std::allocate_shared<T>(allocator<T>(stack), std::forward<Args>(args)...);
    }

    template<class T>
    std::shared_ptr<T> make_shared(T *&value) {
        return std::shared_ptr<T>(value, destroy<T>);
    }
}

#endif /* SPIDER_STACKALLOCATOR_H */

