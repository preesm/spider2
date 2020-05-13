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

/* === Include(s) === */

#include "FastExpression.h"
#include <graphs/pisdf/Param.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::FastExpression::FastExpression(std::string expression, const vector<std::shared_ptr<pisdf::Param>> &params) {
    /* == Get the postfix expression stack == */
    auto postfixStack = rpn::extractPostfixElements(std::move(expression));
    if (log::enabled<log::EXPR>()) {
        log::verbose<log::EXPR>("infix expression: [%s].\n", rpn::infixString(postfixStack).c_str());
        log::verbose<log::EXPR>("postfix expression: [%s].\n", rpn::postfixString(postfixStack).c_str());
    }

    /* == Reorder the postfix stack elements to increase the number of static evaluation done on construction == */
//    rpn::reorderPostfixStack(postfixStack);

    /* == Perform partial evaluation of the expression (if possible) == */
    auto partialPostfixStack = partialEvaluation(postfixStack, params);

    if (partialPostfixStack.size() == 1) {
        /* == Static expression is optimized away == */
        value_ = std::strtod(partialPostfixStack[0].token_.c_str(), nullptr);
    } else {
        /* == Compile the expression == */
        compile(partialPostfixStack);
    }
}

spider::FastExpression::FastExpression(int64_t value) : value_{ static_cast<double>(value) } {

}

spider::FastExpression::FastExpression(const spider::FastExpression &other) : value_{ other.value_ } {
    if (other.stack_) {
        stack_ = make<spider::vector<ExpressionNode>, StackID::EXPRESSION>(*(other.stack_));
    }
//    if (other.symbols_) {
//        symbols_ = make<unordered_map<std::string, double>, StackID::EXPRESSION>(*(other.symbols_));
//    }
}

spider::FastExpression::~FastExpression() {
    destroy(symbols_);
    destroy(stack_);
}

spider::FastExpression &spider::FastExpression::operator+=(const FastExpression &rhs) {
    if (!rhs.dynamic()) {
        value_ = value_ + rhs.value_;
    } else if (!dynamic()) {
        stack_ = make<spider::vector<ExpressionNode>, StackID::EXPRESSION>(*(rhs.stack_));
        auto val = value_;
        stack_->emplace_back([val]() { return val; }, stack_->size());
        auto &arg1 = stack_->back();
        auto &arg0 = (*stack_)[arg1.lastArgIndex_ - 1];
        stack_->emplace_back([&arg0, &arg1]() { return arg0() + arg1(); }, arg0.lastArgIndex_);
    } else {
        size_t offset = stack_->size();
        auto &arg0 = stack_->back();
        for (auto &elt : *(rhs.stack_)) {
            stack_->emplace_back(elt);
            stack_->back().lastArgIndex_ += offset;
        }
        auto &arg1 = stack_->back();
        stack_->emplace_back([&arg0, &arg1]() { return arg0() + arg1(); }, arg0.lastArgIndex_);
    }
    return *this;
}

/* === Private method(s) implementation === */

static spider::pisdf::Param *
getParam(const spider::vector<std::shared_ptr<spider::pisdf::Param>> &params, const std::string &name) {
    for (const auto &p : params) {
        if (p->name() == name) {
            return p.get();
        }
    }
    throwSpiderException("Did not find parameter [%s] for expression parsing.", name.c_str());
}

