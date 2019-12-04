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
        inline int64_t computeConsLowerDep(int64_t sinkRate,
                                           int64_t sourceRate,
                                           uint32_t instance,
                                           int64_t delay) {
            int64_t consumed = instance * sinkRate - delay;
            int64_t lowerDep = spider::math::floorDiv(consumed, sourceRate);
            constexpr int64_t initBound = -1;
            return std::max(initBound, lowerDep);
        }

        inline int64_t computeConsUpperDep(int64_t sinkRate,
                                           int64_t sourceRate,
                                           uint32_t instance,
                                           int64_t delay) {
            int64_t consumed = (instance + 1) * sinkRate - delay - 1;
            int64_t lowerDep = spider::math::floorDiv(consumed, sourceRate);
            constexpr int64_t initBound = -1;
            return std::max(initBound, lowerDep);
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
