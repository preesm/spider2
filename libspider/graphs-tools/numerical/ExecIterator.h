/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_EXECITERATOR_H
#define SPIDER2_EXECITERATOR_H

/* === Include(s) === */

#include <common/Types.h>
#include <memory/unique_ptr.h>
#include <containers/vector.h>
#include <graphs-tools/numerical/detail/ExecDependencyInfo.h>

namespace spider {

    namespace srless {
        class FiringHandler;
    }

    namespace pisdf {
        class Edge;

        class Vertex;

        /* === Class definition === */

        class ExecIterator final {
        public:
            using type_t = ExecDependencyInfo;
            using pointer_t = ExecDependencyInfo *;
            using const_pointer_t = const ExecDependencyInfo *;

            ExecIterator(const Edge *edge, int64_t lowerCons, int64_t upperCons, const srless::FiringHandler *handler);

            ~ExecIterator() = default;

            ExecIterator(const ExecIterator &) = delete;

            ExecIterator &operator=(const ExecIterator &) = delete;

            ExecIterator &operator=(ExecIterator &&) = default;

            ExecIterator(ExecIterator &&) = default;

            /* === Method(s) === */

            pointer_t begin();

            pointer_t operator++(int);

            pointer_t end();

            /* === Getter(s) === */

            /* === Setter(s) === */

        private:
            spider::vector<spider::unique_ptr<ExecIterator>> deps_;
            spider::unique_ptr<ExecDependencyInfo> info_;
            u32 current_{ };

            /* === Private method(s) === */

            ExecDependencyInfo createExecDependency(const Edge *edge,
                                                    int64_t lowerCons,
                                                    int64_t upperCons,
                                                    int64_t srcRate,
                                                    int64_t delayValue,
                                                    const srless::FiringHandler *handler) const;
        };

        ExecIterator make_iterator(const Vertex *vertex, u32 firing, size_t edgeIx, const srless::FiringHandler *handler);
    }
}

#endif //SPIDER2_EXECITERATOR_H
