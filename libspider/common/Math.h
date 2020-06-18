/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
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
#ifndef SPIDER2_MATH_H
#define SPIDER2_MATH_H

/* === Includes === */

#include <cstdint>
#include <limits>
#include <type_traits>

/* === Methods prototype === */

namespace spider {
    namespace math {

        template<class T>
        inline T abs(T x) {
            return static_cast<T>((x < 0) ? (-x) : x);
        }

        template<class T>
        inline T factorial(T x) {
            T res = 1;
            T i = 1;
            while (i <= x) { res *= (i++); }
            return res;
        }

        template<class T>
        inline T ceilDiv(T x, T y) {
            static_assert(std::is_integral<T>::value, "ceilDiv should be used with integer value");
            T d = x / y;
            T r = x % y; /* = Should be optimize into single division = */
            return r ? d + !((x < 0) ^ (y < 0)) : d;
        }

        template<class T>
        inline T floorDiv(T x, T y) {
            static_assert(std::is_integral<T>::value, "floorDiv should be used with integer only");
            T d = x / y;
            T r = x % y; /* = Should be optimize into single division = */
            return r ? (d - ((x < 0) ^ (y < 0))) : d;
        }

        template<class T>
        inline T gcd(T x, T y) {
            static_assert(std::is_integral<T>::value, "GCD expect integral values.");
            auto a = static_cast<typename std::make_unsigned<T>::type>(x);
            auto b = static_cast<typename std::make_unsigned<T>::type>(y);
            typename std::make_unsigned<T>::type t;
            while (b != 0) {
                t = b;
                b = a % b;
                a = t;
            }
            return static_cast<T>(a);
        }

        template<class T>
        inline T lcm(T a, T b) {
            return abs(a * b) / gcd(a, b);
        }

        template<class T>
        inline T saturateAdd(T a, T b) {
            return b > (std::numeric_limits<T>::max() - a) ? std::numeric_limits<T>::max() : a + b;
        }
    }
}

#endif //SPIDER2_MATH_H
