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
#ifndef SPIDER2_DEPENDENCYITERATOR_H
#define SPIDER2_DEPENDENCYITERATOR_H

/* === Include(s) === */

#include <graphs-tools/numerical/detail/DependencyInfo.h>
#include <containers/vector.h>

namespace spider {
    namespace pisdf {

        /* === Class definition === */

        struct DependencyIterator {
        public:
            using iterator = DependencyInfo *;

            using const_iterator = const DependencyInfo *;

            explicit DependencyIterator(spider::vector<DependencyInfo> infos) : infos_{ std::move(infos) } { }

            DependencyIterator(const DependencyIterator &) = default;

            DependencyIterator(DependencyIterator &&) noexcept = default;

            DependencyIterator &operator=(const DependencyIterator &) = default;

            DependencyIterator &operator=(DependencyIterator &&) noexcept = default;

            ~DependencyIterator() noexcept = default;

            /* === Methods === */

            inline size_t count() const {
                return infos_.size();
            }

            inline size_t total() const {
                size_t count = 0;
                for (const auto &dep : infos_) {
                    count += (dep.rate_ >= 0) * (dep.firingEnd_ - dep.firingStart_ + 1u);
                }
                return count;
            }

            inline iterator begin() {
                return infos_.data();
            }

            inline const_iterator begin() const {
                return infos_.data();
            }

            inline iterator end() {
                return infos_.data() + infos_.size();
            }

            inline const_iterator end() const {
                return infos_.data() + infos_.size();
            }

        private:
            spider::vector<DependencyInfo> infos_;
        };
    }
}

#endif //SPIDER2_DEPENDENCYITERATOR_H
