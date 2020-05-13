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
/* === Includes === */

#include <containers/vector.h>
#include <graphs-tools/expression-parser/Expression.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Graph.h>

/* === Static method(s) === */

static spider::pisdf::Param *
findParam(const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params, const std::string &name) {
    for (const auto &p : params) {
        if (p->name() == name) {
            return p.get();
        }
    }
    throwSpiderException("Did not find parameter [%s] for expression parsing.", name.c_str());
}

/* === Methods implementation === */

spider::Expression::Expression(std::string expression, const spider::vector<std::shared_ptr<pisdf::Param>> &params) {
    /* == Get the postfix expression stack == */
    auto postfixStack = rpn::extractPostfixElements(std::move(expression));
    if (log::enabled<log::EXPR>()) {
        log::verbose<log::EXPR>("infix expression: [%s].\n", rpn::infixString(postfixStack).c_str());
        log::verbose<log::EXPR>("postfix expression: [%s].\n", rpn::postfixString(postfixStack).c_str());
    }

    /* == Reorder the postfix stack elements to increase the number of static evaluation done on construction == */
//    rpn::reorderPostfixStack(postfixStack);

    /* == Build the expression stack == */
    bool staticExpression = true;
    auto stack = compile(postfixStack, params, staticExpression);

    if (staticExpression) {
        value_ = stack.empty() ? 0. : stack.back().arg.value_;
    } else {
        /* == Doing dynamic alloc of vector member if stack is not static == */
        expressionStack_ = make<spider::vector<ExpressionElt>, StackID::EXPRESSION>(std::move(stack));
    }
}

spider::Expression::Expression(int64_t value) : value_{ static_cast<double>(value) } {
}

spider::Expression::~Expression() {
    destroy(expressionStack_);
}

spider::Expression &spider::Expression::operator+=(const spider::Expression &rhs) {
    if (!rhs.dynamic()) {
        value_ = value_ + rhs.value_;
    } else if (!dynamic()) {
        expressionStack_ = make<spider::vector<ExpressionElt>, StackID::EXPRESSION>(*(rhs.expressionStack_));
        expressionStack_->emplace_back(
                RPNElement{ RPNElementType::OPERAND, RPNElementSubType::VALUE, std::to_string(value_) });
        expressionStack_->emplace_back(RPNElement{ RPNElementType::OPERATOR, RPNElementSubType::OPERATOR, "+" });
    } else {
        for (auto &elt : *(rhs.expressionStack_)) {
            expressionStack_->emplace_back(elt);
        }
        expressionStack_->emplace_back(RPNElement{ RPNElementType::OPERATOR, RPNElementSubType::OPERATOR, "+" });
    }
    return *this;
}

/* === Private method(s) === */

spider::vector<spider::ExpressionElt>
spider::Expression::compile(spider::vector<RPNElement> &postfixStack,
                            const spider::vector<std::shared_ptr<pisdf::Param>> &params,
                            bool &staticExpression) {
    auto stack = factory::vector<ExpressionElt>(StackID::EXPRESSION);
    stack.reserve(postfixStack.size());
    auto operatorStack = factory::vector<RPNElement *>(StackID::EXPRESSION);
    operatorStack.reserve(6);
    auto evalStack = factory::vector<double>(StackID::EXPRESSION);
    evalStack.reserve(6); /* = In practice, the evalStack will most likely not exceed 3 values = */
    for (auto &elt : postfixStack) {
        if (elt.type_ == RPNElementType::OPERAND) {
            auto exprElt = ExpressionElt{ std::move(elt) };
            if (exprElt.elt_.subtype_ == RPNElementSubType::PARAMETER) {
                const auto param = findParam(params, exprElt.elt_.token_);
                const auto dynamic = param->dynamic();
                /* == By default, dynamic parameters have 0 value and dynamic expression are necessary built on startup == */
                staticExpression &= (!dynamic);
                if (!dynamic) {
                    exprElt.elt_.type_ = RPNElementType::OPERAND;
                    exprElt.elt_.subtype_ = RPNElementSubType::VALUE;
                    exprElt.arg.value_ = static_cast<double>(param->value(params));
                }
            } else {
                exprElt.arg.value_ = std::strtod(exprElt.elt_.token_.c_str(), nullptr);
            }
            evalStack.emplace_back(exprElt.arg.value_);
            stack.emplace_back(std::move(exprElt));
            operatorStack.emplace_back(&(stack.back().elt_));
        } else {
            const auto opType = rpn::getOperatorTypeFromString(elt.token_);
            const auto &op = rpn::getOperatorFromOperatorType(opType);
            if (elt.subtype_ == RPNElementSubType::FUNCTION && operatorStack.size() < op.argCount) {
                throwSpiderException("Function [%s] expecting argument !", elt.token_.c_str());
            }
            bool skip = false;
            for (auto it = operatorStack.rbegin(); !skip && (it != (operatorStack.rbegin() + op.argCount)); ++it) {
                skip |= ((*it)->subtype_ == RPNElementSubType::PARAMETER || (*it)->type_ == RPNElementType::OPERATOR);
            }
            stack.emplace_back(std::move(elt));
            stack.back().arg.opType_ = opType;
            if (!skip && evalStack.size() >= op.argCount) {
                const auto result = rpn::apply(op.type, evalStack, evalStack.size() - op.argCount);
                for (uint8_t i = 0; i < op.argCount; ++i) {
                    stack.pop_back();
                    evalStack.pop_back();
                }
                auto &exprElt = stack.back();
                exprElt.elt_.type_ = RPNElementType::OPERAND;
                exprElt.elt_.subtype_ = RPNElementSubType::VALUE;
                exprElt.elt_.token_ = std::to_string(result);
                exprElt.arg.value_ = result;
                evalStack.emplace_back(result);
            } else {
                for (uint8_t i = 0; !evalStack.empty() && i < op.argCount; ++i) {
                    evalStack.pop_back();
                }
            }
            for (uint8_t i = 0; i < op.argCount; ++i) {
                operatorStack.pop_back();
            }
            operatorStack.emplace_back(&(stack.back().elt_));
        }
    }
    return stack;
}

double spider::Expression::evaluateStack(const spider::vector<std::shared_ptr<pisdf::Param>> &params) const {
    auto evalStack = factory::vector<double>(StackID::EXPRESSION);
    evalStack.reserve(6); /* = In practice, the evalStack will most likely not exceed 3 values = */
    for (const auto &elt : *(expressionStack_)) {
        if (elt.elt_.type_ == RPNElementType::OPERAND) {
            if (elt.elt_.subtype_ == RPNElementSubType::PARAMETER) {
                const auto param = findParam(params, elt.elt_.token_);
                evalStack.emplace_back(static_cast<double>(param->value(params)));
            } else {
                evalStack.emplace_back(elt.arg.value_);
            }
        } else {
            const auto &op = rpn::getOperatorFromOperatorType(elt.arg.opType_);
            const auto result = rpn::apply(op.type, evalStack, evalStack.size() - op.argCount);
            for (uint8_t i = 0; i < op.argCount - 1; ++i) {
                evalStack.pop_back();
            }
            evalStack.back() = result;
        }
    }
    return evalStack.back();
}
