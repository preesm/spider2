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
#ifndef SPIDER2_ALLOCATOR_H
#define SPIDER2_ALLOCATOR_H

/* === Include(s) === */

#include <cstdint>
#include <limits>
#include <memory/alloc.h>

namespace spider {

    /* === Class definition === */

    /**
     * @brief Allocator class used for STL containers (allows to track memory usage of STL containers)
     * @tparam T      Type to allocate.
     * @tparam stack  Stack used by the STLAllocator class (default is StackID::GENERAL)
     */
    template<class T>
    class allocator {
    public:

        /* === Type definitions === */

        using value_type = T;

//        using pointer = value_type *;
//        using const_pointer = typename std::pointer_traits<pointer>::template rebind<value_type const>;
//        using void_pointer = typename std::pointer_traits<pointer>::template rebind<void>;
//        using const_void_pointer = typename std::pointer_traits<pointer>::template rebind<const void>;
//
//        using difference_type = typename std::pointer_traits<pointer>::difference_type;
//        using size_t = typename std::make_unsigned<difference_type>::type;

        /* == Rebind SpiderAllocator to type U == */
//        template<class U>
//        struct rebind {
//            typedef allocator<U> other;
//        };

        /* === Constructors / Destructors === */

        allocator() noexcept : allocator_{ getStackAllocator<StackID::GENERAL>() } { };

        template<class U>
        allocator(const allocator<U> &other) noexcept : allocator_{ other.allocator_impl() } { }

        explicit allocator(StackID stack) : allocator_{ getStackAllocator(stack) } {
            if (!allocator_) {
                throwSpiderException("trying to use non-initialized allocator.");
            }
        }

        allocator(const allocator &other) noexcept : allocator_{ other.allocator_ } { };

        ~allocator() = default;

        inline AbstractAllocator *allocator_impl() const {
            return allocator_;
        }

        inline value_type *allocate(size_t n, size_t extra = 0) {
            return static_cast<value_type *>(allocator_->allocate(n * sizeof(value_type) + extra));
        }

        inline void deallocate(value_type *p, size_t) {
            allocator_->deallocate(p);
        }

//        inline void construct(value_type *p, const T &value) { new(p) T(value); }

//        template<class ...Args>
//        inline void construct(pointer_type ptr, Args &&... args) { ::new(ptr) T(std::forward<Args>(args)...); }

//        inline void destroy(pointer_type ptr) { ptr->~T(); }

//        inline size_type max_size() const { return std::numeric_limits<size_t>::max() / sizeof(T); }

//        allocator select_on_container_copy_construction() const { return *this; }

//        using propagate_on_container_copy_assignment = std::false_type;
//        using propagate_on_container_move_assignment = std::false_type;
//        using propagate_on_container_swap = std::false_type;
//        using is_always_equal = std::is_empty<allocator>;

        template<class T1, class T2>
        friend bool operator==(const allocator<T1> &, const allocator<T2> &);

        template<class T1, class T2>
        friend bool operator!=(const allocator<T1> &, const allocator<T2> &);

    private:
        AbstractAllocator *allocator_ = nullptr;
    };

    template<class T1, class T2>
    inline bool operator==(const allocator<T1> &a, const allocator<T2> &b) {
        return a.allocator_ == b.allocator_;
    }

    template<class T1, class T2>
    inline bool operator!=(const allocator<T1> &a, const allocator<T2> &b) {
        return !(a == b);
    }
}

#endif //SPIDER2_ALLOCATOR_H
