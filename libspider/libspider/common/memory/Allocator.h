/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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

#include "FreeListStaticAllocator.h"
#include "GenericAllocator.h"
#include "FreeListAllocator.h"

/**
 * @brief Stack ids
 */
enum class SpiderStack : std::uint64_t {
    PISDF_STACK,     /*!< Stack used for PISDF graph (should be static) */
    ARCHI_STACK,     /*!< Stack used for architecture (should be static) */
    PLATFORM_STACK,  /*!< Stack used by the platform (should be static) */
    TRANSFO_STACK,   /*!< Stack used for graph transformations */
    SCHEDULE_STACK,  /*!< Stack used for scheduling */
    SRDAG_STACK,     /*!< Stack used for SRDAG graph */
    LRT_STACK,        /*!< Stack used by LRTs */
    NEW_STACK,        /*!< Stack used by calls to new / delete */
};

/**
 * @brief Allocator types
 */
enum class SpiderAllocatorType {
    FREELIST,        /*!< (Dynamic) FreeList type allocator */
    GENERIC,         /*!< (Dynamic) Generic type allocator (=malloc) */
    LIFO_STATIC,     /*!< (Static) LIFO type allocator */
    FREELIST_STATIC, /*!< (Static) FreeList type allocator */
    LINEAR_STATIC    /*!< (Static) Linear type allocator */
};

typedef struct SpiderStackConfig {
    const char *name;
    SpiderAllocatorType allocatorType = SpiderAllocatorType::GENERIC;
    std::uint64_t size = 0;
    std::uint64_t alignment = 0;
} SpiderStackConfig;


namespace Allocator {
    void stackInit(SpiderStack stack, SpiderStackConfig cfg) {
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
    void construct(T *ptr, Args &&... args) {
        if (ptr) {
            new((void *) ptr) T(std::forward<Args>(args)...);
        }
    }

    /**
     * @brief Destroy an object
     * @attention This method does not deallocate memory of the pointer, use @refitem Allocator::deallocate
     * @tparam T  Type of the object to destroy
     * @param ptr Reference pointer to the object to destroy
     */
    template<typename T>
    void destroy(T *ptr) {
        if (ptr) {
            ptr->~T();
        }
    }

    /**
     * @brief Allocate raw memory buffer on given stack.
     * @tparam T    Type of the pointer to allocate.
     * @param size  Size of the buffer to allocate.
     * @param stack Stack on which the buffer should be allocated, see #SpiderStack
     * @return pointer to allocated buffer, nullptr if size is 0.
     */
    template<typename T>
    T *allocate(SpiderStack stack, std::uint64_t size = 1) {
        /* 0. Allocate buffer with (size + 1) to store stack identifier */
        size = size * sizeof(T);
        char *buffer = nullptr;
        switch (stack) {
            case SpiderStack::PISDF_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
                break;
            case SpiderStack::ARCHI_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
                break;
            case SpiderStack::PLATFORM_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
                break;
            case SpiderStack::TRANSFO_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
                break;
            case SpiderStack::SCHEDULE_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
                break;
            case SpiderStack::SRDAG_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
                break;
            case SpiderStack::LRT_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
                break;
            case SpiderStack::NEW_STACK:
//                buffer = (char *) newStack.alloc(size + sizeof(std::uint64_t));
                break;
            default:
                break;
        }
        /* 1. Return allocated buffer */
        if (buffer) {
            ((std::uint64_t *) (buffer))[0] = (std::uint64_t) stack;
            buffer = buffer + sizeof(std::uint64_t);
        }
        return (T *) buffer;
    }

    /**
     * @brief Deallocate raw memory pointer
     * @attention This method does not destroy the object, use @refitem Allocator::destroy
     * @param ptr Raw pointer to deallocate
     */
    void deallocate(void *ptr) {
        if (ptr) {
            /* 0. Retrieve stack id */
            auto *originalPtr = ((char *) ptr - sizeof(std::uint64_t));
            std::uint64_t stackId = ((std::uint64_t *) (originalPtr))[0];
            /* 1. Deallocate the pointer */
        }
    }
}

//void *operator new(std::size_t size) {
//    return Allocator::allocate<void>(size, SpiderStack::NEW_STACK);
//}
//
//void *operator new[](std::size_t size) {
//    return Allocator::allocate<void>(size, SpiderStack::NEW_STACK);
//}
//
//void operator delete(void *ptr) _GLIBCXX_USE_NOEXCEPT {
//    Allocator::deallocate(ptr);
//}
//
//void operator delete[](void *ptr) _GLIBCXX_USE_NOEXCEPT {
//    Allocator::deallocate(ptr);
//}

#endif /* SPIDER_STACKALLOCATOR_H */

