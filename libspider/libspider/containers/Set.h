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
#ifndef SPIDER2_SET_H
#define SPIDER2_SET_H

/* === Includes === */

#include <type_traits>
#include <cinttypes>
#include <common/Exception.h>
#include <memory/Allocator.h>
#include <containers/SetElement.h>

/* === Class definition === */

namespace Spider {

    /* === Condition to ensure the use of proper derived class with this container === */

    template<typename T>
    using EnableIfPolicy =
    typename std::enable_if<std::is_base_of<SetElement, typename std::remove_pointer<T>::type>::value>::type;

    template<typename T, typename Enable = void>
    class Set;

    /* === Class specialization === */

    template<class T>
    class Set<T, EnableIfPolicy<T>> {
    public:

        Set() = default;

        explicit Set(std::uint64_t capacity, StackID stackId = StackID::GENERAL) : size_{ capacity + 1 },
                                                                                   capacity_{ capacity },
                                                                                   occupied_{ 0 } {
            data_ = Spider::allocate<T>(stackId, size_);
        }

        Set(const Set &other, StackID stackId = StackID::GENERAL) : size_{ other.size_ },
                                                                    capacity_{ other.capacity_ },
                                                                    occupied_{ 0 } {
            data_ = Spider::allocate<T>(stackId, other.size_);
            std::copy(other.data_, other.data_ + other.size_, data_);
        }

        Set(Set &&other) noexcept : Set() {
            swap(*this, other);
        }

        ~Set() {
            Spider::deallocate(data_);
        }

        /* === Operators === */

        inline T &operator[](std::uint64_t ix) {
            return data_[ix];
        }

        inline T &operator[](std::uint64_t ix) const {
            return data_[ix];
        }

        inline Set &operator=(Set other);

        /* === Method(s) === */

        inline friend void swap(Set &first, Set &second) noexcept {
            using std::swap;

            swap(first.data_, second.data_);
            swap(first.size_, second.size_);
            swap(first.capacity_, second.capacity_);
            swap(first.occupied_, second.occupied_);
        }

        inline T &at(std::uint64_t ix);

        inline T &at(std::uint64_t ix) const;

        inline T &front() const;

        inline T &back() const;

        inline bool contains(const T &elt) const;

        inline void add(T &elt);

        inline void add(T &&elt);

        inline void remove(T &elt);

        /**
         * @brief Run-time (probably optimized by compiler though) version of std::remove_pointer.
         * @param obj reference to the object
         * @return reference to the object (itself)
         */
        inline T &remove_pointer(T &obj) {
            return obj;
        }

        /**
         * @brief Run-time (probably optimized by compiler though) version of std::remove_pointer.
         * @param obj Pointer to the object
         * @return reference to the object.
         */
        inline T &remove_pointer(T *obj) {
            return *obj;
        }

        /* === Iterator methods === */

        typedef T *iterator;
        typedef const T *const_iterator;

        inline iterator begin();

        inline iterator end();

        inline const_iterator begin() const;

        inline const_iterator end() const;

        /* === Getter(s) === */

        inline std::uint64_t capacity() const {
            return capacity_;
        }

        inline std::uint64_t occupied() const {
            return occupied_;
        }

        inline const T *data() const {
            return data_;
        }

        /* === Setter(s) === */

    private:

        T *data_ = nullptr;
        std::uint64_t size_ = 0;
        std::uint64_t capacity_ = 0;
        std::uint64_t occupied_ = 0;

        /* === Private method(s) === */
    };

    /* === Inline method(s) === */

    template<class T>
    Set<T, EnableIfPolicy<T>> &Set<T, EnableIfPolicy<T>>::operator=(Set other) {
        swap(*this, other);
        return *this;
    }

    template<class T>
    T &Set<T, EnableIfPolicy<T>>::at(std::uint64_t ix) {
        if (ix >= occupied_) {
            throwSpiderException("Index of non-initialized element. Ix = %"
                                         PRIu32
                                         " -- Size = %"
                                         PRIu32
                                         "", ix, occupied_);
        }
        return data_[ix];
    }

    template<class T>
    T &Set<T, EnableIfPolicy<T>>::at(std::uint64_t ix) const {
        if (ix >= occupied_) {
            throwSpiderException("Index of non-initialized element. Ix = %"
                                         PRIu32
                                         " -- Size = %"
                                         PRIu32
                                         "", ix, occupied_);
        }
        return data_[ix];
    }

    template<class T>
    T &Set<T, EnableIfPolicy<T>>::front() const {
        return data_[0];
    }

    template<class T>
    T &Set<T, EnableIfPolicy<T>>::back() const {
        return data_[occupied_ - 1];
    }

    template<class T>
    bool Set<T, EnableIfPolicy<T>>::contains(const T &elt) const {
        for (auto &e : *this) {
            if (e == elt) {
                return true;
            }
        }
        return false;
    }

    template<class T>
    void Set<T, EnableIfPolicy<T>>::add(T &elt) {
        auto &setElement = static_cast<SetElement &>(remove_pointer(elt));
        if (setElement.ix() == UINT32_MAX) {
            setElement.setIx(occupied_);
            data_[occupied_++] = static_cast<T &>(setElement);
        }
    }

    template<class T>
    void Set<T, EnableIfPolicy<T>>::add(T &&elt) {
        auto &setElement = static_cast<SetElement &>(remove_pointer(elt));
        if (setElement.ix() == UINT32_MAX) {
            setElement.setIx(occupied_);
            data_[occupied_++] = static_cast<T &>(setElement);
        }
    }

    template<class T>
    void Set<T, EnableIfPolicy<T>>::remove(T &elt) {
        if (occupied_) {
            auto &setElement = static_cast<SetElement &>(remove_pointer(elt));
            auto ix = setElement.ix();
            if (ix != UINT32_MAX) {
                auto &element = this->at(ix);
                if (element == elt) {
                    /* == Swap the removed element with the last one == */
                    if (occupied_ > 1) {
                        data_[ix] = data_[occupied_ - 1];
                        static_cast<SetElement &>(remove_pointer(data_[ix])).setIx(ix);
                    }
                    --occupied_;
                }
            }
        }
    }

    template<class T>
    typename Set<T, EnableIfPolicy<T>>::iterator Set<T, EnableIfPolicy<T>>::begin() {
        return data_;
    }

    template<class T>
    typename Set<T, EnableIfPolicy<T>>::iterator Set<T, EnableIfPolicy<T>>::end() {
        return occupied_ == capacity_ ? &data_[capacity_] : &data_[occupied_];
    }

    template<class T>
    typename Set<T, EnableIfPolicy<T>>::const_iterator Set<T, EnableIfPolicy<T>>::begin() const {
        return data_;
    }

    template<class T>
    typename Set<T, EnableIfPolicy<T>>::const_iterator Set<T, EnableIfPolicy<T>>::end() const {
        return occupied_ == capacity_ ? &data_[capacity_] : &data_[occupied_];
    }

}

#endif //SPIDER2_SET_H
