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
#ifndef SPIDER2_EXPRESSION_H
#define SPIDER2_EXPRESSION_H

/* === Includes === */

#include <cstdint>
#include <string>
#include <common/containers/Array.h>
#include "RPNConverter.h"

/* === Forward declaration(s) === */

class PiSDFGraph;

/* === Structure definition(s) === */

struct ExpressionTreeNode {
    ExpressionTreeNode *left = nullptr;
    ExpressionTreeNode *right = nullptr;
    ExpressionTreeNode *parent = nullptr;
    std::uint16_t ix = 0;
    RPNElement elt;

    ExpressionTreeNode(std::uint16_t ix, ExpressionTreeNode *parent) : parent{parent}, ix{ix} {
        right = nullptr;
        left = nullptr;
    }
};

/* === Class definition === */

class Expression {
public:

    Expression(const PiSDFGraph *graph, std::string expression);

    explicit Expression(std::int64_t value);

    ~Expression();

    /* === Methods === */

    /**
     * @brief Evaluate the expression and return the value and cast result in int64_.
     * @return Evaluated value of the expression.
     */
    inline std::int64_t evaluate() const;

    /**
     * @brief Evaluate the expression and return the value.
     * @return Evaluated value of the expression.
     */
    inline double evaluateDBL() const;

    /**
     * @brief Print the ExpressionTree (debug only).
     */
    void printExpressionTree();

    /* === Getters === */

    /**
     * @brief Get the last evaluated value (faster than evaluated on static expressions)
     * @return last evaluated value (default value, i.e no evaluation done, is 0)
     */
    inline std::int64_t value() const;

    /**
     * @brief Get the infix expression string
     * @return Clean infix expression string
     */
    inline const std::string &toString() const;

    /**
     * @brief Get the expression postfix string.
     * @return postfix expression string.
     */
    inline const std::string &postfixString() const;

    /**
     * @brief Get the static property of the expression.
     * @return true if the expression is static, false else.
     */
    inline bool isStatic() const;

private:
    RPNConverter rpnConverter_;
    ExpressionTreeNode *expressionTree_ = nullptr;
    double value_ = 0;
    bool static_ = true;

    /* === Private method(s) === */

    /**
     * @brief Build and reduce the expression tree parser.
     * @param expressionStack Stack of the postfix expression elements.
     */
    void buildExpressionTree(const Spider::deque<RPNElement> *expressionStack);

    /**
     * @brief
     * @param node
     * @param elt
     * @param nodeIx
     * @return
     */
    ExpressionTreeNode *insertExpressionTreeNode(ExpressionTreeNode *node,
                                                 const RPNElement &elt,
                                                 std::uint16_t &nodeIx);

    /**
     * @brief Evaluate the value of a node in the ExpressionTree.
     * @param node  Node to evaluate.
     * @return value of the evaluated node.
     * @remark This is a recursive method.
     */
    double evaluateNode(ExpressionTreeNode *node) const;

    void printExpressionTreeNode(ExpressionTreeNode *node, std::int32_t depth);
};

/* === Inline methods === */

std::int64_t Expression::value() const {
    return static_cast<std::int64_t>(value_);
}

const std::string &Expression::toString() const {
    return rpnConverter_.infixString();
}

const std::string &Expression::postfixString() const {
    return rpnConverter_.postfixString();
}

std::int64_t Expression::evaluate() const {
    if (static_) {
        return static_cast<std::int64_t>(value_);
    }
    return static_cast<std::int64_t>(evaluateNode(expressionTree_));
}

double Expression::evaluateDBL() const {
    if (static_) {
        return value_;
    }
    return evaluateNode(expressionTree_);
}

bool Expression::isStatic() const {
    return static_;
}

#endif //SPIDER2_EXPRESSION_H
