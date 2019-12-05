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

/* === Includes === */

#include <cmath>
#include <graphs-tools/expression-parser/Expression.h>
#include <graphs/pisdf/params/Param.h>
#include <graphs/pisdf/Graph.h>
#include <api/config-api.h>

/* === Static method(s) === */

static std::pair<PiSDFParam *, uint32_t>
findParam(const spider::vector<PiSDFParam *> &params, const std::string &name) {
    uint32_t ix = 0;
    for (const auto &p : params) {
        if (p->name() == name) {
            return std::make_pair(p, ix);
        }
        ix += 1;
    }
    throwSpiderException("Did not find parameter [%s] for expression parsing.", name.c_str());
}

template<class StartIterator>
static double applyOperator(StartIterator start, RPNOperatorType type) {
    switch (type) {
        case RPNOperatorType::ADD:
            return (*start) + (*(start + 1));
        case RPNOperatorType::SUB:
            return (*start) - (*(start + 1));
        case RPNOperatorType::MUL:
            return (*start) * (*(start + 1));
        case RPNOperatorType::DIV:
            return (*start) / (*(start + 1));
        case RPNOperatorType::MOD:
            return static_cast<double>(static_cast<int64_t>((*start)) % static_cast<int64_t>((*(start + 1))));
        case RPNOperatorType::POW:
            return std::pow((*start), (*(start + 1)));
        case RPNOperatorType::COS:
            return std::cos((*start));
        case RPNOperatorType::SIN:
            return std::sin((*start));
        case RPNOperatorType::TAN:
            return std::tan((*start));
        case RPNOperatorType::EXP:
            return std::exp((*start));
        case RPNOperatorType::LOG:
            return std::log((*start));
        case RPNOperatorType::LOG2:
            return std::log2((*start));
        case RPNOperatorType::CEIL:
            return std::ceil((*start));
        case RPNOperatorType::FLOOR:
            return std::floor((*start));
        case RPNOperatorType::SQRT:
            return std::sqrt((*start));
        case RPNOperatorType::MAX:
            return std::max((*start), (*(start + 1)));
        case RPNOperatorType::MIN:
            return std::min((*start), (*(start + 1)));
        default:
            if (log_enabled<LOG_EXPR>()) {
                spider::log::error<LOG_EXPR>("Unsupported operation.\n");
            }
    }
    return 0;
}

/* === Methods implementation === */

spider::Expression::Expression(std::string expression, const spider::vector<PiSDFParam *> &params) {
    /* == Get the postfix expression stack == */
    auto postfixStack = spider::rpn::extractPostfixElements(std::move(expression));
    if (spider::api::verbose() && log_enabled<LOG_EXPR>()) {
        spider::log::verbose<LOG_EXPR>("infix expression: [%s].\n", spider::rpn::infixString(postfixStack).c_str());
        spider::log::verbose<LOG_EXPR>("postfix expression: [%s].\n",
                                       spider::rpn::postfixString(postfixStack).c_str());
    }

    /* == Reorder the postfix stack elements to increase the number of static evaluation done on construction == */
    spider::rpn::reorderPostfixStack(postfixStack);

    /* == Build the expression stack == */
    auto stack = buildExpressionStack(postfixStack, params);

    if (static_) {
        value_ = stack.empty() ? 0. : stack.back().arg.value_;
    } else {
        /* == Doing dynamic alloc of vector member if stack is not static == */
        expressionStack_ = spider::make<spider::vector<ExpressionElt>, StackID::EXPRESSION>(std::move(stack));
    }
}

spider::Expression::Expression(int64_t value) {
    static_ = true;
    value_ = static_cast<double>(value);
}

spider::Expression::~Expression() {
    spider::destroy(expressionStack_);
}

std::string spider::Expression::string() const {
    if (!expressionStack_ || expressionStack_->empty()) {
        return std::to_string(value_);
    }
    /* == Build the postfix string expression == */
    std::string postfixExpr;
    for (auto &t : *(expressionStack_)) {
        postfixExpr += t.elt_.token + " ";
    }
    postfixExpr.pop_back();
    return postfixExpr;
}

