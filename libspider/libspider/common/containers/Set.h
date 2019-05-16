/*
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
#ifndef SPIDER2_SET_H
#define SPIDER2_SET_H

/* === Includes === */

#include <type_traits>
#include <common/containers/Array.h>

/* === Structure(s) definition === */

struct SetElement {
    std::uint32_t ix = UINT32_MAX;
};

/* === Class definition === */

template<typename T, typename Enable = void>
class Set;

/* == Condition to ensure the use of proper derived class with this container == */
template<typename T>
using EnableIfPolicy = typename std::enable_if<std::is_base_of<SetElement, typename std::remove_pointer<T>::type>::value>::type;

template<class T>
class Set<T, EnableIfPolicy<T>> {
public:

    Set(StackID stack, std::uint64_t size);

    ~Set() = default;

    /* === Operators === */

    T &operator[](std::uint64_t ix);

    T &operator[](std::uint64_t ix) const;

    /* === Methods === */

    inline void add(T elt);

    inline void remove(T elt);

    inline bool contains(T elt);

    /* === Iterator methods === */

    typedef T *iterator;
    typedef const T *const_iterator;

    iterator begin();

    iterator end();

    const_iterator begin() const;

    const_iterator end() const;

    /* === Getters === */

    /**
     * @brief Return the maximum size of the Set.
     * @return Maximum size of the Set.
     */
    inline std::uint64_t size() const;

    /**
     * @brief Current occupied size of Set, necessary less or equal to @refitem size.
     * @return
     */
    inline std::uint64_t occupied() const;

private:
    Array<T> elements_;
    std::uint64_t occupied_ = 0;
};

/* === Inline methods === */

template<typename T>
Set<T, EnableIfPolicy<T>>::Set(StackID stack, std::uint64_t size) : elements_(stack, size) {
    if (!std::is_pointer<T>()) {
        throwSpiderException("Set should only be used with pointer type.");
    }
}

template<typename T>
T &Set<T, EnableIfPolicy<T>>::operator[](std::uint64_t ix) {
    if (ix >= occupied_) {
        throwSpiderException("Index of non-initialized element. Ix = %"
                                     PRIu32
                                     " -- Size = %"
                                     PRIu32
                                     "", ix, occupied_);
    }
    return elements_[ix];
}

template<typename T>
T &Set<T, EnableIfPolicy<T>>::operator[](std::uint64_t ix) const {
    if (ix >= occupied_) {
        throwSpiderException("Index of non-initialized element. Ix = %"
                                     PRIu32
                                     " -- Size = %"
                                     PRIu32
                                     "", ix, occupied_);
    }
    return elements_[ix];
}

template<typename T>
void Set<T, EnableIfPolicy<T>>::add(T elt) {
    auto *setElement = (SetElement *) (elt);
    if (setElement->ix != UINT32_MAX) {
        return;
    }
    setElement->ix = occupied_;
    elements_[occupied_++] = elt;
}

template<typename T>
void Set<T, EnableIfPolicy<T>>::remove(T elt) {
    if (occupied_) {
        auto *setElement = (SetElement *) (elt);
        elements_[setElement->ix] = elements_[occupied_ - 1];
        ((SetElement *) (elements_[setElement->ix]))->ix = setElement->ix;
        setElement->ix = UINT32_MAX;
        --occupied_;
    }
}

template<typename T>
bool Set<T, EnableIfPolicy<T>>::contains(T elt) {
    for (auto e = begin(); e != end(); ++e) {
        if (*e == elt) {
            return true;
        }
    }
    return false;
}

template<typename T>
typename Set<T, EnableIfPolicy<T>>::iterator Set<T, EnableIfPolicy<T>>::begin() {
    return elements_.begin();
}

template<typename T>
typename Set<T, EnableIfPolicy<T>>::iterator Set<T, EnableIfPolicy<T>>::end() {
    return &elements_[occupied_];
}

template<typename T>
typename Set<T, EnableIfPolicy<T>>::const_iterator Set<T, EnableIfPolicy<T>>::begin() const {
    return elements_.begin();
}

template<typename T>
typename Set<T, EnableIfPolicy<T>>::const_iterator Set<T, EnableIfPolicy<T>>::end() const {
    return &elements_[occupied_];
}

template<typename T>
std::uint64_t Set<T, EnableIfPolicy<T>>::size() const {
    return elements_.size();
}

template<class T>
std::uint64_t Set<T, EnableIfPolicy<T>>::occupied() const {
    return occupied_;
}

#endif //SPIDER2_SET_H
