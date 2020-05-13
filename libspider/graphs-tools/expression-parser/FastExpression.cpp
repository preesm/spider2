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

#include <graphs-tools/expression-parser/FastExpression.h>
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

    /* == Perform partial evaluation of the expression (if possible) and compile the expression stack == */
    compile(postfixStack, params);
}

spider::FastExpression::FastExpression(int64_t value) : value_{ static_cast<double>(value) } { }

spider::FastExpression::FastExpression(const spider::FastExpression &other) : value_{ other.value_ } {
    if (other.stack_) {
        stack_ = make<spider::vector<ExpressionNode>, StackID::EXPRESSION>(*(other.stack_));
    }
    if (other.symbols_) {
        symbols_ = make<unordered_map<std::string, double>, StackID::EXPRESSION>(*(other.symbols_));
    }
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
        stack_->emplace_back([val]() { return val; }, stack_->size(), RPNElementSubType::VALUE);
        auto &arg1 = stack_->back();
        auto &arg0 = (*stack_)[arg1.lastArgIndex_ - 1];
        stack_->emplace_back([&arg0, &arg1]() { return arg0() + arg1(); }, arg0.lastArgIndex_,
                             RPNElementSubType::FUNCTION);
    } else {
        size_t offset = stack_->size();
        auto &arg0 = stack_->back();
        for (auto &elt : *(rhs.stack_)) {
            stack_->emplace_back(elt);
            stack_->back().lastArgIndex_ += offset;
        }
        auto &arg1 = stack_->back();
        stack_->emplace_back([&arg0, &arg1]() { return arg0() + arg1(); }, arg0.lastArgIndex_,
                             RPNElementSubType::FUNCTION);
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

void spider::FastExpression::compile(vector<RPNElement> &postfixStack,
                                     const vector<std::shared_ptr<pisdf::Param>> &params) {
    /* == Compile partially evaluated stack into evaluation stack == */
    auto localSymbols = factory::unordered_map<std::string, double>(StackID::EXPRESSION);
    auto stack = factory::vector<ExpressionNode>(StackID::EXPRESSION);
    stack.reserve(postfixStack.size());
    for (auto &elt : postfixStack) {
        if (elt.type_ == RPNElementType::OPERAND) {
            if (elt.subtype_ == RPNElementSubType::PARAMETER) {
                const auto param = getParam(params, elt.token_);
                if (!param->dynamic()) {
                    const auto value = static_cast<double>(param->value());
                    stack.emplace_back([value]() { return value; }, stack.size(), RPNElementSubType::VALUE);
                } else {
                    localSymbols[elt.token_] = 0.0;
                    const auto &value = localSymbols[elt.token_];
                    stack.emplace_back([&value]() { return value; }, stack.size(), RPNElementSubType::PARAMETER);
                }
            } else {
                const auto value = std::strtod(elt.token_.c_str(), nullptr);
                stack.emplace_back([value]() { return value; }, stack.size(), RPNElementSubType::VALUE);
            }
        } else {
            const auto &opType = rpn::getOperatorTypeFromString(elt.token_);
            const auto &op = rpn::getOperatorFromOperatorType(opType);
            if (stack.size() < op.argCount) {
                throwSpiderException("Function [%s] expecting argument !", elt.token_.c_str());
            }
            auto *arg = &(stack.back());
            auto isEvaluable = arg->type_ == RPNElementSubType::VALUE;
            for (uint8_t i = 1; isEvaluable && (i < op.argCount); ++i) {
                arg = &(stack[arg->lastArgIndex_ - 1]);
                isEvaluable &= (arg->type_ == RPNElementSubType::VALUE);
            }
            auto expressionNode = createNode(rpn::getOperatorTypeFromString(elt.token_), stack);
            if (isEvaluable) {
                const auto value = expressionNode();
                for (uint8_t i = 0; i < op.argCount; ++i) {
                    stack.pop_back();
                }
                stack.emplace_back([value]() { return value; }, stack.size(), RPNElementSubType::VALUE);
            } else {
                stack.emplace_back(std::move(expressionNode));
            }
        }
    }
    if (localSymbols.empty()) {
        /* == Static expression is optimized away == */
        value_ = stack.size() == 1 ? stack.back()() : 0.;
    } else {
        symbols_ = make<unordered_map<std::string, double>, StackID::EXPRESSION>(std::move(localSymbols));
        stack_ = make<spider::vector<ExpressionNode>, StackID::EXPRESSION>(std::move(stack));
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
            symbol.second = static_cast<double>((*it)->value(params));
        } else {
            throwSpiderException("Did not find parameter [%s] for expression parsing.", token.c_str());
        }
    }
    /* == Evaluate stack == */
    return stack_->back()();
}

