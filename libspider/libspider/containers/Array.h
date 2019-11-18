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
#ifndef SPIDER2_ARRAY_H
#define SPIDER2_ARRAY_H

/* === Includes === */

#include <cstdint>
#include <vector>
#include <memory/Allocator.h>

/* === Class definition === */

namespace spider {

    template<typename T>
    class Array {
    public:

        /**
         * @brief Create an array of size size on stack stack.
         * @param stack  Stack on which the array should be allocated.
         * @param size   Size of the array.
         */
        inline Array(std::uint64_t size, StackID stack = StackID::GENERAL);

        /**
         * @brief Create an array of size size on stack stack with all values set to value;
         * @param stack  Stack on which the array should be allocated.
         * @param size   Size of the array.
         * @param value  Value to set to all the elements of the array.
         */
        inline Array(std::uint64_t size, T value, StackID stack = StackID::GENERAL);

        inline Array() noexcept;

        inline Array(const Array &other, StackID stack = StackID::GENERAL);

        inline Array(Array &&other) noexcept;

        inline ~Array();

        /* === Operators === */

        inline T &operator[](std::uint64_t ix);

        inline T &operator[](std::uint64_t ix) const;

        /**
         * @brief use the "making new friends idiom" from
         * https://en.wikibooks.org/wiki/More_C%2B%2B_Idioms/Making_New_Friends
         * @param first   First element to swap.
         * @param second  Second element to swap.
         */
        inline friend void swap(Array<T> &first, Array<T> &second) noexcept {
            using std::swap;

            /* == Do the swapping of the values == */
            swap(first.array_, second.array_);
            swap(first.size_, second.size_);
        }

        /**
         * @brief Assignment operator. Use move constructor if used with an rvalue reference, copy constructor else.
         * Use the copy and swap idiom.
         * @param temp
         * @return reference to this.
         */
        inline Array &operator=(Array temp);

        /* === Iterator methods === */

        typedef T *iterator;
        typedef const T *const_iterator;

        iterator begin();

        iterator end();

        const_iterator begin() const;

        const_iterator end() const;

        /* === Setters === */

        /**
         * @brief Set the values of the array from a vector.
         * @param values   Vector of values to set.
         * @param offset   Offset from which values are added (0 by default).
         * @throw @refitem SpiderException if sizes do not match.
         */
        inline void setValues(std::vector<T> &values, std::uint32_t offset = 0);

        /* === Getters === */

        /**
         * @brief  Return the size of the Array.
         * @return size of the array
         */
        inline std::uint64_t size() const;

        /**
         * @brief Return the raw array pointer.
         * @return pointer to the array
         */
        inline const T *data() const;

        /**
         * @brief Return element of the array at position ix with bound check.
         * @param ix Index of the element.
         * @return reference to the element
         * @throws Spider::Exception if out of bound.
         */
        inline T &at(std::uint64_t ix);

        /**
         * @brief Return element of the array at position ix with bound check.
         * @param ix Index of the element.
         * @return const reference to the element
         * @throws Spider::Exception if out of bound.
         */
        inline T &at(std::uint64_t ix) const;

    private:
        std::uint64_t size_;
        T *array_;
    };

    /* === Inline methods === */

    template<typename T>
    Array<T>::Array() noexcept : size_{ 0 }, array_{ nullptr } {

    }

    template<typename T>
    Array<T>::Array(const Array &other, StackID stack) : size_{ other.size_ } {
        array_ = spider::allocate<T>(stack, size_ + 1);
        if (array_) {
            std::copy(other.array_, other.array_ + other.size_, array_);
        }
    }

    template<typename T>
    Array<T>::Array(Array &&other) noexcept : Array<T>() {
        swap(*this, other);
    }

    template<typename T>
    Array<T>::Array(std::uint64_t size, StackID stack) : size_{ size } {
        array_ = spider::allocate<T>(stack, size_ + 1);
        if (!array_) {
            throwSpiderException("Failed to allocate array.");
        }
    }

    template<typename T>
    Array<T>::Array(std::uint64_t size, T value, StackID stack) : Array<T>(size, stack) {
        std::fill(this->begin(), this->end(), value);
    }

    template<typename T>
    Array<T>::~Array() {
        spider::deallocate(array_);
    }

    template<typename T>
    T &Array<T>::operator[](std::uint64_t ix) {
        return array_[ix];
    }

    template<typename T>
    T &Array<T>::operator[](std::uint64_t ix) const {
        return array_[ix];
    }

    template<typename T>
    Array<T> &Array<T>::operator=(Array temp) {
        swap(*this, temp);
        return *this;
    }

    template<typename T>
    T &Array<T>::at(std::uint64_t ix) {
        if (ix >= size_) {
            throwSpiderException("Index out of bound. Ix = %"
                                         PRIu32
                                         " -- Size = %"
                                         PRIu32
                                         "", ix, size_ + 1);
        }
        return array_[ix];
    }

    template<typename T>
    T &Array<T>::at(std::uint64_t ix) const {
        if (ix >= size_) {
            throwSpiderException("Index out of bound. Ix = %"
                                         PRIu32
                                         " -- Size = %"
                                         PRIu32
                                         "", ix, size_ + 1);
        }
        return array_[ix];
    }

    template<typename T>
    typename Array<T>::iterator Array<T>::begin() {
        return array_;
    }

    template<typename T>
    typename Array<T>::iterator Array<T>::end() {
        return &array_[size_];
    }

    template<typename T>
    typename Array<T>::const_iterator Array<T>::begin() const {
        return array_;
    }

    template<typename T>
    typename Array<T>::const_iterator Array<T>::end() const {
        return &array_[size_];
    }

    template<typename T>
    void Array<T>::setValues(std::vector<T> &values, uint32_t offset) {
        if (values.size() + offset > size_) {
            throwSpiderException("Size of the vector %"
                                         PRIu64
                                         " do not match size of the Array %"
                                         PRIu64
                                         "", values.size(), size_);
        }
        std::uint32_t i = offset;
        for (auto &v: values) {
            array_[i++] = v;
        }
    }

    template<typename T>
    std::uint64_t Array<T>::size() const {
        return size_;
    }

    template<typename T>
    const T *Array<T>::data() const {
        return array_;
    }
}
#endif //SPIDER2_ARRAY_H
