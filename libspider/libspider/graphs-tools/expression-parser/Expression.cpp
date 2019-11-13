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

#include <graphs-tools/expression-parser/Expression.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Graph.h>
#include <algorithm>
#include <cmath>

/* === Static method(s) === */

static std::pair<PiSDFParam *, std::uint32_t>
findParam(const Spider::vector<PiSDFParam *> &params, const std::string &name) {
    std::uint32_t ix = 0;
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
            return static_cast<std::int64_t>((*start)) % static_cast<std::int64_t>((*(start + 1)));
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
                Spider::Logger::error<LOG_EXPR>("Unsupported operation.\n");
            }
    }
    return 0;
}

/* === Methods implementation === */

Expression::Expression(std::string expression, const Spider::vector<PiSDFParam *> &params) {
    /* == Get the postfix expression stack == */
    auto postfixStack = RPNConverter::extractPostfixElements(std::move(expression));
    if (Spider::API::verbose() && log_enabled<LOG_EXPR>()) {
        Spider::Logger::verbose<LOG_EXPR>("infix expression: [%s].\n", RPNConverter::infixString(postfixStack).c_str());
        Spider::Logger::verbose<LOG_EXPR>("postfix expression: [%s].\n",
                                          RPNConverter::postfixString(postfixStack).c_str());
    }

    /* == Reorder the postfix stack elements to increase the number of static evaluation done on construction == */
    RPNConverter::reorderPostfixStack(postfixStack);

    /* == Build the expression stack == */
    expressionStack_ = buildExpressionStack(postfixStack, params);

    if (static_) {
        value_ = expressionStack_.empty() ? 0. : expressionStack_.back().arg.value_;
    }
}

Expression::Expression(std::int64_t value) {
    static_ = true;
    value_ = value;
}

std::string Expression::string() const {
    /* == Build the postfix string expression == */
    std::string postfixExpr;
    if (expressionStack_.empty()) {
        postfixExpr = std::to_string(value_);
    }
    for (auto &t : expressionStack_) {
        postfixExpr += t.elt_.token + " ";
    }
    return postfixExpr;
}

/* === Private method(s) === */

Spider::vector<ExpressionElt> Expression::buildExpressionStack(Spider::vector<RPNElement> &postfixStack,
                                                               const Spider::vector<PiSDFParam *> &params) {
    Spider::vector<ExpressionElt> stack;
    stack.reserve(postfixStack.size());
    Spider::vector<double> evalStack;
    evalStack.reserve(6); /* = In practice, the evalStack will most likely not exceed 3 values = */
    bool skipEval = false;
    std::uint8_t argCount = 0;
    for (auto &elt : postfixStack) {
        if (elt.type == RPNElementType::OPERAND) {
            argCount += 1;
            if (elt.subtype == RPNElementSubType::PARAMETER) {
                const auto &pair = findParam(params, elt.token);
                const auto &param = pair.first;
                const auto &dynamic = param->dynamic();
                const auto &value = param->value();
                evalStack.push_back(value);

                /* == By default, dynamic parameters have 0 value and dynamic expression are necessary built on startup == */
                static_ &= (!dynamic);
                skipEval |= dynamic;
                stack.emplace_back(std::move(elt));
                stack.back().arg.value_ = static_cast<double>(dynamic & pair.second) + value;
            } else {
                const auto &value = std::strtod(elt.token.c_str(), nullptr);
                evalStack.push_back(value);
                stack.emplace_back(std::move(elt));
                stack.back().arg.value_ = value;
            }
        } else {
            const auto &opType = RPNConverter::getOperatorTypeFromString(elt.token);
            const auto &op = RPNConverter::getOperatorFromOperatorType(opType);
            if (elt.subtype == RPNElementSubType::FUNCTION && argCount < op.argCount) {
                throwSpiderException("Function [%s] expecting argument !", elt.token.c_str());
            }
            stack.emplace_back(std::move(elt));
            stack.back().arg.opType_ = opType;
            if (!skipEval && evalStack.size() >= op.argCount) {
                auto &&result = applyOperator(evalStack.begin() + (evalStack.size() - op.argCount), op.type);
                for (std::uint8_t i = 0; i < op.argCount; ++i) {
                    stack.pop_back();
                    evalStack.pop_back();
                }
                stack.back().elt_.type = RPNElementType::OPERAND;
                stack.back().elt_.subtype = RPNElementSubType::VALUE;
                stack.back().elt_.token = std::to_string(result);
                stack.back().arg.value_ = result;
                evalStack.push_back(result);
            } else {
                for (std::uint8_t i = 0; !evalStack.empty() && i < op.argCount; ++i) {
                    evalStack.pop_back();
                }
            }
            argCount = evalStack.size();
            skipEval = false;
        }
    }
    return stack;
}

double Expression::evaluateStack(const Spider::vector<PiSDFParam *> &params) const {
    Spider::vector<double> evalStack;
    evalStack.reserve(6); /* = In practice, the evalStack will most likely not exceed 3 values = */
    for (const auto &elt : expressionStack_) {
        if (elt.elt_.type == RPNElementType::OPERAND) {
            if (elt.elt_.subtype == RPNElementSubType::PARAMETER) {
                evalStack.push_back(params[elt.arg.value_]->value());
            } else {
                evalStack.push_back(elt.arg.value_);
            }
        } else {
            const auto &op = RPNConverter::getOperatorFromOperatorType(elt.arg.opType_);
            auto &&result = applyOperator(evalStack.begin() + (evalStack.size() - op.argCount), op.type);
            for (std::uint8_t i = 0; i < op.argCount - 1; ++i) {
                evalStack.pop_back();
            }
            evalStack.back() = result;
        }
    }
    return evalStack.back();
}
