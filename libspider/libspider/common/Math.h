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
#ifndef SPIDER2_MATH_H
#define SPIDER2_MATH_H

/* === Includes === */

#include <cstdint>

/* === Methods prototype === */

namespace spider {
    namespace math {

        inline uint16_t ceilDiv(uint16_t x, uint16_t y) {
            return static_cast<uint16_t>(x / y + (x % y != 0));
        }

        inline uint32_t ceilDiv(uint32_t x, uint32_t y) {
            return x / y + (x % y != 0);
        }

        inline uint64_t ceilDiv(uint64_t x, uint64_t y) {
            return x / y + (x % y != 0);
        }

        inline int32_t ceilDiv(int32_t x, int32_t y) {
            auto neg = (x < 0 && y > 0) || (x > 0 && y < 0);
            auto a = static_cast<uint32_t>((x < 0 ? (-x) : x));
            auto b = static_cast<uint32_t>((y < 0 ? (-y) : y));
            return neg ? -static_cast<int32_t>(a / b) : static_cast<int32_t>(ceilDiv(a, b));
        }

        inline int64_t ceilDiv(int64_t x, int64_t y) {
            auto neg = (x < 0 && y > 0) || (x > 0 && y < 0);
            auto a = static_cast<uint64_t>((x < 0 ? (-x) : x));
            auto b = static_cast<uint64_t>((y < 0 ? (-y) : y));
            return neg ? -static_cast<int64_t>(a / b) : static_cast<int64_t>(ceilDiv(a, b));
        }

        inline int64_t floorDiv(int64_t x, int64_t y) {
            auto neg = (x < 0 && y > 0) || (x > 0 && y < 0);
            auto a = static_cast<uint64_t>((x < 0 ? (-x) : x));
            auto b = static_cast<uint64_t>((y < 0 ? (-y) : y));
            return neg ? -static_cast<int64_t>(ceilDiv(a, b)) : static_cast<int64_t>(a / b);
        }

        inline int32_t floorDiv(int32_t x, int32_t y) {
            auto neg = (x < 0 && y > 0) || (x > 0 && y < 0);
            auto a = static_cast<uint32_t>((x < 0 ? (-x) : x));
            auto b = static_cast<uint32_t>((y < 0 ? (-y) : y));
            return neg ? -static_cast<int32_t>(ceilDiv(a, b)) : static_cast<int32_t>(a / b);
        }

        inline int16_t abs(int16_t x) {
            return static_cast<int16_t>(x < 0 ? -x : x);
        }

        inline int32_t abs(int32_t x) {
            return x < 0 ? -x : x;
        }

        inline int64_t abs(int64_t x) {
            return x < 0 ? -x : x;
        }

        inline int64_t gcd(int64_t x, int64_t y) {
            int64_t t;
            while (y != 0) {
                t = y;
                y = x % y;
                x = t;
            }
            return x;
        }

        inline uint64_t gcd(uint64_t x, uint64_t y) {
            uint64_t t;
            while (y != 0) {
                t = y;
                y = x % y;
                x = t;
            }
            return x;
        }

        inline int64_t lcm(int64_t a, int64_t b) {
            return spider::math::abs(a * b) / spider::math::gcd(a, b);
        }

        inline uint64_t lcm(uint64_t a, uint64_t b) {
            return (a * b) / spider::math::gcd(a, b);
        }

        inline uint64_t saturateAdd(uint64_t a, uint64_t b) {
            return b > (UINT64_MAX - a) ? UINT64_MAX : a + b;
        }

        inline uint32_t saturateAdd(uint32_t a, uint32_t b) {
            return b > (UINT32_MAX - a) ? UINT32_MAX : a + b;
        }
    }
}

#endif //SPIDER2_MATH_H
