/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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

#include "Expression.h"
#include "RPNConverter.h"
#include <graphs/pisdf/PiSDFParam.h>

/* === Static method(s) === */

/* === Methods implementation === */

Expression::Expression(const PiSDFGraph *graph, std::string expression) : rpnConverter_{graph, std::move(expression)} {
    const auto *expressionStack = &rpnConverter_.postfixStack();

    /* == Check if expression is static == */
    for (const auto &elt : *expressionStack) {
        if (elt.subType == RPNElementSubType::PARAMETER) {
            static_ = false;
            break;
        }
    }

    /* == Build and reduce the expression tree for fast resolving == */
    buildExpressionTree(expressionStack);

    /* == If static, evaluate and delete the expression == */
    if (static_) {
        valueDBL_ = evaluateNode(expressionTree_);
        valueInt64_ = static_cast<std::int64_t >(valueDBL_);
        Spider::deallocate(expressionTree_);
        expressionTree_ = nullptr;
    }
}

Expression::Expression(std::int64_t value) {
    static_ = true;
    valueDBL_ = value;
    valueInt64_ = value;
}

Expression::~Expression() {
    if (expressionTree_) {
        Spider::deallocate(expressionTree_);
        expressionTree_ = nullptr;
    }
}

void Expression::printExpressionTree() {
    if (expressionTree_) {
        printExpressionTreeNode(expressionTree_, 0);
    }
}

/* === Private method(s) === */

void Expression::buildExpressionTree(const Spider::deque<RPNElement> *expressionStack) {
    expressionTree_ = Spider::allocate<ExpressionTreeNode>(StackID::GENERAL, expressionStack->size());
    Spider::construct(expressionTree_, 0, nullptr);
    std::uint16_t nodeIx = 1;
    auto *node = expressionTree_;
    for (auto elt = expressionStack->rbegin(); elt != expressionStack->rend(); ++elt) {
        node = insertExpressionTreeNode(node, *elt, nodeIx);
    }
}

ExpressionTreeNode *Expression::insertExpressionTreeNode(ExpressionTreeNode *node,
                                                         const RPNElement &elt,
                                                         std::uint16_t &nodeIx) {
    node->elt = elt;
    while (node) {
        auto *tmpElt = &node->elt;
        if (tmpElt->subType == RPNElementSubType::OPERATOR && !node->right) {
            node->right = &expressionTree_[nodeIx];
            Spider::construct(node->right, nodeIx++, node);
            return node->right;
        } else if (!node->left && (node->right || tmpElt->subType == RPNElementSubType::FUNCTION)) {
            node->left = &expressionTree_[nodeIx];
            Spider::construct(node->left, nodeIx++, node);
            return node->left;
        } else {
            auto *left = node->left;
            auto *right = node->right;
            if (node->elt.type == RPNElementType::OPERATOR && left && left->elt.subType == RPNElementSubType::VALUE) {
                auto valLeft = left->elt.value;
                if (!right || right->elt.subType == RPNElementSubType::VALUE) {
                    auto valRight = right ? right->elt.value : 0.;
                    node->elt.type = RPNElementType::OPERAND;
                    node->elt.subType = RPNElementSubType::VALUE;
                    node->elt.value = RPNConverter::getOperator(node->elt.op).eval(valLeft, valRight);
                    Spider::destroy(node->left);
                    node->left = nullptr;
                    Spider::destroy(node->right);
                    node->right = nullptr;
                }
            }
            node = node->parent;
        }
    }
    return node;
}

double Expression::evaluateNode(ExpressionTreeNode *node) const {
    auto &elt = node->elt;
    if (elt.type == RPNElementType::OPERAND) {
        if (elt.subType == RPNElementSubType::PARAMETER) {
            return elt.param->value();
        }
        return elt.value;
    }
    auto valLeft = evaluateNode(node->left);
    auto valRight = node->right ? evaluateNode(node->right) : 0.;
    return RPNConverter::getOperator(elt.op).eval(valLeft, valRight);
}

void Expression::printExpressionTreeNode(ExpressionTreeNode *node, std::int32_t depth) {
    if (!node) {
        return;
    }
    auto &elt = node->elt;
    if (depth) {
        fprintf(stderr, "|");
        for (auto i = 0; i < depth; ++i) {
            fprintf(stderr, "-");
        }
        fprintf(stderr, "> ");
    }
    if (elt.type == RPNElementType::OPERATOR) {
        fprintf(stderr, "%s\n", RPNConverter::getStringFromOperatorType(elt.op).c_str());
    } else {
        if (elt.subType == RPNElementSubType::PARAMETER) {
            fprintf(stderr, "%s\n", elt.param->name().c_str());
        } else {
            fprintf(stderr, "%lf\n", elt.value);
        }
    }
    printExpressionTreeNode(node->right, depth + 1);
    printExpressionTreeNode(node->left, depth + 1);
}
