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
#ifndef SPIDER2_STLALLOCATOR_H
#define SPIDER2_STLALLOCATOR_H

/* === Include(s) === */

#include <cstdint>
#include <limits>
#include <memory/Allocator.h>

namespace spider {

    /* === Class definition === */

    /**
     * @brief Allocator class used for STL containers (allows to track memory usage of STL containers)
     * @tparam T      Type to allocate.
     * @tparam stack  Stack used by the STLAllocator class (default is StackID::GENERAL)
     */
    template<class T, StackID stack = StackID::GENERAL>
    class STLAllocator {
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
            typedef STLAllocator<U> other;
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

        explicit STLAllocator() = default;

        STLAllocator(const STLAllocator &) = default;

        template<class U>
        explicit STLAllocator(const STLAllocator<U> &) { }

        ~STLAllocator() = default;

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
            if (!allocator<stack>()) {
                throwSpiderException("Allocating memory with non-initialized allocator.");
            }
            return static_cast<pointer_type >(allocator<stack>()->allocate(size * sizeof(T)));
        }

        /**
         * @brief Deallocate raw memory pointer
         * @attention This method does not destroy the object, use @refitem Allocator::destroy
         * @param ptr Raw pointer to deallocate
         */
        inline void deallocate(pointer_type ptr, std::size_t) {
            allocator<stack>()->deallocate(ptr);
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
            new(ptr) T(std::forward<Args>(args)...);
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

        template<class T1, class T2>
        friend bool operator==(const STLAllocator<T1> &, const STLAllocator<T2> &);

        template<class T1, class T2>
        friend bool operator!=(const STLAllocator<T1> &, const STLAllocator<T2> &);

    private:

    };

    template<class T1, class T2>
    inline bool operator==(const STLAllocator<T1> &, const STLAllocator<T2> &) {
        return true;
    }

    template<class T1, class T2>
    inline bool operator!=(const STLAllocator<T1> &, const STLAllocator<T2> &) {
        return false;
    }
}

#endif //SPIDER2_STLALLOCATOR_H
