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

#include <algorithm>
#include <iostream>
#include "RPNConverter.h"
#include <common/containers/StlContainers.h>
#include <common/SpiderException.h>
#include <graphs/pisdf/PiSDFParam.h>
#include <common/expression-parser/ParserFunctions.h>

/* === Static variable definition(s) === */

using operator2ArgsFct = double (*)(double &, double &);

using operator1ArgFct = double (*)(double &);

static operator2ArgsFct operatorsFctArray[N_OPERATOR - 2] = {
        add,
        sub,
        mul,
        div,
        pow,
        mod,
};

static operator1ArgFct functionsFctArray[N_FUNCTION] = {
        cos,
        sin,
        exp,
        tan,
        log,
        log2,
        ceil,
        floor,
};

/**
 * @brief String containing all supported operators.
 */
static std::string operators{"+-*/%^()"};

/**
 * @brief String containing all supported functions.
 */
static std::string functions{",0cos,1sin,2exp,3tan,4log,5log2,6ceil,7floor,"};

/**
 * @brief Pre-declared @refitem RPNOperator (same order as @refitem RPNOperatorType enum class).
 */
static RPNOperator rpnOperators[N_OPERATOR + N_FUNCTION]{
        {.type = RPNOperatorType::ADD, .precendence = 2, .isRighAssociative = false},   /*! ADD operator */
        {.type = RPNOperatorType::SUB, .precendence = 2, .isRighAssociative = false},   /*! ADD operator */
        {.type = RPNOperatorType::MUL, .precendence = 3, .isRighAssociative = false},   /*! MUL operator */
        {.type = RPNOperatorType::DIV, .precendence = 3, .isRighAssociative = false},   /*! DIV operator */
        {.type = RPNOperatorType::MOD, .precendence = 3, .isRighAssociative = false},   /*! MOD operator */
        {.type = RPNOperatorType::POW, .precendence = 4, .isRighAssociative = true},    /*! POW operator */
        {.type = RPNOperatorType::CEIL, .precendence = 5, .isRighAssociative = false},  /*! CEIL operator */
        {.type = RPNOperatorType::FLOOR, .precendence = 5, .isRighAssociative = false}, /*! FLOOR operator */
        {.type = RPNOperatorType::LOG, .precendence = 5, .isRighAssociative = false},   /*! LOG operator */
        {.type = RPNOperatorType::LOG2, .precendence = 5, .isRighAssociative = false},  /*! LOG2 operator */
        {.type = RPNOperatorType::COS, .precendence = 5, .isRighAssociative = false},   /*! COS operator */
        {.type = RPNOperatorType::SIN, .precendence = 5, .isRighAssociative = false},   /*! SIN operator */
        {.type = RPNOperatorType::TAN, .precendence = 5, .isRighAssociative = false},   /*! TAN operator */
        {.type = RPNOperatorType::EXP, .precendence = 5, .isRighAssociative = false},   /*! EXP operator */
};

/**
 * @brief Array of @refitem RPNOperatorType operators (same order as @refitem operators) for the translation from string.
 */
static RPNOperatorType rpnOperatorsType[N_OPERATOR] = {
        RPNOperatorType::ADD, RPNOperatorType::SUB,
        RPNOperatorType::MUL, RPNOperatorType::DIV,
        RPNOperatorType::MOD, RPNOperatorType::POW,
        RPNOperatorType::LEFT_PAR, RPNOperatorType::RIGHT_PAR,
};

/**
 * @brief Array of @refitem RPNOperatorType functions (same order as @refitem functions) for the translation from string.
 */
static RPNOperatorType rpnFunctionsType[N_FUNCTION] = {
        RPNOperatorType::COS, RPNOperatorType::SIN,
        RPNOperatorType::EXP, RPNOperatorType::TAN,
        RPNOperatorType::LOG, RPNOperatorType::LOG2,
        RPNOperatorType::CEIL, RPNOperatorType::FLOOR,
};

/* === Static Functions === */

static RPNOperator &getOperator(RPNOperatorType type) {
    return rpnOperators[static_cast<std::uint32_t >(type)];
}

static bool isFunction(const std::string &s) {
    auto pos = functions.find(s);
    return pos != std::string::npos && functions[pos - 2] == ',' && functions[pos + s.size()] == ',';
}

