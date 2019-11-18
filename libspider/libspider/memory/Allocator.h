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
#include <spider-api/config.h>

/* === Define(s) === */

/* === Enumeration(s) === */

/* === Structure(s) === */

typedef struct AllocatorConfig {
    std::string name = "unnamed-allocator";
    AllocatorType allocatorType = AllocatorType::FREELIST;
    std::uint64_t size = 0;
    std::uint64_t alignment = sizeof(std::uint64_t);
    FreeListPolicy policy = FreeListPolicy::FIND_FIRST;
    char *baseAddr = nullptr;

    AllocatorConfig() = default;

    AllocatorConfig(std::string name,
                    AllocatorType type,
                    std::uint64_t size,
                    std::uint64_t alignment,
                    FreeListPolicy policy,
                    char *base) : name{ std::move(name) },
                                  allocatorType{ type },
                                  size{ size },
                                  alignment{ alignment },
                                  policy{ policy },
                                  baseAddr{ base } {

    }

} AllocatorConfig;

/* === Namespace === */

namespace spider {

    /* === Static methods for allocating memory === */

    AbstractAllocator *&getAllocator(StackID stack);

    void initAllocator(StackID stack, const AllocatorConfig &cfg);

    void finalizeAllocators();

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
        if (ptr) {
            new(ptr) T(std::forward<Args>(args)...);
        }
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
        auto *&allocator = getAllocator(stack);
        if (!allocator) {
            throwSpiderException("Allocating memory with non-initialized allocator.");
        }
        auto buffer = reinterpret_cast<uintptr_t>(allocator->allocate(size + sizeof(std::uint64_t)));

        /* == Return allocated buffer == */
        if (buffer) {
            reinterpret_cast<std::uint64_t *>(buffer)[0] = static_cast<std::uint64_t>(stack);
            buffer = buffer + sizeof(std::uint64_t);
        }
        return reinterpret_cast<T *>(buffer);
    }

    /**
     * @brief Deallocate raw memory pointer
     * @attention This method does not destroy the object, use @refitem Allocator::destroy
     * @param ptr Raw pointer to deallocate
     */
    void deallocate(void *ptr);


    /* === Class definition of the Allocator for stl containers (use of GENERAL stack) === */

    template<class T, StackID stack = StackID::GENERAL>
    class Allocator {
    public:

        /* === Type definitions === */

        typedef size_t size_type;
        typedef ptrdiff_t difference_type;
        typedef T *pointer_type;
        typedef const T *const_pointer;
        typedef T &reference_type;
        typedef const T &const_reference;
        typedef T value_type;

        /* == Rebind SpiderAllocator to type U == */
        template<class U>
        struct rebind {
            typedef Allocator<U> other;
        };

        /* == Return address of value == */
        inline pointer_type address(reference_type value) const {
            return &value;
        }

        /* == Return const address of value == */
        inline const_pointer address(const_reference value) const {
            return &value;
        }

        /* === Constructors / Destructors === */

        explicit Allocator() = default;

        Allocator(const Allocator &) = default;

        template<class U>
        explicit Allocator(const Allocator<U> &) { }

        ~Allocator() = default;

        /* == Maximum size elements that can be allocated == */
        inline size_type max_size() const {
            return std::numeric_limits<size_t>::max() / sizeof(T);
        }

        /**
         * @brief Allocate raw memory buffer on given stack.
         * @tparam T    Type of the pointer to allocate.
         * @param size  Size of the buffer to allocate.
         * @param stack Stack on which the buffer should be allocated, see #SpiderStack
         * @return pointer to allocated buffer, nullptr if size is 0.
         */
        inline pointer_type allocate(size_type size) {
            auto *&allocator = getAllocator(stack);
            if (!allocator) {
                throwSpiderException("Allocating memory with non-initialized allocator.");
            }
            return static_cast<pointer_type >(allocator->allocate(size * sizeof(T)));
        }

        /**
         * @brief Deallocate raw memory pointer
         * @attention This method does not destroy the object, use @refitem Allocator::destroy
         * @param ptr Raw pointer to deallocate
         */
        inline void deallocate(pointer_type ptr, std::size_t) {
            auto *&allocator = getAllocator(stack);
            allocator->deallocate(ptr);
        }

        /**
         * @brief  Construct a previously allocated object from value.
         * @attention This method does not allocate memory, use @refitem Spider::Allocator::allocate first
         * @tparam T     Type of the object to construct
         * @param ptr    Reference pointer of the object to be constructed
         * @param value  Arguments use for by the constructor of the object
         */
        inline void construct(pointer_type ptr, const T &value) {
            new(ptr) T(value);
        }

        /**
         * @brief  Construct a previously allocated object
         * @attention This method does not allocate memory, use @refitem Allocator::allocate first
         * @tparam T     Type of the object to construct
         * @tparam Args  Packed arguments list for construction
         * @param ptr    Reference pointer of the object to be constructed
         * @param args   Arguments use for by the constructor of the object
         */
        template<class ...Args>
        inline void construct(pointer_type ptr, Args &&... args) {
            if (ptr) {
                new(ptr) T(std::forward<Args>(args)...);
            }
        }

        /**
         * @brief Destroy an object
         * @attention This method does not deallocate memory of the pointer, use @refitem Allocator::deallocate
         * @tparam T  Type of the object to destroy
         * @param ptr Reference pointer to the object to destroy
         */
        inline void destroy(pointer_type ptr) {
            ptr->~T();
        }
    };

    template<class T1, class T2>
    bool operator==(const Allocator<T1> &,
                    const Allocator<T2> &) {
        return true;
    }

    template<class T1, class T2>
    bool operator!=(const Allocator<T1> &,
                    const Allocator<T2> &) {
        return false;
    }
}

#endif /* SPIDER_STACKALLOCATOR_H */

