/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2018)
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
#ifndef SPIDER_SET_H
#define SPIDER_SET_H

#include <common/SpiderException.h>
#include <common/memory/StackAllocator.h>

class SetElement {
public:
    SetElement() = default;

    ~SetElement() = default;

    inline void setSetIx(std::int32_t ix) {
        setIx_ = ix;
    }

    inline std::int32_t getSetIx() const {
        return setIx_;
    }

private:
    std::int32_t setIx_ = -1;
};

template<typename TYPE>
class Set {
public:

    Set(std::int32_t sizeMax, SpiderStack stackId) : sizeMax_{sizeMax}, stackId_{stackId} {
        elements_ = Allocator::allocate<TYPE>(sizeMax, stackId);
    }

    ~Set() {
        Allocator::deallocate(elements_);
    }

    inline void add(TYPE value);

    inline void del(TYPE value);

    inline bool contains(TYPE value);

    inline std::int32_t size() const;

    inline TYPE const *getElements() const;

    inline TYPE operator[](int ix);

    inline TYPE operator[](int ix) const;

    inline TYPE *begin();

    inline TYPE *end();

private:
    SetElement *elements_;
    std::int32_t size_ = 0;
    std::int32_t sizeMax_;
    SpiderStack stackId_;
};

template<typename TYPE>
inline void Set<TYPE>::add(TYPE value) {
    if (size_ >= sizeMax_) {
        throwSpiderException("Can not add element to set. Requested: %d -- Max: %d", size_ + 1, sizeMax_);
    }
    ((SetElement *) value)->setSetIx(size_);
    elements_[size_++] = value;
}

template<typename TYPE>
inline void Set<TYPE>::del(TYPE value) {
    auto ix = ((SetElement *) value)->getSetIx();
    elements_[ix] = elements_[--size_];
    ((SetElement *) elements_[ix])->setSetIx(ix);
}

template<typename TYPE>
inline bool Set<TYPE>::contains(TYPE value) {
    bool found = false;
    for (auto it = begin(); it != end() && !found; ++it) {
        found |= (*it == value);
    }
    return found;
}

template<typename TYPE>
inline std::int32_t Set<TYPE>::size() const {
    return size_;
}

template<typename TYPE>
inline TYPE const *Set<TYPE>::getElements() const {
    return elements_;
}

template<typename TYPE>
inline TYPE Set<TYPE>::operator[](int ix) {
    if (size_ < 0 || ix >= size_) {
        throwSpiderException("operator[] got bad index: %d -- Set size: %d", ix, size_);
    } else {
        return elements_[ix];
    }
}

template<typename TYPE>
inline TYPE Set<TYPE>::operator[](int ix) const {
    if (size_ < 0 || ix >= size_) {
        throwSpiderException("operator[] got bad index: %d -- Set size: %d", ix, size_);
    } else {
        return elements_[ix];
    }
}

template<typename TYPE>
inline TYPE *Set<TYPE>::begin() {
    return &elements_[0];
}

template<typename TYPE>
inline TYPE *Set<TYPE>::end() {
    return &elements_[size_ - 1];
}

#endif // SPIDER_SET_H
