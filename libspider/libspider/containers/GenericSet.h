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
#ifndef SPIDER2_GENERICSET_H
#define SPIDER2_GENERICSET_H

/* === Includes === */

#include <containers/Set.h>

/* === Class definition === */

namespace Spider {

    template<class T>
    struct GenericSetElement : SetElement {

        GenericSetElement() = default;

        explicit GenericSetElement(T &elt) : element_{elt} { }

        explicit GenericSetElement(const T &elt) : element_{elt} { }

        explicit GenericSetElement(T &&elt) : element_{elt} { }

        /* === Operators === */

        inline T &operator*() {
            return element_;
        }

        inline T &operator->() {
            return element_;
        }

        explicit operator T &() {
            return element_;
        }

        explicit operator const T &() const {
            return element_;
        }

        inline GenericSetElement &operator=(T element) {
            using std::swap;
            swap(element_, element);
            return *this;
        }

        inline GenericSetElement &operator=(GenericSetElement temp) {
            using std::swap;
            swap(element_, temp.element_);
            setIx(temp.ix());
            return *this;
        }

        inline bool operator==(const GenericSetElement &other) const {
            return this == &other;
        }

    private:
        T element_;
    };

    template<class T>
    class GenericSet {
    public:

        GenericSet() = default;

        explicit GenericSet(std::uint64_t capacity, StackID stackId = StackID::GENERAL) : set_{capacity, stackId} { }

        GenericSet(const GenericSet &other, StackID stackId = StackID::GENERAL) : set_{other.set_, stackId} { }

        GenericSet(GenericSet &&other) noexcept : set_{std::move(other.set_)} { }

        ~GenericSet() = default;

        /* === Operators === */

        inline T &operator[](std::uint64_t ix) {
            return static_cast<T &>(set_[ix]);
        }

        inline T &operator[](std::uint64_t ix) const {
            return static_cast<const T &>(set_[ix]);
        }

        inline GenericSet &operator=(GenericSet other) {
            swap(*this, other);
            return *this;
        }

        /* === Iterator methods === */

        typedef GenericSetElement<T> *iterator;
        typedef const GenericSetElement<T> *const_iterator;

        inline iterator begin() {
            return set_.begin();
        }

        inline iterator end() {
            return set_.end();
        }

        inline const_iterator begin() const {
            return set_.begin();
        }

        inline const_iterator end() const {
            return set_.end();
        }


        /* === Method(s) === */

        inline friend void swap(GenericSet &first, GenericSet &second) noexcept {
            using std::swap;
            swap(first.set_, second.set_);
        }

        inline T &at(std::uint64_t ix) {
            return static_cast<T &>(set_.at(ix));
        }

        inline T &at(std::uint64_t ix) const {
            return static_cast<const T &>(set_.at(ix));
        }

        inline T &front() const {
            return static_cast<T &>(set_.front());
        }

        inline T &back() const {
            return static_cast<T &>(set_.back());
        }

        inline void add(T &elt) {
            set_.add(elt);
        }

        inline void add(T &&elt) {
            set_.add(GenericSetElement<T>(elt));
        }

        inline void remove(GenericSetElement<T> &elt) {
            set_.remove(elt);
        }

        /* === Getter(s) === */

        inline std::uint64_t capacity() const {
            return set_.capacity();
        }

        inline std::uint64_t occupied() const {
            return set_.occupied();
        }

        inline const GenericSetElement<T> *data() const {
            return set_.data();
        }

        /* === Setter(s) === */

    private:
        Set<GenericSetElement<T>> set_;
    };
}

#endif //SPIDER2_GENERICSET_H
