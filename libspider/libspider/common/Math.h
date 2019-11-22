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

        inline std::uint16_t ceilDiv(std::uint16_t x, std::uint16_t y) {
            return static_cast<std::uint16_t>(x / y + (x % y != 0));
        }

        inline std::uint32_t ceilDiv(std::uint32_t x, std::uint32_t y) {
            return x / y + (x % y != 0);
        }

        inline std::uint64_t ceilDiv(std::uint64_t x, std::uint64_t y) {
            return x / y + (x % y != 0);
        }

        inline std::int32_t ceilDiv(std::int32_t x, std::int32_t y) {
            auto neg = (x < 0 && y > 0) || (x > 0 && y < 0);
            auto a = static_cast<uint32_t>((x < 0 ? (-x) : x));
            auto b = static_cast<uint32_t>((y < 0 ? (-y) : y));
            return static_cast<int32_t>((neg ? -(a / b) : ceilDiv(a, b)));
        }

        inline std::int64_t ceilDiv(std::int64_t x, std::int64_t y) {
            auto neg = (x < 0 && y > 0) || (x > 0 && y < 0);
            auto a = static_cast<uint64_t>((x < 0 ? (-x) : x));
            auto b = static_cast<uint64_t>((y < 0 ? (-y) : y));
            return static_cast<int64_t>((neg ? -(a / b) : ceilDiv(a, b)));
        }

        inline std::int64_t floorDiv(std::int64_t x, std::int64_t y) {
            auto neg = (x < 0 && y > 0) || (x > 0 && y < 0);
            auto a = static_cast<uint64_t>((x < 0 ? (-x) : x));
            auto b = static_cast<uint64_t>((y < 0 ? (-y) : y));
            return static_cast<int64_t>((neg ? -ceilDiv(a, b) : a / b));
        }

        inline std::int32_t floorDiv(std::int32_t x, std::int32_t y) {
            auto neg = (x < 0 && y > 0) || (x > 0 && y < 0);
            auto a = static_cast<uint32_t>((x < 0 ? (-x) : x));
            auto b = static_cast<uint32_t>((y < 0 ? (-y) : y));
            return static_cast<int32_t>((neg ? -ceilDiv(a, b) : a / b));
        }

        inline std::int16_t abs(std::int16_t x) {
            return static_cast<std::int16_t>(x < 0 ? -x : x);
        }

        inline std::int32_t abs(std::int32_t x) {
            return x < 0 ? -x : x;
        }

        inline std::int64_t abs(std::int64_t x) {
            return x < 0 ? -x : x;
        }

        inline std::int64_t gcd(std::int64_t x, std::int64_t y) {
            std::int64_t t;
            while (y != 0) {
                t = y;
                y = x % y;
                x = t;
            }
            return x;
        }

        inline std::uint64_t gcd(std::uint64_t x, std::uint64_t y) {
            std::uint64_t t;
            while (y != 0) {
                t = y;
                y = x % y;
                x = t;
            }
            return x;
        }

        inline std::int64_t lcm(std::int64_t a, std::int64_t b) {
            return spider::math::abs(a * b) / spider::math::gcd(a, b);
        }

        inline std::uint64_t lcm(std::uint64_t a, std::uint64_t b) {
            return (a * b) / spider::math::gcd(a, b);
        }

        inline std::uint64_t saturateAdd(std::uint64_t a, std::uint64_t b) {
            return b > (UINT64_MAX - a) ? UINT64_MAX : a + b;
        }

        inline std::uint32_t saturateAdd(std::uint32_t a, std::uint32_t b) {
            return b > (UINT32_MAX - a) ? UINT32_MAX : a + b;
        }
    }
}

#endif //SPIDER2_MATH_H
