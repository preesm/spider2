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
#ifndef SPIDER2_ARRAY_VIEW_H
#define SPIDER2_ARRAY_VIEW_H

/* === Include(s) === */

#include <cstddef>
#include <stdexcept>
#include <common/Exception.h>

namespace spider {

    /* === Class definition === */

    /**
     * @brief Fixed size array handle for c-style arrays.
     *        Allows for iterating on c-style array and safe access without owning the memory.
     * @tparam T Type of the container content.
     */
    template<typename T>
    class array_view {
    public:
        using value_type = T;
        using size_type = size_t;
        using reference = value_type &;
        using const_reference = const value_type &;
        using pointer = value_type *;
        using const_pointer = const value_type *;
        using iterator = value_type *;
        using const_iterator = const value_type *;

        array_view() = default;

        array_view(T *data, size_type size) : data_{ data }, size_{ size } {
#ifndef NDEBUG
            if (!data && size) {
                throwSpiderException("unsafe array handle created with nullptr data and size != 0.");
            }
#endif
        }

        array_view(const array_view &) = default;

        array_view(array_view &&) noexcept = default;

        ~array_view() = default;

        /* === Member functions === */

        array_view &operator=(const array_view &) = default;

        array_view &operator=(array_view &&) noexcept = default;

        /**
         * @brief Replaces the contents of the container.
         * Replaces the contents with size() copies of value value.
         * @warning All iterators, pointers and references to the elements of the container are invalidated.
         * @param value the value to initialize elements of the container with.
         */
        inline void assign(const_reference value) {
            std::fill(begin(), end(), value);
        }

        /**
         * @brief Replaces the contents of the container.
         * Replaces the contents with the elements from the initializer list ilist.
         * @remark if ilist.size() > size() only the elements of those in the range [0, size()] are copied.
         * @warning All iterators, pointers and references to the elements of the container are invalidated.
         * @param ilist initializer list to copy the values from
         */
        inline void assign(std::initializer_list<value_type> ilist) {
            std::copy(ilist.begin(), std::min(ilist.begin() + size(), ilist.end()), begin());
        }

        /**
         * @brief Exchanges the contents of the container with those of other.
         * Does not invoke any move, copy, or swap operations on individual elements.
         * All iterators and references remain valid.
         * @param first   First container.
         * @param second  Other container to exchange the contents with.
         */
        inline friend void swap(array_view<T> &first, array_view<T> &second) noexcept {
            /* == Do the swapping of the values == */
            using std::swap;
            swap(first.data_, second.data_);
            swap(first.size_, second.size_);
        }

        /* === Element access === */

        /**
         * @brief Returns a reference to the element at specified location pos, with bounds checking.
         * If pos is not within the range of the container, an exception of type std::out_of_range is thrown.
         * @param pos position of the element to return
         * @return Reference to the requested element.
         * @throws std::out_of_range if !(pos < size()).
         */
        inline reference at(size_type pos) {
#ifndef NDEBUG
            if (pos >= size()) { throw std::out_of_range("array out of bound."); }
#endif
            return data_[pos];
        }

        inline const_reference at(size_type pos) const {
#ifndef NDEBUG
            if (pos >= size()) { throw std::out_of_range("array out of bound."); }
#endif
            return data_[pos];
        }

        /**
         * @brief Returns a reference to the element at specified location pos.
         * No bounds checking is performed.
         * @param pos position of the element to return
         * @return Reference to the requested element.
         */
        inline reference operator[](size_type pos) {
            return data_[pos];
        }

        inline const_reference operator[](size_type pos) const {
            return data_[pos];
        }

        /**
         * @brief Returns a reference to the first element in the container.
         * @warning Calling back on an empty container is undefined.
         * @return Reference to the first element.
         */
        inline reference front() {
            return *(begin());
        }

        inline const_reference front() const {
            return *(cbegin());
        }

        /**
         * @brief Returns reference to the last element in the container.
         * @warning Calling back on an empty container is undefined.
         * @return Reference to the last element.
         */
        inline reference back() {
            return *(end() - 1);
        }

        inline const_reference back() const {
            return *(cend() - 1);
        }

        /**
         * @brief Returns pointer to the underlying array serving as element storage.
         * The pointer is such that range [data(); data() + size()) is always a valid range,
         * even if the container is empty (data() is not dereferenceable in that case).
         * @return Pointer to the underlying element storage. For non-empty containers,
         * the returned pointer compares equal to the address of the first element.
         */
        inline pointer data() {
            return data_;
        }

        inline const_pointer data() const {
            return data_;
        }

        /* === Iterators === */

        /**
         * @brief Returns an iterator to the first element of the container.
         * If the container is empty, the returned iterator will be equal to end().
         * @return Iterator to the first element.
         */
        inline iterator begin() {
            if (empty()) {
                return end();
            }
            return data_;
        }

        inline const_iterator begin() const {
            if (empty()) {
                return end();
            }
            return data_;
        }

        inline const_iterator cbegin() const {
            return begin();
        }

        /**
         * @brief Returns an iterator to the element following the last element of the container.
         * This element acts as a placeholder; attempting to access it results in undefined behavior.
         * @return Iterator to the element following the last element.
         */
        inline iterator end() {
            return data_ + size_;
        }

        inline const_iterator end() const {
            return data_ + size_;
        }

        inline const_iterator cend() const {
            return end();
        }

        /* === Capacity === */

        /**
         * @brief Checks if the container has no elements, i.e. whether begin() == end().
         * @return true if the container is empty, false otherwise
         */
        inline bool empty() const {
            return !size_;
        }

        /**
         * @brief  Returns the number of elements in the container, i.e. std::distance(begin(), end()).
         * @return The number of elements in the container.
         */
        inline size_type size() const {
            return size_;
        }

        /* === Non member functions === */

        inline friend bool operator==(const array_view &lhs, const array_view &rhs) {
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

        inline friend bool operator!=(const array_view &lhs, const array_view &rhs) {
            return !(lhs == rhs);
        }

    protected:
        pointer data_ = nullptr;
        size_type size_ = 0;
    };

    template<class T>
    array_view<T> make_view(T *data, size_t size) {
        return array_view<T>(data, size);
    }
}
#endif //SPIDER2_ARRAY_VIEW_H