spider::vector<RPNElement> spider::FastExpression::partialEvaluation(vector<RPNElement> &postfixStack,
                                                                     const vector<std::shared_ptr<pisdf::Param>> &params) const {
    auto operatorStack = factory::vector<RPNElement *>(StackID::EXPRESSION);
    operatorStack.reserve(6);
    auto evalStack = factory::vector<double>(StackID::EXPRESSION);
    evalStack.reserve(6); /* = In practice, the evalStack will most likely not exceed 3 values = */
    auto partialPostFixStack = factory::vector<RPNElement>(StackID::EXPRESSION);
    partialPostFixStack.reserve(postfixStack.size());
    for (auto &elt : postfixStack) {
        if (elt.type_ == RPNElementType::OPERAND) {
            double value = 0.0;
            if (elt.subtype_ == RPNElementSubType::PARAMETER) {
                const auto param = getParam(params, elt.token_);
                if (!param->dynamic()) {
                    elt.type_ = RPNElementType::OPERAND;
                    elt.subtype_ = RPNElementSubType::VALUE;
                    elt.token_ = std::to_string(param->value(params));
                }
            } else {
                value = std::strtod(elt.token_.c_str(), nullptr);
            }
            evalStack.emplace_back(value);
            partialPostFixStack.emplace_back(std::move(elt));
            operatorStack.emplace_back(&(partialPostFixStack.back()));
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
            partialPostFixStack.emplace_back(std::move(elt));
            if (!skip && evalStack.size() >= op.argCount) {
                const auto result = rpn::apply(op.type, evalStack, evalStack.size() - op.argCount);
                for (uint8_t i = 0; i < op.argCount; ++i) {
                    partialPostFixStack.pop_back();
                    evalStack.pop_back();
                }
                auto &exprElt = partialPostFixStack.back();
                exprElt.type_ = RPNElementType::OPERAND;
                exprElt.subtype_ = RPNElementSubType::VALUE;
                exprElt.token_ = std::to_string(result);
                evalStack.emplace_back(result);
            } else {
                for (uint8_t i = 0; !evalStack.empty() && i < op.argCount; ++i) {
                    evalStack.pop_back();
                }
            }
            for (uint8_t i = 0; i < op.argCount; ++i) {
                operatorStack.pop_back();
            }
            operatorStack.emplace_back(&(partialPostFixStack.back()));
        }
    }
    return partialPostFixStack;
}

void spider::FastExpression::compile(spider::vector<RPNElement> &postfixStack) {
    /* == Compile partially evaluated stack into evaluation stack == */
    symbols_ = make<unordered_map<std::string, double>, StackID::EXPRESSION>();
    stack_ = make<spider::vector<ExpressionNode>, StackID::EXPRESSION>();
    stack_->reserve(postfixStack.size());
    for (auto &elt : postfixStack) {
        if (elt.type_ == RPNElementType::OPERAND) {
            if (elt.subtype_ == RPNElementSubType::PARAMETER) {
                (*symbols_)[elt.token_] = 0.0;
                const auto &value = (*symbols_)[elt.token_];
                stack_->emplace_back([&value]() { return value; }, stack_->size());
            } else {
                const auto value = std::strtod(elt.token_.c_str(), nullptr);
                stack_->emplace_back([value]() { return value; }, stack_->size());
            }
        } else {
            stack_->emplace_back(createNode(rpn::getOperatorTypeFromString(elt.token_)));
        }
    }
}

double spider::FastExpression::evaluateImpl(const vector<std::shared_ptr<pisdf::Param>> &params) const {
    /* == Update symbol table == */
    for (auto &symbol : *symbols_) {
        const auto &token = symbol.first;
        const auto &it = std::find_if(params.begin(), params.end(),
                                      [&token](const std::shared_ptr<pisdf::Param> &p) {
                                          return p->name() == token;
                                      });
        if (it != params.end()) {
            symbol.second =  static_cast<double>((*it)->value(params));
        } else {
            throwSpiderException("Did not find parameter [%s] for expression parsing.", token.c_str());
        }
    }
    /* == Evaluate stack == */
    return stack_->back()();
}

