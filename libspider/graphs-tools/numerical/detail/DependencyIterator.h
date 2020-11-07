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
#ifndef SPIDER2_DEPENDENCYITERATOR_H
#define SPIDER2_DEPENDENCYITERATOR_H

/* === Include(s) === */

#include <graphs-tools/numerical/detail/DependencyInfo.h>
#include <containers/vector.h>
#include <memory/unique_ptr.h>

namespace spider {
    namespace pisdf {

        /* === Class definition === */

        struct DependencyIterator {
        public:
            using iterator = DependencyInfo *;

            using const_iterator = const DependencyInfo *;

            explicit DependencyIterator(spider::vector<DependencyInfo> infos) {
                dependencies_ = spider::make_unique(allocate<DependencyInfo, StackID::TRANSFO>(infos.size()));
                for (size_t i = 0; i < infos.size(); ++i) {
                    dependencies_[i] = infos[i];
                }
                count_ = static_cast<u32>(infos.size());
            }

            DependencyIterator() = default;

            DependencyIterator(const DependencyIterator &) = delete;

            DependencyIterator(DependencyIterator &&) = default;

            DependencyIterator &operator=(const DependencyIterator &) = delete;

            DependencyIterator &operator=(DependencyIterator &&) = default;

            ~DependencyIterator() noexcept = default;

            /* === Methods === */

            inline size_t count() const {
                return count_;
            }

            inline size_t total() const {
                if (!count_ && dependencies_[0].rate_ < 0) {
                    return SIZE_MAX;
                }
                size_t res = 0;
                for (u32 i = 0; i < count_; ++i) {
                    const auto &dep = dependencies_[i];
                    res += (dep.rate_ >= 0) * (dep.firingEnd_ - dep.firingStart_ + 1u);
                }
                return res;
            }

            inline DependencyInfo &operator[](size_t ix) {
                return dependencies_[ix];
            }

            inline const DependencyInfo &operator[](size_t ix) const {
                return dependencies_[ix];
            }

            inline iterator begin() {
                return dependencies_.get();
            }

            inline const_iterator begin() const {
                return dependencies_.get();
            }

            inline iterator end() {
                return dependencies_.get() + count_;
            }

            inline const_iterator end() const {
                return dependencies_.get() + count_;
            }

        private:
            spider::unique_ptr<DependencyInfo> dependencies_;
            u32 count_;
        };

        /* == Type def for full dependencies of a vertex == */

        using VertexDependencies = spider::vector<DependencyIterator>;
    }
}

#endif //SPIDER2_DEPENDENCYITERATOR_H
