/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_VECTOR_H
#define SPIDER2_VECTOR_H

/* === Include(s) === */

#include <vector>
#include <memory/memory.h>

/* === Container definition === */

namespace spider {

    template<class T, class alloc = spider::allocator<T>>
    using vector = std::vector<T, alloc>;

    template<class T>
    inline void out_of_order_erase(vector<T> &v, size_t pos) {
        v[pos] = std::move(v.back());
        v.pop_back();
    }

    template<class T>
    inline void out_of_order_erase(vector<T> &v, typename vector<T>::iterator it) {
        (*it) = std::move(v.back());
        v.pop_back();
    }

    template<class T>
    inline void set_at(vector<T> &v, size_t ix, const T &value) {
#ifndef NDEBUG
        v.at(ix) = value;
#else
        v[ix] = value;
#endif
    }

    template<class T>
    inline void set_at(vector<T> &v, size_t ix, T &&value) {
#ifndef NDEBUG
        v.at(ix) = std::forward<T>(value);
#else
        v[ix] = std::forward<T>(value);
#endif
    }

    template<class T>
    inline T get_at(const vector<T> &v, size_t ix) {
#ifndef NDEBUG
        return v.at(ix);
#else
        return v[ix];
#endif
    }

    template<typename T>
    inline void reserve(spider::vector<T> &vect, size_t size) {
        vect.reserve(std::max(vect.capacity(), vect.size() + size));
    }

    /**
     * @brief Append a vector using move semantics. Inspired by this answer: https://stackoverflow.com/a/37210097 but
     *        modified to allow the caller to choose between copy or move.
     * @tparam T
     * @param dest  destination vector to be appended.
     * @param src   source vector
     * @return begin iterator of dest after append.
     */
    template<typename T>
    typename spider::vector<T>::iterator append(spider::vector<T> &dest, spider::vector<T> src) {
        typename spider::vector<T>::iterator result;
        if (dest.empty()) {
            dest = std::move(src);
            result = std::begin(dest);
        } else {
            result = dest.insert(std::end(dest),
                                 std::make_move_iterator(std::begin(src)),
                                 std::make_move_iterator(std::end(src)));
        }
//        src.clear();
//        src.shrink_to_fit();
        return result;
    }

    namespace factory {

        template<class T>
        inline spider::vector<T> vector(StackID stack = StackID::GENERAL) {
            return spider::vector<T>(spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::vector<T> vector(size_t count, StackID stack = StackID::GENERAL) {
            return spider::vector<T>(count, T(), spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::vector<T> vector(size_t count, const T &value, StackID stack = StackID::GENERAL) {
            return spider::vector<T>(count, value, spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::vector<T> vector(const spider::vector<T> &other, StackID stack = StackID::GENERAL) {
            return spider::vector<T>(other, spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::vector<T> vector(spider::vector<T> &&other, StackID stack = StackID::GENERAL) {
            return spider::vector<T>(std::move(other), spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::vector<T> vector(std::initializer_list<T> init, StackID stack = StackID::GENERAL) {
            return spider::vector<T>(std::move(init), spider::allocator<T>(stack));
        }
    }
}

#endif //SPIDER2_VECTOR_H