spider::FastExpression::ExpressionNode spider::FastExpression::createNode(RPNOperatorType operatorType) const {
    const auto &op = rpn::getOperatorFromOperatorType(operatorType);
    if (op.argCount == 1) {
        auto &arg = (*stack_)[stack_->size() - 1];
        switch (operatorType) {
            case RPNOperatorType::FACT:
                return { [&arg]() { return math::factorial(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::COS:
                return { [&arg]() { return std::cos(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::SIN:
                return { [&arg]() { return std::sin(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::TAN:
                return { [&arg]() { return std::tan(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::COSH:
                return { [&arg]() { return std::cosh(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::SINH:
                return { [&arg]() { return std::sinh(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::TANH:
                return { [&arg]() { return std::tanh(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::EXP:
                return { [&arg]() { return std::exp(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::LOG:
                return { [&arg]() { return std::log(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::LOG2:
                return { [&arg]() { return std::log2(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::LOG10:
                return { [&arg]() { return std::log10(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::CEIL:
                return { [&arg]() { return std::ceil(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::FLOOR:
                return { [&arg]() { return std::floor(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::ABS:
                return { [&arg]() { return math::abs(arg()); }, arg.lastArgIndex_ };
            case RPNOperatorType::SQRT:
                return { [&arg]() { return std::sqrt(arg()); }, arg.lastArgIndex_ };
            default:
                break;
        }
    } else if (op.argCount == 2) {
        auto &arg1 = (*stack_)[stack_->size() - 1];
        auto &arg0 = (*stack_)[arg1.lastArgIndex_ - 1];
        switch (operatorType) {
            case RPNOperatorType::ADD:
                return { [&arg0, &arg1]() { return arg0() + arg1(); }, arg0.lastArgIndex_ };
            case RPNOperatorType::SUB:
                return { [&arg0, &arg1]() { return arg0() - arg1(); }, arg0.lastArgIndex_ };
            case RPNOperatorType::MUL:
                return { [&arg0, &arg1]() { return arg0() * arg1(); }, arg0.lastArgIndex_ };
            case RPNOperatorType::DIV:
                return { [&arg0, &arg1]() { return arg0() / arg1(); }, arg0.lastArgIndex_ };
            case RPNOperatorType::MOD:
                return { [&arg0, &arg1]() {
                    return static_cast<double>(static_cast<int64_t>(arg0()) % static_cast<int64_t>(arg1()));
                }, arg0.lastArgIndex_ };
            case RPNOperatorType::POW:
                return { [&arg0, &arg1]() { return std::pow(arg0(), arg1()); }, arg0.lastArgIndex_ };
            case RPNOperatorType::MAX:
                return { [&arg0, &arg1]() { return std::max(arg0(), arg1()); }, arg0.lastArgIndex_ };
            case RPNOperatorType::MIN:
                return { [&arg0, &arg1]() { return std::min(arg0(), arg1()); }, arg0.lastArgIndex_ };
            case RPNOperatorType::LOG_AND:
                return { [&arg0, &arg1]() {
                    return static_cast<double>(static_cast<long>(arg0()) && static_cast<long>(arg1()));
                }, arg0.lastArgIndex_ };
            case RPNOperatorType::LOG_OR:
                return { [&arg0, &arg1]() {
                    return static_cast<double>(static_cast<long>(arg0()) || static_cast<long>(arg1()));
                }, arg0.lastArgIndex_ };
            case RPNOperatorType::GREATER:
                return { [&arg0, &arg1]() { return arg0() > arg1(); }, arg0.lastArgIndex_ };
            case RPNOperatorType::GEQ:
                return { [&arg0, &arg1]() { return arg0() >= arg1(); }, arg0.lastArgIndex_ };
            case RPNOperatorType::LESS:
                return { [&arg0, &arg1]() { return arg0() < arg1(); }, arg0.lastArgIndex_ };
            case RPNOperatorType::LEQ:
                return { [&arg0, &arg1]() { return arg0() <= arg1(); }, arg0.lastArgIndex_ };
            default:
                break;
        }
    } else if (op.argCount == 3) {
        auto &arg2 = (*stack_)[stack_->size() - 1];
        auto &arg1 = (*stack_)[arg2.lastArgIndex_ - 1];
        auto &arg0 = (*stack_)[arg1.lastArgIndex_ - 1];
        switch (operatorType) {
            case RPNOperatorType::IF:
                return { [&arg0, &arg1, &arg2]() { return arg0() >= 1. ? arg1() : arg2(); }, arg0.lastArgIndex_ };
            default:
                break;
        }
    }
    throwSpiderException("Unsupported operation.\n");
}


