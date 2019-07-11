/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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
#ifndef SPIDER2_PARSERFUNCTIONS_H
#define SPIDER2_PARSERFUNCTIONS_H

/* === Includes === */

#include <cstdint>
#include <cmath>

/* === Methods prototype === */

namespace Spider {
    inline double dummyEval(double &, double &) {
        return 0.;
    }

    inline double add(double &a, double &b) {
        return a + b;
    }

    inline double sub(double &a, double &b) {
        return a - b;
    }

    inline double mul(double &a, double &b) {
        return a * b;
    }

    inline double div(double &a, double &b) {
        return a / b;
    }

    inline double mod(double &a, double &b) {
        return static_cast<std::int64_t >(a) % static_cast<std::int64_t >(b);
    }

    inline double pow(double &a, double &b) {
        return std::pow(a, b);
    }

    inline double cos(double &a, double &) {
        return std::cos(a);
    }

    inline double sin(double &a, double &) {
        return std::sin(a);
    }

    inline double tan(double &a, double &) {
        return std::tan(a);
    }

    inline double exp(double &a, double &) {
        return std::exp(a);
    }

    inline double log(double &a, double &) {
        return std::log(a);
    }

    inline double log2(double &a, double &) {
        return std::log2(a);
    }

    inline double ceil(double &a, double &) {
        return std::ceil(a);
    }

    inline double floor(double &a, double &) {
        return std::floor(a);
    }

    inline double sqrt(double &a, double &) {
        return std::sqrt(a);
    }

    inline double max(double &a, double &b) {
        return std::max(a, b);
    }

    inline double min(double &a, double &b) {
        return std::min(a, b);
    }
}

#endif //SPIDER2_PARSERFUNCTIONS_H
