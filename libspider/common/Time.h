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
#ifndef SPIDER2_TIME_H
#define SPIDER2_TIME_H

/* === Include(s) === */

#include <chrono>

namespace spider {

    namespace time {

        /* === Type(s) definition === */

        using time_point = std::chrono::steady_clock::time_point;

        /* === Function(s) prototype === */

        inline time_point now() noexcept {
            return std::chrono::steady_clock::now();
        }

        namespace duration {

            inline long long hours(time_point first, time_point second) noexcept {
                return std::chrono::duration_cast<std::chrono::hours>(second - first).count();
            }

            inline long long minutes(time_point first, time_point second) noexcept {
                return std::chrono::duration_cast<std::chrono::minutes>(second - first).count();
            }

            inline long long seconds(time_point first, time_point second) noexcept {
                return std::chrono::duration_cast<std::chrono::seconds>(second - first).count();
            }

            inline long long milliseconds(time_point first, time_point second) noexcept {
                return std::chrono::duration_cast<std::chrono::milliseconds>(second - first).count();
            }

            inline long long microseconds(time_point first, time_point second) noexcept {
                return std::chrono::duration_cast<std::chrono::microseconds>(second - first).count();
            }

            inline long long nanoseconds(time_point first, time_point second) noexcept {
                return std::chrono::duration_cast<std::chrono::nanoseconds>(second - first).count();
            }

        }

    }
}

#endif //SPIDER2_TIME_H
