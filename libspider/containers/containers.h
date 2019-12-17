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
#ifndef SPIDER2_CONTAINERS_H
#define SPIDER2_CONTAINERS_H

/* === Includes === */

#include <memory/allocator.h>
#include <vector>
#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <stack>

/* === Macro to make stack_vector with proper stack Allocator in declaration,    === */
/* === avoiding useless copies from one stack to another.                        === */
/* === Basically just a wrapper calling default ctor with Allocator<type>(stack) === */
/* === (I know it is ugly AF but still better than letting it clearly)           === */

#define stack_vector(name, type, stack) spider::vector<type> (name){spider::allocator<type>(stack)}

/* === Namespace === */

namespace spider {

    /* === Sequence containers === */

    template<class T, class alloc = spider::allocator<T>>
    using vector = std::vector<T, alloc>;

    template<class T>
    using deque = std::deque<T, spider::allocator<T>>;

    template<class T>
    using forward_list = std::forward_list<T, spider::allocator<T>>;

    template<class T>
    using list = std::list<T, spider::allocator<T>>;

    /* === Associative containers === */

    template<class Key, class Compare = std::less<Key>>
    using set = std::set<Key, Compare, spider::allocator<Key>>;

    template<class Key, class T, class Compare = std::less<Key>>
    using map = std::map<Key, T, Compare, spider::allocator<std::pair<const Key, T>>>;

    /* === Unordered associative containers === */

    template<class Key, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>>
    using unordered_set = std::unordered_set<Key, Hash, KeyEqual, spider::allocator<Key>>;

    template<class Key, class T>
    using unordered_map = std::unordered_map<Key, T, std::hash<Key>, std::equal_to<Key>, spider::allocator<std::pair<const Key, T>>>;

    /* === Container adaptors === */

    template<class T, class Container = spider::deque<T>>
    using queue = std::queue<T, Container>;

    template<class T, class Container = spider::deque<T>>
    using stack = std::stack<T, Container>;

    /* === Helper functions to make containers using specific stack === */

    namespace containers {

