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
#include "Expression.h"

/* === Static method(s) === */

/* === Methods implementation === */

Expression::Expression(std::string expression, const PiSDFGraph *graph) {
    /* == Get the postfix expression stack == */
    auto postfixStack = RPNConverter::extractPostfixElements(std::move(expression));
    // TODO make printVerbose call a no-op when disabled (use CMake option)
//    Spider::Logger::printVerbose(LOG_GENERAL, "infix expression: [%s].\n", RPNConverter::infixString(postfixStack));
//    Spider::Logger::printVerbose(LOG_GENERAL, "postfix expression: [%s].\n", RPNConverter::postfixString(postfixStack));

    /* == Build expression stack == */
    expressionTree_.reserve(postfixStack.size());
    for (auto elt = postfixStack.rbegin(); elt != postfixStack.rend(); ++elt) {
        auto exprNode = ExpressionTreeNode(nullptr, std::move((*elt)));
        if (exprNode.elt.subtype == RPNElementSubType::PARAMETER) {
            if (!graph) {
                throwSpiderException("nullptr graph in expression containing parameter.");
            }
            auto *param = graph->param(exprNode.elt.token);
            if (!param) {
                throwSpiderException("Did not find parameter [%s] for expression parsing.", exprNode.elt.token.c_str());
            }
            if (param->dynamic()) {
                static_ = false;
                exprNode.arg.param = param;
            } else {
                exprNode.elt.subtype = RPNElementSubType::VALUE;
                exprNode.arg.value = param->value();
            }
        } else if (exprNode.elt.subtype == RPNElementSubType::VALUE) {
            exprNode.arg.value = std::strtod(exprNode.elt.token.c_str(), nullptr);
        } else {
            exprNode.arg.operatorType = RPNConverter::getOperatorTypeFromString(exprNode.elt.token);
        }
        expressionTree_.push_back(std::move(exprNode));
    }

    /* == Build and reduce the expression tree for fast resolving == */
    buildExpressionTree();

    /* == If static, evaluate and delete the expression == */
    if (static_) {
        value_ = evaluateNode(&expressionTree_[0]);
    }
}

Expression::Expression(std::int64_t value) {
    static_ = true;
    value_ = value;
}

void Expression::printExpressionTree() {
    if (!expressionTree_.empty()) {
        printExpressionTreeNode(&expressionTree_[0], 0);
    }
}

static std::string elementToString(const ExpressionTreeNode *node) {
    if (!node) {
        return "";
    }
    const auto &elt = node->elt;
    if (elt.type == RPNElementType::OPERAND) {
        if (elt.subtype == RPNElementSubType::PARAMETER) {
            return elt.token;
        }

        /* == Due to the automatic reduction in the buildExpressionTree method, token might not correspond to actual value == */
        return std::to_string(node->arg.value);
    }
    const auto &operatorType = RPNConverter::getOperatorTypeFromString(elt.token);
    const auto &op = RPNConverter::getOperatorFromOperatorType(operatorType);
    if (elt.subtype == RPNElementSubType::FUNCTION) {
        if (op.argCount == 1) {
            return elt.token + '(' + elementToString(node->right) + ')';
        }
        return elt.token + '(' + elementToString(node->left) + ',' + elementToString(node->right) + ')';
    }
    return '(' + elementToString(node->left) + elt.token + elementToString(node->right) + ')';
}

std::string Expression::string() const {
    if (expressionTree_.empty()) {
        return std::to_string(value_);
    }
    return elementToString(&expressionTree_[0]);
}

/* === Private method(s) === */

void Expression::buildExpressionTree() {
    if (expressionTree_.empty()) {
        return;
    }
    std::size_t nodeIx = 0;
    auto *node = &expressionTree_[0];
    while (node) {
        if (!node->right && node->elt.type == RPNElementType::OPERATOR) {
            if ((nodeIx + 1) >= expressionTree_.size()) {
                throwSpiderException("operator [%s] missing operand.", node->elt.token.c_str());
            }
            node->right = &expressionTree_[++nodeIx];
            node->right->parent = node;
            node = node->right;
        } else if (!node->left && node->elt.type != RPNElementType::OPERAND &&
                   RPNConverter::getOperatorFromOperatorType(node->arg.operatorType).argCount != 1) {
            if ((nodeIx + 1) >= expressionTree_.size()) {
                throwSpiderException("operator [%s] missing operand.", node->elt.token.c_str());
            }
            node->left = &expressionTree_[++nodeIx];
            node->left->parent = node;
            node = node->left;
        } else {
            auto *left = node->left;
            auto *right = node->right;
            if (node->elt.type == RPNElementType::OPERATOR && right && right->elt.subtype == RPNElementSubType::VALUE) {
                auto valRight = right->arg.value;
                if (!left || left->elt.subtype == RPNElementSubType::VALUE) {
                    auto valLeft = left ? left->arg.value : 0.;
                    node->elt.type = RPNElementType::OPERAND;
                    node->elt.subtype = RPNElementSubType::VALUE;
                    node->arg.value = RPNConverter::getOperatorFromOperatorType(node->arg.operatorType).eval(valLeft,
                                                                                                             valRight);
                    node->left = nullptr;
                    node->right = nullptr;
                }
            }
            node = node->parent;
        }
    }
}

double Expression::evaluateNode(const ExpressionTreeNode *node) const {
    if (!node) {
        return 0.;
    }
    auto &elt = node->elt;
    if (elt.type == RPNElementType::OPERAND) {
        if (elt.subtype == RPNElementSubType::PARAMETER) {
            return node->arg.param->value();
        }
        return node->arg.value;
    }
    return RPNConverter::getOperatorFromOperatorType(node->arg.operatorType).eval(evaluateNode(node->left),
                                                                                  evaluateNode(node->right));
}

void Expression::printExpressionTreeNode(ExpressionTreeNode *node, std::int32_t depth) {
    if (!node) {
        return;
    }
    auto &elt = node->elt;
    if (depth) {
        Spider::Logger::printInfo(LOG_GENERAL, "|");
        for (auto i = 0; i < depth; ++i) {
            Spider::Logger::printInfo(LOG_GENERAL, "-");
        }
        Spider::Logger::printInfo(LOG_GENERAL, ">");
    }
    if (elt.type == RPNElementType::OPERATOR) {
        Spider::Logger::printInfo(LOG_GENERAL, "%s\n",
                                  RPNConverter::getStringFromOperatorType(node->arg.operatorType).c_str());
    } else {
        if (elt.subtype == RPNElementSubType::PARAMETER) {
            Spider::Logger::printInfo(LOG_GENERAL, "%s\n", node->arg.param->name().c_str());
        } else {
            Spider::Logger::printInfo(LOG_GENERAL, "%lf\n", node->arg.value);
        }
    }
    printExpressionTreeNode(node->right, depth + 1);
    printExpressionTreeNode(node->left, depth + 1);
}
