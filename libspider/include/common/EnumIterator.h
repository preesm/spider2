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
#ifndef SPIDER2_ENUMITERATOR_H
#define SPIDER2_ENUMITERATOR_H

/* === Include(s) === */

#include <cstdint>

namespace spider {

    /* === Class definition === */

    /**
     * @brief This iterator is used to iterate over enum class and is strongly inspired by
     *  https://stackoverflow.com/questions/8498300/allow-for-range-based-for-with-enum-classes answer of deft_code.
     */
    template<class T>
    class EnumIterator {
    public:
        class Iterator {
        public:
            explicit Iterator(int_fast32_t value) : value_{ value } { }

            Iterator(const Iterator &) = default;

            Iterator(Iterator &&) noexcept = default;

            Iterator &operator=(const Iterator &) = default;

            Iterator &operator=(Iterator &&) noexcept = default;

            ~Iterator() = default;

            /* === Method(s) === */

            inline T operator*() const {
                return static_cast<T>(value_);
            }

            inline T operator->() const {
                return static_cast<T>(value_);
            }

            inline Iterator &operator++() {
                ++value_;
                return *this;
            }

            inline Iterator operator++(int) {
                auto tmp = *this;
                ++(*this);
                return tmp;
            }

            void operator--() {
                --value_;
            }

            bool operator!=(const Iterator &other) {
                return value_ != other.value_;
            }

            bool operator<(const Iterator &other) {
                return value_ < other.value_;
            }

            bool operator>(const Iterator &other) {
                return value_ < other.value_;
            }

            bool operator<=(const Iterator &other) {
                return !(*this > other);
            }

            bool operator>=(const Iterator &other) {
                return !(*this < other);
            }

            Iterator &operator-=(int a) {
                value_ -= a;
                return *this;
            }

            Iterator &operator+=(int a) {
                value_ += a;
                return *this;
            }

            Iterator operator-(int a) {
                return Iterator(value_ - a);
            }

            Iterator operator+(int a) {
                return Iterator(value_ + a);
            }

        private:
            int_fast32_t value_;
        };

        inline typename EnumIterator<T>::Iterator begin() {
            return typename EnumIterator<T>::Iterator(static_cast<int_fast32_t>(T::First));
        }

        inline typename EnumIterator<T>::Iterator end() {
            return typename EnumIterator<T>::Iterator(static_cast<int_fast32_t>(T::Last) + 1);
        }
    };
}
#endif //SPIDER2_ENUMITERATOR_H