static bool isFunction(RPNOperatorType type) {
    return static_cast<std::int32_t >(type) > static_cast<std::int32_t >(RPNOperatorType::MOD) &&
           type != RPNOperatorType::LEFT_PAR &&
           type != RPNOperatorType::RIGHT_PAR;
}

static bool isOperator(const std::string &s) {
    return operators.find(s) != std::string::npos;
}

/**
 * @brief Retrieve the @refitem RPNOperatorType corresponding to the input string.
 * @param operatorString input string corresponding to the operator.
 * @return RPNOperatorType
 */
static RPNOperatorType getOperatorTypeFromString(const std::string &operatorString) {
    auto op = operators.find(operatorString);
    if (op == std::string::npos) {
        op = functions.find(operatorString);
        op = functions[op - 1] - '0';
        return rpnFunctionsType[op];
    }
    return rpnOperatorsType[op];
}

static std::string getStringFromOperatorType(RPNOperatorType type) {
    static constexpr const char *stringOperators[N_OPERATOR + N_FUNCTION] = {
            "+", "-", "*", "/", "^", "%", "cos", "sin", "tan", "exp",
            "log", "log2", "ceil", "floor", "(", ")",
    };
    return stringOperators[static_cast<std::uint32_t>(type)];
}

static inline void setOperatorElement(RPNElement *elt, RPNOperatorType opType) {
    elt->type = RPNElementType::OPERATOR;
    elt->subType = isFunction(opType) ? RPNElementSubType::FUNCTION
                                      : RPNElementSubType::OPERATOR;
    elt->element.op = opType;
}

static inline void setOperandElement(RPNElement *elt, const std::string &token, PiSDFGraph *graph = nullptr) {
    elt->type = RPNElementType::OPERAND;
    char *end;
    auto value = std::strtod(token.c_str(), &end);
    if (end == token.c_str() || (*end) != '\0') {
        elt->subType = RPNElementSubType::PARAMETER;
    } else {
        elt->subType = RPNElementSubType::VALUE;
        elt->element.value = value;
    }
}

static void retrieveExprTokens(std::string &inFixExpr, Spider::vector<RPNElement> &tokens) {
    std::uint32_t nTokens = 1;

    /* == Compute the number of tokens in the expression == */
    bool prevOP = false;
    bool first = true;
    for (const auto &c: inFixExpr) {
        if (operators.find(c) != std::string::npos) {
            bool last = &c == &inFixExpr.back();
            nTokens += (2 - (prevOP || first || last) - last);
            prevOP = true;
        } else {
            prevOP = false;
        }
        first = false;
    }
    std::uint32_t i = 0;
    std::uint32_t last = 0;
    tokens.reserve(nTokens);
    for (std::uint32_t j = 0; j < nTokens; ++j) {
        tokens.push_back(RPNElement());
    }

    /* == Extract the tokens from the infix expression == */
    std::uint32_t nDone = 0;
    for (const auto &c: inFixExpr) {
        if (operators.find(c) != std::string::npos) {
            if ((i - last) > 0) {
                auto token = inFixExpr.substr(last, i - last);
                if (isFunction(token) || isOperator(token)) {
                    /* == Operator and Function case == */
                    setOperatorElement(&tokens[nDone++], getOperatorTypeFromString(token));
                } else {
                    /* == Operand case == */
                    setOperandElement(&tokens[nDone++], token);
                }
            }
            std::string tokenOp{c};
            setOperatorElement(&tokens[nDone++], getOperatorTypeFromString(tokenOp));
            last = i + 1;
        }
        i++;
    }
    if ((i - last) > 0) {
        auto token = inFixExpr.substr(last, i - last);
        if (isFunction(token) || isOperator(token)) {
            /* == Operator and Function case == */
            setOperatorElement(&tokens[nDone], getOperatorTypeFromString(token));
        } else {
            /* == Operand case == */
            setOperandElement(&tokens[nDone], token);
        }
    }
}

static void printExpressionTreeNode(ExpressionTreeNode *node, std::int32_t depth) {
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
        fprintf(stderr, "%s\n", getStringFromOperatorType(elt.element.op).c_str());
    } else {
        fprintf(stderr, "%lf\n", elt.element.value);
    }
    printExpressionTreeNode(node->right, depth + 1);
    printExpressionTreeNode(node->left, depth + 1);
}

