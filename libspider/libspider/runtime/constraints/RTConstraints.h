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
#ifndef SPIDER2_RTCONSTRAINTS_H
#define SPIDER2_RTCONSTRAINTS_H

/* === Include(s) === */

#include <containers/containers.h>
#include <graphs-tools/expression-parser/Expression.h>
#include <archi/Platform.h>
#include <spider-api/archi.h>
#include <archi/Cluster.h>
#include <archi/PE.h>

namespace spider {

    /* === Class definition === */

    class RTConstraints {
    public:

        RTConstraints() {
            auto *platform = spider::platform();
            const auto &clusterCount = platform->clusterCount();
            const auto &peCount = platform->PECount();
            peMappableVector_.resize(peCount, true);
            clusterMappableVector_.resize(clusterCount, true);
            timingVector_.resize(peCount, Expression(100));
        }

        ~RTConstraints() = default;

        /* === Getter(s) === */

        /**
         * @brief Evaluate if vertex associated to this RTConstraints is mappable at PE level.
         * @param ix Spider pe ix to evaluate.
         * @return true if mappable, false else
         * @throws std::out_of_range
         */
        inline bool isPEMappable(size_t ix) const {
            return peMappableVector_.at(ix);
        }

        /**
         * @brief Evaluate if vertex associated to this RTConstraints is mappable at cluster level.
         * @param ix Spider cluster ix to evaluate.
         * @return true if mappable, false else
         * @throws std::out_of_range
         */
        inline bool isClusterMappable(size_t ix) const {
            return clusterMappableVector_.at(ix);
        }

        /**
         * @brief Evaluate the timing of vertex associated to this RTConstraints on given PE.
         * @param ix      Spider pe ix to evaluate.
         * @param params  Extra parameters in case timing is parameterized.
         * @return timing on given PE (100 by default).
         * @throws std::out_of_range
         */
        inline int64_t timingOnPE(size_t ix, const spider::vector<pisdf::Param *> &params = { }) const {
            return timingVector_.at(ix).evaluate(params);
        }

        /* === Setter(s) === */

        /**
         * @brief Set mappable constraint on given PE.
         * @remark This method update value of clusterMappableVector_ of the corresponding cluster.
         * @param pe        PE to evaluate.
         * @param mappable  Value to set (default is true).
         * @throws std::out_of_range
         */
        inline void setMappableConstraintOnPE(const PE *pe, bool mappable = true) {
            auto *cluster = pe->cluster();
            peMappableVector_.at(pe->spiderPEIx()) = mappable;
            auto clusterMappableValue = false;
            std::for_each(cluster->peArray().begin(), cluster->peArray().end(),
                          [&clusterMappableValue, this](const PE *pe) {
                              clusterMappableValue |= peMappableVector_.at(pe->spiderPEIx());
                          });
            clusterMappableVector_.at(cluster->ix()) = clusterMappableValue;
        }

        /**
         * @brief Set mappable constraint on all PE.
         * @param mappable  Value to set (default is true).
         */
        inline void setMappableConstraintOnAllPE(bool mappable = true) {
            std::fill(peMappableVector_.begin(), peMappableVector_.end(), mappable);
            std::fill(clusterMappableVector_.begin(), clusterMappableVector_.end(), mappable);
        }

        /**
         * @brief Set timing on a given PE by value.
         * @param ix     Spider PE ix to evaluate.
         * @param value  Value to set (default is 100).
         * @throws std::out_of_range
         */
        inline void setTimingOnPE(size_t ix, int64_t value = 100) {
            timingVector_.at(ix) = Expression(value);
        }

        /**
         * @brief Set timing on a given PE by Expression.
         * @param ix          Spider PE ix to evaluate.
         * @param expression  Expression to set.
         */
        inline void setTimingOnPE(size_t ix, Expression &&expression) {
            timingVector_.at(ix) = std::move(expression);
        }

        /**
         * @brief Set timing on all PE by value.
         * @param value  Value to set (default is 100).
         */
        inline void setTimingOnAllPE(int64_t value = 100) {
            std::fill(timingVector_.begin(), timingVector_.end(), Expression(value));
        }

        /**
         * @brief Set timing on all PE by Expression.
         * @param expression Expression to set.
         */
        inline void setTimingOnAllPE(const Expression &expression) {
            std::fill(timingVector_.begin(), timingVector_.end(), expression);
        }

    private:
        stack_vector(peMappableVector_, bool, StackID::CONSTRAINTS);
        stack_vector(clusterMappableVector_, bool, StackID::CONSTRAINTS);
        stack_vector(timingVector_, Expression, StackID::CONSTRAINTS);
    };
}
#endif //SPIDER2_RTCONSTRAINTS_H
