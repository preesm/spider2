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
#ifndef SPIDER2_STLCONTAINERS_H
#define SPIDER2_STLCONTAINERS_H

/* === Includes === */

#include <set>
#include <map>
#include <deque>
#include <queue>
#include <vector>
#include <forward_list>
#include <unordered_set>
#include <unordered_map>
#include <memory/Allocator.h>

/* === Namespace === */

namespace Spider {

    template<class T>
    using vector = std::vector<T, Spider::Allocator<T>>;

    template<class T>
    using deque = std::deque<T, Spider::Allocator<T>>;

    template<class T>
    using queue = std::queue<T, Spider::deque<T>>;

    template<class Key>
    using unordered_set = std::unordered_set<Key, std::hash<Key>, std::equal_to<Key>, Spider::Allocator<Key>>;

    template<class Key>
    using set = std::set<Key, std::less<Key>, Spider::Allocator<Key>>;

    template<class Key, class T>
    using unordered_map = std::unordered_map<Key,
            T,
            std::hash<Key>,
            std::equal_to<Key>,
            Spider::Allocator<std::pair<const Key, T>>>;

    template<class Key, class T>
    using map = std::map<Key, T, std::less<Key>, Spider::Allocator<std::pair<const Key, T>>>;

    template<class T>
    using forward_list = std::forward_list<T, Spider::Allocator<T>>;
}

#endif //SPIDER2_STLCONTAINERS_H
