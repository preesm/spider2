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
#ifndef SPIDER2_ARRAY_H
#define SPIDER2_ARRAY_H

/* === Includes === */

#include <cstdint>
#include <vector>
#include <memory/memory.h>
#include <containers/array_view.h>

/* === Class definition === */

namespace spider {

    /**
     * @brief Fixed size array whose size is determined at runtime (as opposed to std::array) -> basically a wrapper around an std::vector.
     * @tparam T Type of the container content.
     */
    template<typename T>
    class array : public array_view<T> {
    public:
        using value_type = T;
        using size_type = size_t;
        using reference = value_type &;
        using const_reference = const value_type &;
        using pointer = value_type *;
        using const_pointer = const value_type *;
        using iterator = value_type *;
        using const_iterator = const value_type *;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        using difference_type = ptrdiff_t;

        /* === Constructors, assignment and destructor === */

        /**
         * @brief Create an array of size size on stack stack.
         * @param stack  Stack on which the array should be allocated.
         * @param size   Size of the array.
         */
        explicit array(size_type size, StackID stack = StackID::GENERAL) : array_view<T>(allocate<T>(stack, size),
                                                                                           size) { }

        /**
         * @brief Create an array of size size on stack stack with all values set to value;
         * @param stack  Stack on which the array should be allocated.
         * @param size   Size of the array.
         * @param value  Value to set to all the elements of the array.
         */
        array(size_t size, const_reference value, StackID stack = StackID::GENERAL) : array(size, stack) {
            array_view<T>::assign(value);
        }

        array(std::initializer_list<value_type> il, StackID stack = StackID::GENERAL) : array(il.size(), stack) {
            std::copy(il.begin(), il.end(), array_view<T>::begin());
        }

        array() noexcept = default;

        array(const array &other) : array(other.size_) {
            std::copy(other.begin(), other.end(), array_view<T>::begin());
        };

        array(array &&other) noexcept: array() {
            swap(*this, other);
        }

        array(T *data, size_type size) : array_view<T>(data, size) { }

        ~array() {
            deallocate(array_view<T>::data_);
        }

        /* === Member functions === */

        array &operator=(const array &other) {
            deallocate(array_view<T>::data_);
            array_view<T>::data_ = allocate<T>(other.size_);
            array_view<T>::size_ = other.size_;
            std::copy(other.begin(), other.end(), array_view<T>::begin());
            return *this;
        }

        array &operator=(array &&other) noexcept {
            swap(*this, other);
            return *this;
        }

        /* === Modifiers === */

        /**
         * @brief Exchanges the contents of the container with those of other.
         * Does not invoke any move, copy, or swap operations on individual elements.
         * All iterators and references remain valid.
         * @param first   First container.
         * @param second  Other container to exchange the contents with.
         */
        inline friend void swap(array<T> &first, array<T> &second) noexcept {
            /* == Do the swapping of the values == */
            using std::swap;
            swap(static_cast<array_view<T> &>(first), static_cast<array_view<T> &>(second));
        }

        /* === Non member functions === */

        template<typename U>
        friend bool operator==(const array<U> &lhs, const array<U> &rhs);

        template<typename U>
        friend bool operator!=(const array<U> &lhs, const array<U> &rhs);
    };

    template<typename U>
    inline bool operator==(const array<U> &lhs, const array<U> &rhs) {
        if (lhs.size() != rhs.size()) {
            return false;
        }
        auto itlhs = lhs.begin();
        auto itrhs = rhs.begin();
        while (itlhs != lhs.end()) {
            if ((*(itlhs++)) != (*(itrhs++))) {
                return false;
            }
        }
        return true;
    }

    template<typename U>
    inline bool operator!=(const array<U> &lhs, const array<U> &rhs) {
        return !(lhs == rhs);
    }

}
#endif //SPIDER2_ARRAY_H
