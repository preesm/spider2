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
#ifndef SPIDER2_EXPRESSION_H
#define SPIDER2_EXPRESSION_H

/* === Includes === */

#include <cstdint>
#include <string>
#include <common/containers/Array.h>
#include "RPNConverter.h"

/* === Type declarations === */

using Param = std::int64_t;

/* === Forward declaration(s) === */

class PiSDFGraph;

/* === Class definition === */

class Expression {
public:

    Expression(Spider::string expression, PiSDFGraph *graph);

    ~Expression() = default;

    /* === Methods === */

    /**
     * @brief Evaluate the expression and return the value.
     * @return Evaluated value of the expression.
     */
    inline Param evaluate() const;

    /* === Getters === */

    /**
     * @brief Get the last evaluated value (faster than evaluated on static expressions)
     * @return last evaluated value (default value, i.e no evaluation done, is 0)
     */
    inline Param value() const;

    /**
     * @brief Get the infix expression string
     * @return Clean infix expression string
     */
    inline Spider::string toString() const;

private:

    Spider::string infixExpression_{""};
    RPNConverter postFixExpression_;
    Param value_ = 0;
};

/* === Inline methods === */

Param Expression::value() const {
    return value_;
}

Spider::string Expression::toString() const {
    return infixExpression_;
}

Param Expression::evaluate() const {
    if (postFixExpression_.isStatic()) {
        return value_;
    }
    return static_cast<Param>(postFixExpression_.evaluate());
}

#endif //SPIDER2_EXPRESSION_H