        /* === std::vector === */

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
            return spider::vector<T>(other, spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::vector<T> vector(std::initializer_list<T> init, StackID stack = StackID::GENERAL) {
            return spider::vector<T>(std::move(init), spider::allocator<T>(stack));
        }

        /* === std::deque === */

        template<class T>
        inline spider::deque<T> deque(StackID stack = StackID::GENERAL) {
            return spider::deque<T>(spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::deque<T> deque(size_t count, StackID stack = StackID::GENERAL) {
            return spider::deque<T>(count, T(), spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::deque<T> deque(size_t count, const T &value, StackID stack = StackID::GENERAL) {
            return spider::deque<T>(count, value, spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::deque<T> deque(const spider::deque<T> &other, StackID stack = StackID::GENERAL) {
            return spider::deque<T>(other, spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::deque<T> deque(spider::deque<T> &&other, StackID stack = StackID::GENERAL) {
            return spider::deque<T>(other, spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::deque<T> deque(std::initializer_list<T> init, StackID stack = StackID::GENERAL) {
            return spider::deque<T>(std::move(init), spider::allocator<T>(stack));
        }

        /* === std::forward_list === */

        template<class T>
        inline spider::forward_list<T> forward_list(StackID stack = StackID::GENERAL) {
            return spider::forward_list<T>(spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::forward_list<T> forward_list(size_t count, const T &value, StackID stack = StackID::GENERAL) {
            return spider::forward_list<T>(count, value, spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::forward_list<T>
        forward_list(const spider::forward_list<T> &other, StackID stack = StackID::GENERAL) {
            return spider::forward_list<T>(other, spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::forward_list<T> forward_list(spider::forward_list<T> &&other, StackID stack = StackID::GENERAL) {
            return spider::forward_list<T>(other, spider::allocator<T>(stack));
        }

        /* === std::list === */

        template<class T>
        inline spider::list<T> list(StackID stack = StackID::GENERAL) {
            return spider::list<T>(spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::list<T> list(size_t count, const T &value, StackID stack = StackID::GENERAL) {
            return spider::list<T>(count, value, spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::list<T> list(const spider::list<T> &other, StackID stack = StackID::GENERAL) {
            return spider::list<T>(other, spider::allocator<T>(stack));
        }

        template<class T>
        inline spider::list<T> list(spider::list<T> &&other, StackID stack = StackID::GENERAL) {
            return spider::list<T>(other, spider::allocator<T>(stack));
        }

        /* === std::set === */

        template<class Key, class Compare = std::less<Key>>
        inline spider::set<Key, Compare> set(StackID stack = StackID::GENERAL) {
            return spider::set<Key, Compare>(spider::allocator<Key>(stack));
        }

        template<class Key, class Compare = std::less<Key>>
        inline spider::set<Key, Compare> set(const spider::set<Key, Compare> &other, StackID stack = StackID::GENERAL) {
            return spider::set<Key, Compare>(other, spider::allocator<Key>(stack));
        }

        template<class Key, class Compare = std::less<Key>>
        inline spider::set<Key, Compare> set(spider::map<Key, Compare> &&other, StackID stack = StackID::GENERAL) {
            return spider::set<Key, Compare>(other, spider::allocator<Key>(stack));
        }

        /* === std::map === */

        template<class Key, class T, class Compare = std::less<Key>>
        inline spider::map<Key, T, Compare> map(StackID stack = StackID::GENERAL) {
            return spider::map<Key, T, Compare>(spider::allocator<std::pair<const Key, T>>(stack));
        }

        template<class Key, class T, class Compare = std::less<Key>>
        inline spider::map<Key, T, Compare>
        map(const spider::map<Key, T, Compare> &other, StackID stack = StackID::GENERAL) {
            return spider::map<Key, T, Compare>(other, spider::allocator<std::pair<const Key, T>>(stack));
        }

        template<class Key, class T, class Compare = std::less<Key>>
        inline spider::map<Key, T, Compare>
        map(spider::map<Key, T, Compare> &&other, StackID stack = StackID::GENERAL) {
            return spider::map<Key, T, Compare>(other, spider::allocator<std::pair<const Key, T>>(stack));
        }

        /* === std::unordered_set === */

        template<class Key>
        inline spider::unordered_set<Key> unordered_set(StackID stack = StackID::GENERAL) {
            return spider::unordered_set<Key>(spider::allocator<Key>(stack));
        }

        template<class Key>
        inline spider::unordered_set<Key>
        unordered_set(const spider::unordered_set<Key> &other, StackID stack = StackID::GENERAL) {
            return spider::unordered_set<Key>(other, spider::allocator<Key>(stack));
        }

        template<class Key>
        inline spider::unordered_set<Key>
        unordered_set(spider::unordered_set<Key> &&other, StackID stack = StackID::GENERAL) {
            return spider::unordered_set<Key>(other, spider::allocator<Key>(stack));
        }

        /* === std::unordered_map === */

        template<class Key, class T>
        inline spider::unordered_map<Key, T> unordered_map(StackID stack = StackID::GENERAL) {
            return spider::unordered_map<Key, T>(spider::allocator<std::pair<const Key, T>>(stack));
        }

        template<class Key, class T>
        inline spider::unordered_map<Key, T>
        unordered_map(const spider::unordered_map<Key, T> &other, StackID stack = StackID::GENERAL) {
            return spider::unordered_map<Key, T>(other, spider::allocator<std::pair<const Key, T>>(stack));
        }

        template<class Key, class T>
        inline spider::unordered_map<Key, T>
        unordered_map(spider::unordered_map<Key, T> &&other, StackID stack = StackID::GENERAL) {
            return spider::unordered_map<Key, T>(other, spider::allocator<std::pair<const Key, T>>(stack));
        }
    }
}

#endif //SPIDER2_CONTAINERS_H