static double evaluateNode(ExpressionTreeNode *node) {
    if (node->elt.type == RPNElementType::OPERAND) {
        return node->elt.element.value;
    } else if (node->elt.subType == RPNElementSubType::FUNCTION) {
        auto val = evaluateNode(node->left);
        return functionsFctArray[static_cast<std::uint32_t >(node->elt.element.op) - FUNCTION_OPERATOR_OFFSET](val);
    } else {
        auto valLeft = evaluateNode(node->left);
        auto valRight = evaluateNode(node->right);
        return operatorsFctArray[static_cast<std::uint32_t >(node->elt.element.op)](valLeft, valRight);
    }
}

/* === Methods implementation === */

RPNConverter::RPNConverter(std::string inFixExpr) : infixExpr_{std::move(inFixExpr)} {
    if (missMatchParenthesis()) {
        throwSpiderException("Expression with miss matched parenthesis: %s", infixExpr_.c_str());
    }

    /* == Format properly the expression == */
    cleanInfixExpression();

    std::cerr << infixExpr_ << std::endl;

    /* == Check for incoherence == */
    checkInfixExpression();

    /* == Build the postfix expression == */
    buildPostFix();

    /* == Build tree for fast resolving == */
    buildExpressionTree();
}

RPNConverter::~RPNConverter() {
    if (expressionTree_) {
        Spider::deallocate(expressionTree_);
        expressionTree_ = nullptr;
    }
}



void RPNConverter::printExpressionTree() {
    if (expressionTree_) {
        printExpressionTreeNode(expressionTree_, 0);
    }
}

std::string RPNConverter::toString() {
    if (postfixExprString_.empty()) {
        for (auto &t:postfixExpr_) {
            if (t.type == RPNElementType::OPERATOR) {
                postfixExprString_ += getStringFromOperatorType(t.element.op) + " ";
            } else {
                postfixExprString_ += std::to_string(t.element.value) + " ";
            }
        }
    }
    return postfixExprString_;
}



double RPNConverter::evaluate() const {
    return evaluateNode(expressionTree_);
}

std::string &RPNConverter::replace(std::string &s, const std::string &pattern, const std::string &replace) {
    if (!pattern.empty()) {
        for (size_t pos = 0; (pos = s.find(pattern, pos)) != std::string::npos; pos += replace.size()) {
            s.replace(pos, pattern.size(), replace);
        }
    }
    return s;
}

void RPNConverter::cleanInfixExpression() {
    /* == Clean the inFix expression by removing all white spaces == */
    infixExpr_.erase(std::remove(infixExpr_.begin(), infixExpr_.end(), ' '), infixExpr_.end());

    /* == Convert the infix to lowercase == */
    std::transform(infixExpr_.begin(), infixExpr_.end(), infixExpr_.begin(), ::tolower);

    /* == Clean the inFix expression by adding '*' for #valueY -> #value * Y  == */
    std::string tmp{std::move(infixExpr_)};
    infixExpr_.reserve(tmp.size() * 2); /*= Worst case is actually 1.5 = */
    std::uint32_t i = 0;
    for (const auto &c:tmp) {
        infixExpr_ += c;
        auto next = tmp[++i];
        if ((std::isdigit(c) && (std::isalpha(next) || next == '(')) ||
            (c == ')' && (next == '(' || std::isdigit(next) || std::isalpha(next)))) {
            infixExpr_ += '*';
        }
    }

    /* == Clean the inFix expression by replacing every occurrence of PI to its value == */
    replace(infixExpr_, std::string("pi"), std::string("3.1415926535"));
}

void RPNConverter::checkInfixExpression() const {
    std::string restrictedOperators{"*/+-%^"};
    std::uint32_t i = 0;
    for (const auto &c: infixExpr_) {
        i += 1;
        if (restrictedOperators.find(c) != std::string::npos) {
            auto next = infixExpr_[i];
            if (restrictedOperators.find(next) != std::string::npos) {
                throwSpiderException("Expression ill formed. Two operators without operands between: %c -- %c", c,
                                     next);
            } else if (&c == &infixExpr_.front() ||
                       &c == &infixExpr_.back()) {
                throwSpiderException("Expression ill formed. Operator [%c] expecting two operands.", c);
            }
        }
    }
}

