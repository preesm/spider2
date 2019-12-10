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
#ifndef SPIDER2_DEPENDENCIES_H
#define SPIDER2_DEPENDENCIES_H

/* === Includes === */

#include <cstdint>
#include <algorithm>
#include <common/Math.h>

/* === Methods prototype === */

/* === Inline function(s) === */

namespace spider {
    namespace pisdf {

        /**
         * @brief Compute the upper consumption dependencies of a vertex in a flat graph:
         *                     |   k * c - d   |
         *         lower_dep = | ------------- |
         *                     |_      p      _|
         *
         *         with c = consumption, p = production, k = firing and d = delay.
         *
         *        see https://hal-univ-rennes1.archives-ouvertes.fr/hal-02355636 for more details.
         * @remark edge: sourceRate -> delay -> sinkRate
         * @remark If dependency is inferior to 0, it will be bound to -1. Such value means that the instance depends on
         * the delay initialization.
         * @param consumption  Consumption value on the edge.
         * @param production   Production value on the edge.
         * @param firing       Firing of the vertex.
         * @param delay        Value of the delay.
         * @return value of the lower firing dependency on the producer.
         */
        inline int64_t computeConsLowerDep(int64_t consumption, int64_t production, uint32_t firing, int64_t delay) {
            return std::max(static_cast<int64_t>(-1), spider::math::floorDiv(firing * consumption - delay,
                                                                             production));
        }

        /**
         * @brief Compute the upper consumption dependencies of a vertex in a flat graph:
         *                     |  (k + 1) * c - d - 1  |
         *         upper_dep = | --------------------- |
         *                     |_         p           _|
         *
         *         with c = consumption, p = production, k = firing and d = delay.
         *
         *        see https://hal-univ-rennes1.archives-ouvertes.fr/hal-02355636 for more details.
         * @remark: edge: sourceRate -> delay -> sinkRate
         * @remark If dependency is inferior to 0, it will be bound to -1. Such value means that the instance depends on
         * the delay initialization.
         * @param consumption  Consumption value on the edge.
         * @param production   Production value on the edge.
         * @param firing       Firing of the vertex.
         * @param delay        Value of the delay.
         * @return value of the upper firing dependency on the producer.
         */
        inline int64_t computeConsUpperDep(int64_t consumption, int64_t production, uint32_t firing, int64_t delay) {
            return std::max(static_cast<int64_t>(-1), spider::math::floorDiv((firing + 1) * consumption - delay - 1,
                                                                             production));
        }

        inline int64_t computeProdLowerDep(int64_t sinkRate,
                                           int64_t sourceRate,
                                           int32_t instance,
                                           int64_t delay,
                                           int64_t sinkRepetitionValue) {
            int64_t produced = instance * sourceRate + delay;
            int64_t lowerDep = spider::math::floorDiv(produced, sinkRate);
            return std::min(sinkRepetitionValue, lowerDep);
        }

        inline int64_t computeProdUpperDep(int64_t sinkRate,
                                           int64_t sourceRate,
                                           int32_t instance,
                                           int64_t delay,
                                           int64_t sinkRepetitionValue) {
            int64_t produced = (instance + 1) * sourceRate + delay - 1;
            int64_t upperDep = spider::math::floorDiv(produced, sinkRate);
            return std::min(sinkRepetitionValue, upperDep);
        }

        inline int64_t computeProdLowerDep(int64_t sinkRate,
                                           int64_t sourceRate,
                                           uint32_t instance,
                                           int64_t delay) {
            int64_t produced = instance * sourceRate + delay;
            int64_t lowerDep = produced / sinkRate;
            return lowerDep;
        }

        inline int64_t computeProdUpperDep(int64_t sinkRate,
                                           int64_t sourceRate,
                                           uint32_t instance,
                                           int64_t delay) {
            int64_t produced = (instance + 1) * sourceRate + delay - 1;
            int64_t upperDep = produced / sinkRate;
            return upperDep;
        }
    }
}

#endif //SPIDER2_DEPENDENCIES_H
