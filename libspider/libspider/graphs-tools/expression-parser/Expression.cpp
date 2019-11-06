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
    return std::make_pair(nullptr, 0);
}

/* === Methods implementation === */

Expression::Expression(std::string expression, const Spider::vector<PiSDFParam *> &params) {
    /* == Get the postfix expression stack == */
    auto postfixStack = RPNConverter::extractPostfixElements(std::move(expression));
    // TODO make printVerbose call a no-op when disabled (use CMake option)
//    Spider::Logger::printVerbose(LOG_GENERAL, "infix expression: [%s].\n", RPNConverter::infixString(postfixStack));
//    Spider::Logger::printVerbose(LOG_GENERAL, "postfix expression: [%s].\n", RPNConverter::postfixString(postfixStack));

    /* == Build the expression stack == */
    expressionStack_.reserve(postfixStack.size());
    buildExpressionStack(postfixStack, params);

    /* == Reduce expression == */
    // TODO: see how to reduce expression to the smallest dynamic form
    // TODO: ((2+w)+6)*(20) -> [2 w + 6 + 20 *] -> [8 w + 20 *]
    // TODO: (w*2)*(4*h) -> [w 2 * 4 h * *] -> [w h * 8 *]
    // TODO: (4/w)/2 -> [4 w / 2 /] -> [4 2 / w /]  -> [2 w /]
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

void Expression::buildExpressionStack(Spider::vector<RPNElement> &postfixStack,
                                      const Spider::vector<PiSDFParam *> &params) {
    Spider::vector<ExpressionElt> evalStack;
    evalStack.reserve(6); /* = In practice, the evalStack will most likely not exceed 3 values = */
    bool skipEval = false;
    for (auto &elt : postfixStack) {
        if (elt.type == RPNElementType::OPERAND) {
            if (elt.subtype == RPNElementSubType::PARAMETER) {
                const auto &pair = findParam(params, elt.token);
                const auto &param = pair.first;
                if (!param) {
                    throwSpiderException("Did not find parameter [%s] for expression parsing.", elt.token.c_str());
                }
                evalStack.emplace_back(std::move(elt));
                if (param->dynamic()) {
                    static_ = false;
                    skipEval = true;
                    evalStack.back().arg.paramIx_ = pair.second;
                } else {
                    evalStack.back().arg.value_ = param->value();
                }
            } else {
                const auto &value = std::strtod(elt.token.c_str(), nullptr);
                evalStack.emplace_back(std::move(elt));
                evalStack.back().arg.value_ = value;
            }
        } else {
            const auto &opType = RPNConverter::getOperatorTypeFromString(elt.token);
            const auto &op = RPNConverter::getOperatorFromOperatorType(opType);
            if (evalStack.size() < op.argCount && elt.subtype == RPNElementSubType::FUNCTION) {
                throwSpiderException("Function [%s] expecting argument !", elt.token.c_str());
            }
            if (skipEval || evalStack.size() < op.argCount) {
                /* == Push elements in expression stack == */
                for (auto &e : evalStack) {
                    expressionStack_.emplace_back(std::move(e));
                }
                evalStack.clear();
                expressionStack_.emplace_back(std::move(elt));
                expressionStack_.back().arg.opType_ = opType;
            } else {
                /* == Do in-place eval == */
                if (op.argCount == 2) {
                    auto &arg1 = evalStack.back();
                    evalStack.pop_back();
                    auto &arg2 = evalStack.back();
                    evalStack.pop_back();
                    const auto &value = op.eval(arg2.arg.value_, arg1.arg.value_);
                    evalStack.emplace_back(RPNElement(RPNElementType::OPERAND,
                                                      RPNElementSubType::VALUE,
                                                      std::to_string(value)));
                    evalStack.back().arg.value_ = value;
                } else {
                    const auto &arg1 = evalStack.back();
                    evalStack.pop_back();
                    const auto &value = op.eval(arg1.arg.value_, 0);
                    evalStack.emplace_back(RPNElement(RPNElementType::OPERAND,
                                                      RPNElementSubType::VALUE,
                                                      std::to_string(value)));
                    evalStack.back().arg.value_ = value;
                }
            }
            skipEval = false;
        }
    }
    for (auto &e : evalStack) {
        expressionStack_.emplace_back(std::move(e));
    }
}

double Expression::evaluateStack(const Spider::vector<PiSDFParam *> &params) const {
    Spider::vector<double> evalStack;
    evalStack.reserve(6); /* = In practice, the evalStack will most likely not exceed 3 values = */
    for (auto &elt : expressionStack_) {
        if (elt.elt_.type == RPNElementType::OPERAND) {
            if (elt.elt_.subtype == RPNElementSubType::PARAMETER) {
                evalStack.push_back(params[elt.arg.paramIx_]->value());
            } else {
                evalStack.push_back(elt.arg.value_);
            }
        } else {
            const auto &op = RPNConverter::getOperatorFromOperatorType(elt.arg.opType_);
            if (op.argCount == 2) {
                auto &arg1 = evalStack.back();
                evalStack.pop_back();
                auto &arg2 = evalStack.back();
                arg2 = op.eval(arg2, arg1);
            } else {
                auto &arg1 = evalStack.back();
                arg1 = (op.eval(arg1, 0));
            }
        }
    }
    return evalStack.back();
}
