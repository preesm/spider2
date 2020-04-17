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

#include <cmath>
#include <common/Math.h>
#include <containers/vector.h>
#include <graphs-tools/expression-parser/Expression.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Graph.h>
#include <common/Printer.h>

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

template<class StartIterator>
static double applyOperator(StartIterator start, RPNOperatorType type) {
    switch (type) {
        case RPNOperatorType::ADD:
            return *start + *(start + 1);
        case RPNOperatorType::SUB:
            return *start - *(start + 1);
        case RPNOperatorType::MUL:
            return *start * *(start + 1);
        case RPNOperatorType::DIV:
            return *start / *(start + 1);
        case RPNOperatorType::MOD:
            return static_cast<double>(static_cast<int64_t>(*start) % static_cast<int64_t>(*(start + 1)));
        case RPNOperatorType::POW:
            return std::pow(*start, (*(start + 1)));
        case RPNOperatorType::FACT:
            return spider::math::factorial(*start);
        case RPNOperatorType::COS:
            return std::cos(*start);
        case RPNOperatorType::SIN:
            return std::sin(*start);
        case RPNOperatorType::TAN:
            return std::tan(*start);
        case RPNOperatorType::COSH:
            return std::cosh(*start);
        case RPNOperatorType::SINH:
            return std::sinh(*start);
        case RPNOperatorType::TANH:
            return std::tanh(*start);
        case RPNOperatorType::EXP:
            return std::exp(*start);
        case RPNOperatorType::LOG:
            return std::log(*start);
        case RPNOperatorType::LOG2:
            return std::log2(*start);
        case RPNOperatorType::LOG10:
            return std::log10(*start);
        case RPNOperatorType::CEIL:
            return std::ceil(*start);
        case RPNOperatorType::FLOOR:
            return std::floor(*start);
        case RPNOperatorType::ABS:
            return spider::math::abs(*start);
        case RPNOperatorType::SQRT:
            return std::sqrt(*start);
        case RPNOperatorType::MAX:
            return std::max(*start, *(start + 1));
        case RPNOperatorType::MIN:
            return std::min(*start, *(start + 1));
        case RPNOperatorType::LOG_AND:
            return static_cast<double>(static_cast<long>(*start) && static_cast<long>(*(start + 1)));
        case RPNOperatorType::LOG_OR:
            return static_cast<double>(static_cast<long>(*start) || static_cast<long>(*(start + 1)));
        case RPNOperatorType::IF:
            if (*start >= 1.) {
                return *(start + 1);
            }
            return *(start + 2);
        case RPNOperatorType::GREATER:
            return *start > *(start + 1);
        case RPNOperatorType::GEQ:
            return *start >= *(start + 1);
        case RPNOperatorType::LESS:
            return *start < *(start + 1);
        case RPNOperatorType::LEQ:
            return *start <= *(start + 1);
        case RPNOperatorType::LEFT_PAR:
        case RPNOperatorType::RIGHT_PAR:
        case RPNOperatorType::DUMMY:
        default:
            if (spider::log::enabled<spider::log::EXPR>()) {
                spider::log::error<spider::log::EXPR>("Unsupported operation.\n");
            }
    }
    return 0;
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
    rpn::reorderPostfixStack(postfixStack);

    /* == Build the expression stack == */
    bool staticExpression = true;
    auto stack = buildExpressionStack(postfixStack, params, staticExpression);

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

std::string spider::Expression::string() const {
    if (!expressionStack_ || expressionStack_->empty()) {
        return std::to_string(value_);
    }
    /* == Build the postfix string expression == */
    std::string postfixExpr;
    for (auto &t : *(expressionStack_)) {
        postfixExpr += t.elt_.token_ + " ";
    }
    postfixExpr.pop_back();
    return postfixExpr;
}

/* === Private method(s) === */

spider::vector<spider::ExpressionElt>
spider::Expression::buildExpressionStack(spider::vector<RPNElement> &postfixStack,
                                         const spider::vector<std::shared_ptr<pisdf::Param>> &params,
                                         bool &staticExpression) {
    auto stack = factory::vector<ExpressionElt>(StackID::EXPRESSION);
    stack.reserve(postfixStack.size());
    auto evalStack = factory::vector<double>(StackID::EXPRESSION);
    evalStack.reserve(6); /* = In practice, the evalStack will most likely not exceed 3 values = */
    size_t argCount = 0;
    for (auto &elt : postfixStack) {
        if (elt.type_ == RPNElementType::OPERAND) {
            argCount += 1;
            if (elt.subtype_ == RPNElementSubType::PARAMETER) {
                const auto param = findParam(params, elt.token_);
                const auto dynamic = param->dynamic();
                const auto value = static_cast<double>(param->value(params));
                evalStack.emplace_back(value);

                /* == By default, dynamic parameters have 0 value and dynamic expression are necessary built on startup == */
                staticExpression &= (!dynamic);
                stack.emplace_back(std::move(elt));
                if (!dynamic) {
                    stack.back().elt_.type_ = RPNElementType::OPERAND;
                    stack.back().elt_.subtype_ = RPNElementSubType::VALUE;
                    stack.back().arg.value_ = value;
                }
            } else {
                const auto value = std::strtod(elt.token_.c_str(), nullptr);
                evalStack.emplace_back(value);
                stack.emplace_back(std::move(elt));
                stack.back().arg.value_ = value;
            }
        } else {
            const auto opType = rpn::getOperatorTypeFromString(elt.token_);
            const auto &op = rpn::getOperatorFromOperatorType(opType);
            if (elt.subtype_ == RPNElementSubType::FUNCTION && argCount < op.argCount) {
                throwSpiderException("Function [%s] expecting argument !", elt.token_.c_str());
            }
            bool skip = false;
            for (auto it = stack.rbegin(); !skip && (it != (stack.rbegin() + op.argCount)); ++it) {
                skip |= (*it).elt_.subtype_ == RPNElementSubType::PARAMETER;
            }
            stack.emplace_back(std::move(elt));
            stack.back().arg.opType_ = opType;
            if (!skip && evalStack.size() >= op.argCount) {
                auto &&result = applyOperator(
                        evalStack.begin() + (static_cast<int64_t>(evalStack.size() - op.argCount)), op.type);
                for (uint8_t i = 0; i < op.argCount; ++i) {
                    stack.pop_back();
                    evalStack.pop_back();
                }
                stack.back().elt_.type_ = RPNElementType::OPERAND;
                stack.back().elt_.subtype_ = RPNElementSubType::VALUE;
                stack.back().elt_.token_ = std::to_string(result);
                stack.back().arg.value_ = result;
                evalStack.emplace_back(result);
            } else {
                for (uint8_t i = 0; !evalStack.empty() && i < op.argCount; ++i) {
                    evalStack.pop_back();
                }
            }
            argCount = evalStack.size();
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
            auto &&result = applyOperator(evalStack.begin() + (static_cast<int64_t>(evalStack.size() - op.argCount)),
                                          op.type);
            for (uint8_t i = 0; i < op.argCount - 1; ++i) {
                evalStack.pop_back();
            }
            evalStack.back() = result;
        }
    }
    return evalStack.back();
}
