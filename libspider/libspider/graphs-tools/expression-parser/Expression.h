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
#ifndef SPIDER2_EXPRESSION_H
#define SPIDER2_EXPRESSION_H

/* === Includes === */

#include <cstdint>
#include <string>
#include <graphs-tools/expression-parser/RPNConverter.h>
#include <graphs/pisdf/common/Types.h>

/* === Structure definition(s) === */

struct ExpressionElt {
    RPNElement elt_;
    union {
        double value_ = 0;
        RPNOperatorType opType_;
    } arg;

    ExpressionElt() = default;

    ExpressionElt(const ExpressionElt &) = default;

    ExpressionElt(ExpressionElt &&) noexcept = default;

    ExpressionElt &operator=(ExpressionElt elt) {
        using std::swap;
        swap(this->elt_, elt.elt_);
        swap(this->arg, elt.arg);
        return *this;
    }

    explicit ExpressionElt(RPNElement elt) : elt_{ std::move(elt) } { }
};

/* === Class definition === */

class Expression {
public:

    explicit Expression(std::string expression, const spider::vector<PiSDFParam *> &params = { });

    explicit Expression(std::int64_t value);

    Expression() = default;

    Expression(const Expression &other) = default;

    inline Expression(Expression &&other) noexcept = default;

    ~Expression() = default;

    /* === Operator(s) === */

    inline friend void swap(Expression &first, Expression &second) noexcept {
        /* == Enable ADL == */
        using std::swap;

        /* == Swap members of both objects == */
        swap(first.expressionStack_, second.expressionStack_);
        swap(first.value_, second.value_);
        swap(first.static_, second.static_);
    }

    inline Expression &operator=(Expression temp) {
        swap(*this, temp);
        return *this;
    }

    /* === Methods === */

    /**
     * @brief Evaluate the expression and return the value and cast result in int64_.
     * @return Evaluated value of the expression.
     */
    inline std::int64_t evaluate(const spider::vector<PiSDFParam *> &params = { }) const;

    /**
     * @brief Evaluate the expression and return the value.
     * @return Evaluated value of the expression.
     */
    inline double evaluateDBL(const spider::vector<PiSDFParam *> &params = { }) const;

    /**
     * @brief Get the expression string.
     * @remark The obtained string does not necessarily to the input string due to optimizations.
     * @return expression string.
     */
    std::string string() const;

    /* === Getters === */

    /**
     * @brief Get the last evaluated value (faster than evaluated on static expressions)
     * @return last evaluated value (default value, i.e no evaluation done, is 0)
     */
    inline std::int64_t value() const;

    /**
     * @brief Get the static property of the expression.
     * @return true if the expression is static, false else.
     */
    inline bool dynamic() const;

private:
    spider::vector<ExpressionElt> expressionStack_;
    double value_ = 0;
    bool static_ = true;

    /* === Private method(s) === */

    /**
     * @brief Build and reduce the expression tree parser.
     * @param expressionStack Stack of the postfix expression elements.
     */
    spider::vector<ExpressionElt>
    buildExpressionStack(spider::vector<RPNElement> &postfixStack, const spider::vector<PiSDFParam *> &params);

    /**
     * @brief Evaluate the expression (if dynamic)
     * @warning There is no check for the presence of the parameters in the params vector.
     * @param params  Vector of parameters needed for the eval.
     * @return evaluated value
     */
    double evaluateStack(const spider::vector<PiSDFParam *> &params) const;
};

/* === Inline methods === */

std::int64_t Expression::value() const {
    return static_cast<std::int64_t>(value_);
}

std::int64_t Expression::evaluate(const spider::vector<PiSDFParam *> &params) const {
    return static_ ? static_cast<std::int64_t>(value_) : static_cast<std::int64_t>(evaluateStack(params));
}

double Expression::evaluateDBL(const spider::vector<PiSDFParam *> &params) const {
    return static_ ? value_ : evaluateStack(params);
}

bool Expression::dynamic() const {
    return !static_;
}

#endif //SPIDER2_EXPRESSION_H
