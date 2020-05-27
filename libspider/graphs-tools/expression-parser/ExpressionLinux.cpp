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

#ifdef __linux__

/* === Include(s) === */

#include <graphs-tools/expression-parser/ExpressionLinux.h>
#include <graphs-tools/expression-parser/helper/ExpressionNumeric.h>
#include <graphs/pisdf/Param.h>
#include <dlfcn.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::Expression::Expression(int64_t value) : value_{ static_cast<double>(value) } {

}

spider::Expression::Expression(std::string expression, const vector<std::shared_ptr<pisdf::Param>> &params) {
    /* == Get the postfix expression stack == */
    auto postfixStack = rpn::extractPostfixElements(std::move(expression));
    if (log::enabled<log::EXPR>()) {
        log::verbose<log::EXPR>("infix expression: [%s].\n", rpn::infixString(postfixStack).c_str());
        log::verbose<log::EXPR>("postfix expression: [%s].\n", rpn::postfixString(postfixStack).c_str());
    }

    if (postfixStack.empty()) {
        value_ = 0.;
        return;
    }

    /* == Check if expression is static of not == */
    const auto isDynamic = std::count_if(std::begin(postfixStack), std::end(postfixStack),
                                         [&](const RPNElement &e) {
                                             if (RPNElementSubType::PARAMETER == e.subtype_) {
                                                 const auto param = findParameter(params, e.token_);
                                                 return param->dynamic();
                                             }
                                             return false;
                                         }) > 0;

    if (!isDynamic) {
        auto it = postfixStack.crbegin();
        value_ = evaluateStatic(it, postfixStack.crend(), params);
    } else {
        expr_ = make_shared<expr::CompiledExpression, StackID::EXPRESSION>(postfixStack, params);
    }
}

/* === Private method(s) implementation === */

spider::Expression::param_t
spider::Expression::findParameter(const param_table_t &params, const std::string &name) {
    for (const auto &p : params) {
        if (p->name() == name) {
            return p.get();
        }
    }
    throwSpiderException("Did not find parameter [%s] for expression parsing.", name.c_str());
}

double
spider::Expression::evaluateStatic(spider::vector<RPNElement>::const_reverse_iterator &iterator,
                                   spider::vector<RPNElement>::const_reverse_iterator end,
                                   const spider::Expression::param_table_t &params) {
    if (iterator == end) {
        throwSpiderException("invalid number of argument.");
    }
    const auto &elt = *(iterator++);
    if (elt.type_ == RPNElementType::OPERATOR) {
        const auto opType = rpn::getOperatorTypeFromString(elt.token_);
        const auto &op = rpn::getOperatorFromOperatorType(opType);
        switch (op.argCount) {
            case 1:
                return numeric::apply(opType, evaluateStatic(iterator, end, params));
            case 2: {
                const auto right = evaluateStatic(iterator, end, params);
                const auto left = evaluateStatic(iterator, end, params);
                return numeric::apply(opType, left, right);
            }
            case 3: {
                const auto arg2 = evaluateStatic(iterator, end, params);
                const auto arg1 = evaluateStatic(iterator, end, params);
                const auto arg0 = evaluateStatic(iterator, end, params);
                return numeric::apply(opType, arg0, arg1, arg2);
            }
            default:
                throwSpiderException("Invalid number of argument.");
        }
    } else if (elt.subtype_ == RPNElementSubType::PARAMETER) {
        const auto param = findParameter(params, elt.token_);
        return static_cast<double>(param->value(params));
    }
    return std::strtod(elt.token_.c_str(), nullptr);
}

#endif


