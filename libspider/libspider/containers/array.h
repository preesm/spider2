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
#include <memory/allocator.h>
#include "containers.h"

/* === Class definition === */

namespace spider {

    /**
     * @brief Fixed size array whose size is determined at runtime (as opposed to std::array) -> basically a wrapper around an std::vector.
     * @tparam T Type of the container content.
     */
    template<typename T>
    class array {
    public:

        /**
         * @brief Create an array of size size on stack stack.
         * @param stack  Stack on which the array should be allocated.
         * @param size   Size of the array.
         */
        explicit array(std::size_t size, StackID stack = StackID::GENERAL) : data_(size,
                                                                                   spider::Allocator<T>(stack)) { }

        /**
         * @brief Create an array of size size on stack stack with all values set to value;
         * @param stack  Stack on which the array should be allocated.
         * @param size   Size of the array.
         * @param value  Value to set to all the elements of the array.
         */
        array(std::size_t size, T value, StackID stack = StackID::GENERAL) : data_(size, value,
                                                                                   spider::Allocator<T>(stack)) { }

        array() noexcept = default;

        array(const array &) = default;

        array(array &&) noexcept = default;

        ~array() = default;

        array &operator=(const array &) = default;

        array &operator=(array &&) noexcept = default;

        /* === Operators === */

        inline T &operator[](std::size_t ix);

        inline const T &operator[](std::size_t ix) const;

        /* === Iterator methods === */

        typedef typename spider::vector<T>::iterator iterator;
        typedef typename spider::vector<T>::const_iterator const_iterator;

        iterator begin();

        iterator end();

        const_iterator begin() const;

        const_iterator end() const;


        /* === Getters === */

        /**
         * @brief  Return the size of the Array.
         * @return size of the array
         */
        inline std::size_t size() const;

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
        inline T &at(std::size_t ix);

        /**
         * @brief Return element of the array at position ix with bound check.
         * @param ix Index of the element.
         * @return const reference to the element
         * @throws Spider::Exception if out of bound.
         */
        inline const T &at(std::size_t ix) const;

    private:
        spider::vector<T> data_;
    };

    /* === Inline methods === */

    template<typename T>
    T &array<T>::operator[](std::size_t ix) {
        return data_[ix];
    }

    template<typename T>
    const T &array<T>::operator[](std::size_t ix) const {
        return data_[ix];
    }

    template<typename T>
    T &array<T>::at(std::size_t ix) {
        return data_.at(ix);
    }

    template<typename T>
    const T &array<T>::at(std::size_t ix) const {
        return data_.at(ix);
    }

    template<typename T>
    typename array<T>::iterator array<T>::begin() {
        return data_.begin();
    }

    template<typename T>
    typename array<T>::iterator array<T>::end() {
        return data_.end();
    }

    template<typename T>
    typename array<T>::const_iterator array<T>::begin() const {
        return data_.begin();
    }

    template<typename T>
    typename array<T>::const_iterator array<T>::end() const {
        return data_.end();
    }

    template<typename T>
    std::size_t array<T>::size() const {
        return data_.size();
    }

    template<typename T>
    const T *array<T>::data() const {
        return data_.data();
    }
}
#endif //SPIDER2_ARRAY_H
