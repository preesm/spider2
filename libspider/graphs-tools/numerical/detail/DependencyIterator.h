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

#include <extra/variant.h>
#include <graphs-tools/numerical/detail/UniqueDependency.h>
#include <graphs-tools/numerical/detail/MultipleDependency.h>

namespace spider {
    namespace pisdf {

        /* === Class definition === */

        struct VoidDependency { };

        struct DependencyIterator {
        public:
            using iterator = DependencyInfo *;

            using const_iterator = const DependencyInfo *;

            explicit DependencyIterator(UniqueDependency it) : it_{ it } { };

            explicit DependencyIterator(VoidDependency it) : it_{ it } { };

            explicit DependencyIterator(MultipleDependency it) : it_{ std::move(it) } { };

            DependencyIterator(const DependencyIterator &) = default;

            DependencyIterator(DependencyIterator &&) = default;

            DependencyIterator &operator=(const DependencyIterator &) = default;

            DependencyIterator &operator=(DependencyIterator &&) = default;

            ~DependencyIterator() = default;

            /* === Methods === */

            inline long count() const {
                return std::distance(begin(), end());
            }

            inline iterator begin() {
                if (mpark::holds_alternative<UniqueDependency>(it_)) {
                    return &(mpark::get<UniqueDependency>(it_).info_);
                } else if (mpark::holds_alternative<VoidDependency>(it_)) {
                    return nullptr;
                }
                return mpark::get<MultipleDependency>(it_).infos_.data();
            }

            inline const_iterator begin() const {
                if (mpark::holds_alternative<UniqueDependency>(it_)) {
                    return &(mpark::get<UniqueDependency>(it_).info_);
                } else if (mpark::holds_alternative<VoidDependency>(it_)) {
                    return nullptr;
                }
                return mpark::get<MultipleDependency>(it_).infos_.data();
            }

            inline iterator end() {
                if (mpark::holds_alternative<UniqueDependency>(it_)) {
                    return &(mpark::get<UniqueDependency>(it_).info_) + 1;
                } else if (mpark::holds_alternative<VoidDependency>(it_)) {
                    return nullptr;
                } else {
                    return mpark::get<MultipleDependency>(it_).infos_.end().base();
                }
            }

            inline const_iterator end() const {
                if (mpark::holds_alternative<UniqueDependency>(it_)) {
                    return &(mpark::get<UniqueDependency>(it_).info_) + 1;
                } else if (mpark::holds_alternative<VoidDependency>(it_)) {
                    return nullptr;
                } else {
                    return mpark::get<MultipleDependency>(it_).infos_.end().base();
                }
            }

        private:
            mpark::variant<UniqueDependency, MultipleDependency, VoidDependency> it_;
        };
    }
}

#endif //SPIDER2_DEPENDENCYITERATOR_H