/* === Private method(s) === */

spider::vector<spider::ExpressionElt> spider::Expression::buildExpressionStack(spider::vector<RPNElement> &postfixStack,
                                                                               const spider::vector<PiSDFParam *> &params) {
    auto stack = spider::containers::vector<ExpressionElt>(StackID::EXPRESSION);
    stack.reserve(postfixStack.size());
    auto evalStack = spider::containers::vector<double>(StackID::EXPRESSION);
    evalStack.reserve(6); /* = In practice, the evalStack will most likely not exceed 3 values = */
    bool skipEval = false;
    size_t argCount = 0;
    for (auto &elt : postfixStack) {
        if (elt.type == RPNElementType::OPERAND) {
            argCount += 1;
            if (elt.subtype == RPNElementSubType::PARAMETER) {
                const auto &pair = findParam(params, elt.token);
                const auto &param = pair.first;
                const auto &dynamic = param->dynamic();
                const auto &value = static_cast<double>(param->value());
                evalStack.emplace_back(value);

                /* == By default, dynamic parameters have 0 value and dynamic expression are necessary built on startup == */
                static_ &= (!dynamic);
                skipEval = skipEval | dynamic;
                stack.emplace_back(std::move(elt));
                stack.back().arg.value_ = dynamic ? static_cast<double>(pair.second) : value;
            } else {
                const auto &value = std::strtod(elt.token.c_str(), nullptr);
                evalStack.emplace_back(value);
                stack.emplace_back(std::move(elt));
                stack.back().arg.value_ = value;
            }
        } else {
            const auto &opType = spider::rpn::getOperatorTypeFromString(elt.token);
            const auto &op = spider::rpn::getOperatorFromOperatorType(opType);
            if (elt.subtype == RPNElementSubType::FUNCTION && argCount < op.argCount) {
                throwSpiderException("Function [%s] expecting argument !", elt.token.c_str());
            }
            stack.emplace_back(std::move(elt));
            stack.back().arg.opType_ = opType;
            if (!skipEval && evalStack.size() >= op.argCount) {
                auto &&result = applyOperator(
                        evalStack.begin() + (static_cast<int64_t>(evalStack.size() - op.argCount)), op.type);
                for (uint8_t i = 0; i < op.argCount; ++i) {
                    stack.pop_back();
                    evalStack.pop_back();
                }
                stack.back().elt_.type = RPNElementType::OPERAND;
                stack.back().elt_.subtype = RPNElementSubType::VALUE;
                stack.back().elt_.token = std::to_string(result);
                stack.back().arg.value_ = result;
                evalStack.emplace_back(result);
            } else {
                for (uint8_t i = 0; !evalStack.empty() && i < op.argCount; ++i) {
                    evalStack.pop_back();
                }
            }
            argCount = evalStack.size();
            skipEval = false;
        }
    }
    return stack;
}

double spider::Expression::evaluateStack(const spider::vector<PiSDFParam *> &params) const {
    auto evalStack = spider::containers::vector<double>(StackID::EXPRESSION);
    evalStack.reserve(6); /* = In practice, the evalStack will most likely not exceed 3 values = */
    for (const auto &elt : *(expressionStack_)) {
        if (elt.elt_.type == RPNElementType::OPERAND) {
            if (elt.elt_.subtype == RPNElementSubType::PARAMETER) {
                evalStack.emplace_back(static_cast<double>(params[static_cast<size_t>(elt.arg.value_)]->value()));
            } else {
                evalStack.emplace_back(elt.arg.value_);
            }
        } else {
            const auto &op = spider::rpn::getOperatorFromOperatorType(elt.arg.opType_);
            auto &&result = applyOperator(
                    evalStack.begin() + (static_cast<int64_t>(evalStack.size() - op.argCount)), op.type);
            for (uint8_t i = 0; i < op.argCount - 1; ++i) {
                evalStack.pop_back();
            }
            evalStack.back() = result;
        }
    }
    return evalStack.back();
}
