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
    struct GenericSetElement : public Spider::SetElement {
    private:
        T &elt;
    public:
        explicit GenericSetElement(const T &elt) : elt{elt} { };

        GenericSetElement(GenericSetElement const &other) : elt{other.elt} { };

        GenericSetElement(GenericSetElement &&other) noexcept : elt{other.elt} { };

        GenericSetElement(std::initializer_list<T> l) noexcept : elt{*(l.begin())} { };

        GenericSetElement &operator=(GenericSetElement const &other) {
            elt = other.elt;
            setIx(other.getIx());
            return *this;
        }

        inline T &operator*() const {
            return elt;
        }

        inline T &operator->() const {
            return elt;
        }

        inline T &element() {
            return elt;
        };
    };

    template<class T>
    class GenericSet : public Spider::Set<GenericSetElement<T>> {
    public:
        GenericSet(StackID stack, std::uint64_t size);

        ~GenericSet() = default;

        /* === Method(s) === */

        inline T &at(std::uint64_t ix);

        inline T &at(std::uint64_t ix) const;
    };

    template<class T>
    GenericSet<T>::GenericSet(StackID stack, std::uint64_t size): Set<GenericSetElement<T>>(stack, size) {

    }

    template<class T>
    T &GenericSet<T>::at(std::uint64_t ix) {
        return *(Set<GenericSetElement<T>>::operator[](ix));
    }

    template<class T>
    T &GenericSet<T>::at(std::uint64_t ix) const {
        return *(Set<GenericSetElement<T>>::operator[](ix));
    }
}

#endif //SPIDER2_GENERICSET_H