spider::FastExpression::ExpressionNode spider::FastExpression::createNode(RPNOperatorType operatorType,
                                                                          spider::vector<ExpressionNode> &stack) const {
    const auto &op = rpn::getOperatorFromOperatorType(operatorType);
    if (op.argCount == 1) {
        auto &arg = stack.back();
        switch (operatorType) {
            case RPNOperatorType::FACT:
                return { [&arg]() { return math::factorial(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::COS:
                return { [&arg]() { return std::cos(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::SIN:
                return { [&arg]() { return std::sin(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::TAN:
                return { [&arg]() { return std::tan(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::COSH:
                return { [&arg]() { return std::cosh(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::SINH:
                return { [&arg]() { return std::sinh(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::TANH:
                return { [&arg]() { return std::tanh(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::EXP:
                return { [&arg]() { return std::exp(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::LOG:
                return { [&arg]() { return std::log(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::LOG2:
                return { [&arg]() { return std::log2(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::LOG10:
                return { [&arg]() { return std::log10(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::CEIL:
                return { [&arg]() { return std::ceil(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::FLOOR:
                return { [&arg]() { return std::floor(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::ABS:
                return { [&arg]() { return math::abs(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::SQRT:
                return { [&arg]() { return std::sqrt(arg()); }, arg.lastArgIndex_, RPNElementSubType::FUNCTION };
            default:
                break;
        }
    } else if (op.argCount == 2) {
        auto &arg1 = stack.back();
        auto &arg0 = stack[arg1.lastArgIndex_ - 1];
        switch (operatorType) {
            case RPNOperatorType::ADD:
                return { [&arg0, &arg1]() { return arg0() + arg1(); }, arg0.lastArgIndex_,
                         RPNElementSubType::FUNCTION };
            case RPNOperatorType::SUB:
                return { [&arg0, &arg1]() { return arg0() - arg1(); }, arg0.lastArgIndex_,
                         RPNElementSubType::FUNCTION };
            case RPNOperatorType::MUL:
                return { [&arg0, &arg1]() { return arg0() * arg1(); }, arg0.lastArgIndex_,
                         RPNElementSubType::FUNCTION };
            case RPNOperatorType::DIV:
                return { [&arg0, &arg1]() { return arg0() / arg1(); }, arg0.lastArgIndex_,
                         RPNElementSubType::FUNCTION };
            case RPNOperatorType::MOD:
                return { [&arg0, &arg1]() {
                    return static_cast<double>(static_cast<int64_t>(arg0()) % static_cast<int64_t>(arg1()));
                }, arg0.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::POW:
                return { [&arg0, &arg1]() { return std::pow(arg0(), arg1()); }, arg0.lastArgIndex_,
                         RPNElementSubType::FUNCTION };
            case RPNOperatorType::MAX:
                return { [&arg0, &arg1]() { return std::max(arg0(), arg1()); }, arg0.lastArgIndex_,
                         RPNElementSubType::FUNCTION };
            case RPNOperatorType::MIN:
                return { [&arg0, &arg1]() { return std::min(arg0(), arg1()); }, arg0.lastArgIndex_,
                         RPNElementSubType::FUNCTION };
            case RPNOperatorType::LOG_AND:
                return { [&arg0, &arg1]() {
                    return static_cast<double>(static_cast<long>(arg0()) && static_cast<long>(arg1()));
                }, arg0.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::LOG_OR:
                return { [&arg0, &arg1]() {
                    return static_cast<double>(static_cast<long>(arg0()) || static_cast<long>(arg1()));
                }, arg0.lastArgIndex_, RPNElementSubType::FUNCTION };
            case RPNOperatorType::GREATER:
                return { [&arg0, &arg1]() { return arg0() > arg1(); }, arg0.lastArgIndex_,
                         RPNElementSubType::FUNCTION };
            case RPNOperatorType::GEQ:
                return { [&arg0, &arg1]() { return arg0() >= arg1(); }, arg0.lastArgIndex_,
                         RPNElementSubType::FUNCTION };
            case RPNOperatorType::LESS:
                return { [&arg0, &arg1]() { return arg0() < arg1(); }, arg0.lastArgIndex_,
                         RPNElementSubType::FUNCTION };
            case RPNOperatorType::LEQ:
                return { [&arg0, &arg1]() { return arg0() <= arg1(); }, arg0.lastArgIndex_,
                         RPNElementSubType::FUNCTION };
            default:
                break;
        }
    } else if (op.argCount == 3) {
        auto &arg2 = stack.back();
        auto &arg1 = stack[arg2.lastArgIndex_ - 1];
        auto &arg0 = stack[arg1.lastArgIndex_ - 1];
        switch (operatorType) {
            case RPNOperatorType::IF:
                return { [&arg0, &arg1, &arg2]() { return arg0() >= 1. ? arg1() : arg2(); }, arg0.lastArgIndex_,
                         RPNElementSubType::FUNCTION };
            default:
                break;
        }
    }
    throwSpiderException("Unsupported operation.\n");
}