void RPNConverter::buildPostFix() {

    /* == Retrieve tokens == */
    Spider::vector<RPNElement> tokens;
    retrieveExprTokens(infixExpr_, tokens);

    /* == Build the postfix expression == */
    Spider::deque<RPNOperatorType> operatorStack;
    for (const auto &t : tokens) {
        if (t.type == RPNElementType::OPERATOR) {

            /* == Handle operator == */
            auto opType = t.element.op;
            if (isFunction(opType)) {
                operatorStack.push_front(opType);
            } else {

                /* == Handle right parenthesis case == */
                if (opType == RPNOperatorType::RIGHT_PAR) {
                    auto frontOPType = operatorStack.front();

                    /* == This should not fail because miss match parenthesis is checked on constructor == */
                    while (frontOPType != RPNOperatorType::LEFT_PAR) {

                        /* == Put operator to the output == */
                        RPNElement elt;
                        setOperatorElement(&elt, frontOPType);
                        postfixExpr_.push_back(elt);
                        operatorStack.pop_front();
                        if (operatorStack.empty()) {
                            break;
                        }

                        /* == Pop operator from the stack == */
                        frontOPType = operatorStack.front();
                    }

                    /* == Pop left parenthesis == */
                    operatorStack.pop_front();
                    continue;
                }

                /* == Handle left parenthesis case == */
                if (opType == RPNOperatorType::LEFT_PAR) {
                    operatorStack.push_front(opType);
                    continue;
                }

                /* == Handle general case == */
                if (!operatorStack.empty()) {
                    auto op = getOperator(opType);
                    auto frontOPType = operatorStack.front();
                    auto frontOP = getOperator(frontOPType);
                    while (frontOPType != RPNOperatorType::LEFT_PAR &&
                           (op.precendence < frontOP.precendence ||
                            (op.precendence == frontOP.precendence && !frontOP.isRighAssociative))) {

                        /* == Put operator to the output == */
                        RPNElement elt;
                        setOperatorElement(&elt, frontOPType);
                        postfixExpr_.push_back(elt);
                        operatorStack.pop_front();
                        if (operatorStack.empty()) {
                            break;
                        }

                        /* == Pop operator from the stack == */
                        frontOPType = operatorStack.front();
                        frontOP = getOperator(frontOPType);
                    }
                }

                /* == Push current operator to the stack == */
                operatorStack.push_front(opType);
            }
        } else {
            /* == Handle operand == */
            postfixExpr_.push_back(t);
        }
    }

    while (!operatorStack.empty()) {
        auto frontOPType = operatorStack.front();
        RPNElement elt;
        setOperatorElement(&elt, frontOPType);
        postfixExpr_.push_back(elt);
        operatorStack.pop_front();
    }
}

void RPNConverter::buildExpressionTree() {
    auto *poolNodes = Spider::allocate<ExpressionTreeNode>(StackID::GENERAL, postfixExpr_.size());
    expressionTree_ = &poolNodes[0];
    Spider::construct(expressionTree_);
    ExpressionTreeNode *parent = nullptr;
    auto *node = expressionTree_;
    std::int32_t nNode = 1;
    for (auto elt = postfixExpr_.rbegin(); elt != postfixExpr_.rend(); ++elt) {
        node->elt = *elt;
        node->parent = parent;
        if (elt->type == RPNElementType::OPERATOR) {
            if (elt->subType == RPNElementSubType::FUNCTION) {
                node->left = &poolNodes[nNode++];
                Spider::construct(node->left);
            } else {
                node->right = &poolNodes[nNode++];
                Spider::construct(node->right);
                node->left = &poolNodes[nNode++];
                Spider::construct(node->left);
            }
        }
        parent = node;
        if (node->right) {
            node = node->right;
        } else if (node->left) {
            node = node->left;
        } else {
            /* == Go up the tree to find first left node == */
            auto *tmpParent = node->parent;
            while (tmpParent) {
                auto *tmpNode = tmpParent;
                if (!tmpNode->left->parent) {
                    parent = tmpNode;
                    node = tmpNode->left;
                    break;
                }
                tmpParent = tmpNode->parent;
            }
        }
    }
}

void RPNConverter::reduceExpressionTree() {

}

