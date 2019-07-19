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
#include <graphs/pisdf/PiSDFGraph.h>
#include <common/expression-parser/ParserFunctions.h>

/* === Static variable definition(s) === */

/**
 * @brief String containing all supported operators (should not be edited).
 */
static const std::string &operators() {
    static std::string operators{"+-*/%^()"};
    return operators;
}

/**
 * @brief Pre-declared @refitem RPNOperator (same order as @refitem RPNOperatorType enum class).
 */
static const RPNOperator &rpnOperators(std::uint32_t ix) {
    static RPNOperator rpnOperators[N_OPERATOR + N_FUNCTION]{
            {.type = RPNOperatorType::ADD, .precendence = 2, .isRighAssociative = false,
                    .label="+", .eval = Spider::add},          /*! ADD operator */
            {.type = RPNOperatorType::SUB, .precendence = 2, .isRighAssociative = false,
                    .label="-", .eval = Spider::sub},          /*! SUB operator */
            {.type = RPNOperatorType::MUL, .precendence = 3, .isRighAssociative = false,
                    .label="*", .eval = Spider::mul},          /*! MUL operator */
            {.type = RPNOperatorType::DIV, .precendence = 3, .isRighAssociative = false,
                    .label="/", .eval = Spider::div},          /*! DIV operator */
            {.type = RPNOperatorType::MOD, .precendence = 4, .isRighAssociative = false,
                    .label="%", .eval = Spider::mod},          /*! MOD operator */
            {.type = RPNOperatorType::POW, .precendence = 4, .isRighAssociative = true,
                    .label="^", .eval = Spider::pow},          /*! POW operator */
            {.type = RPNOperatorType::MAX, .precendence = 3, .isRighAssociative = false,
                    .label="max", .eval = Spider::max},        /*! MAX operator */
            {.type = RPNOperatorType::MIN, .precendence = 3, .isRighAssociative = false,
                    .label="min", .eval = Spider::min},        /*! MIN operator */
            {.type = RPNOperatorType::LEFT_PAR, .precendence = 2, .isRighAssociative = false,
                    .label="(", .eval = Spider::dummyEval},    /*! LEFT_PAR operator */
            {.type = RPNOperatorType::RIGHT_PAR, .precendence = 2, .isRighAssociative = false,
                    .label=")", .eval = Spider::dummyEval},    /*! RIGHT_PAR operator */
            {.type = RPNOperatorType::COS, .precendence = 5, .isRighAssociative = false,
                    .label="cos", .eval = Spider::cos},        /*! COS function */
            {.type = RPNOperatorType::SIN, .precendence = 5, .isRighAssociative = false,
                    .label="sin", .eval = Spider::sin},        /*! SIN function */
            {.type = RPNOperatorType::TAN, .precendence = 5, .isRighAssociative = false,
                    .label="tan", .eval = Spider::tan},        /*! TAN function */
            {.type = RPNOperatorType::EXP, .precendence = 5, .isRighAssociative = false,
                    .label="exp", .eval = Spider::exp},        /*! EXP function */
            {.type = RPNOperatorType::LOG, .precendence = 5, .isRighAssociative = false,
                    .label="log", .eval = Spider::log},        /*! LOG function */
            {.type = RPNOperatorType::LOG2, .precendence = 5, .isRighAssociative = false,
                    .label="log2", .eval = Spider::log2},      /*! LOG2 function */
            {.type = RPNOperatorType::CEIL, .precendence = 5, .isRighAssociative = false,
                    .label="ceil", .eval = Spider::ceil},      /*! CEIL function */
            {.type = RPNOperatorType::FLOOR, .precendence = 5, .isRighAssociative = false,
                    .label="floor", .eval = Spider::floor},    /*! FLOOR function */
            {.type = RPNOperatorType::SQRT, .precendence = 5, .isRighAssociative = false,
                    .label="sqrt", .eval = Spider::sqrt},    /*! SQRT function */
    };
    return rpnOperators[ix];
}

/* === Static Functions === */

static inline const RPNOperator &getOperator(RPNOperatorType type) {
    return rpnOperators(static_cast<std::uint32_t >(type));
}

static inline bool isOperator(const std::string &s) {
    bool found = false;
    for (auto i = 0; !found && i < (N_OPERATOR + N_FUNCTION); ++i) {
        found |= (rpnOperators(i).label == s);
    }
    return found;
}

static inline bool isFunction(RPNOperatorType type) {
    return static_cast<std::uint32_t >(type) >= FUNCTION_OPERATOR_OFFSET;
}

/**
 * @brief Retrieve the @refitem RPNOperatorType corresponding to a given string.
 * @param operatorString input string corresponding to the operator.
 * @return RPNOperatorType
 */
static inline RPNOperatorType getOperatorTypeFromString(const std::string &operatorString) {
    bool found = false;
    std::uint32_t i = 0;
    for (i = 0; !found && i < (N_OPERATOR + N_FUNCTION); ++i) {
        found |= (rpnOperators(i).label == operatorString);
    }
    if (!found) {
        throwSpiderException("Can not convert string [%s] to operator.", operatorString.c_str());
    }
    return rpnOperators(i - 1).type;
}

/**
 * @brief Get the string label of an operator from its type.
 * @param type
 * @return
 */
static inline std::string getStringFromOperatorType(RPNOperatorType type) {
    return getOperator(type).label;
}

static inline void setOperatorElement(RPNElement *elt, RPNOperatorType opType) {
    elt->type = RPNElementType::OPERATOR;
    elt->subType = isFunction(opType) ? RPNElementSubType::FUNCTION
                                      : RPNElementSubType::OPERATOR;
    elt->element.op = opType;
}

static inline void setOperandElement(RPNElement *elt, const std::string &token, PiSDFGraph *graph) {
    elt->type = RPNElementType::OPERAND;
    char *end;
    auto value = std::strtod(token.c_str(), &end);
    if (end == token.c_str() || (*end) != '\0') {
        auto *param = graph->findParam(token);
        if (!param) {
            throwSpiderException("Did not find parameter [%s] for expression parsing.", token.c_str());
        }
        if (param->isDynamic()) {
            elt->subType = RPNElementSubType::PARAMETER;
            elt->element.param = param;
        } else {
            elt->subType = RPNElementSubType::VALUE;
            elt->element.value = param->value();
        }
    } else {
        elt->subType = RPNElementSubType::VALUE;
        elt->element.value = value;
    }
}

static inline void setOperandElement(RPNElement *elt, double &value) {
    elt->type = RPNElementType::OPERAND;
    elt->subType = RPNElementSubType::VALUE;
    elt->element.value = value;
}

static inline void addToken(Spider::vector<RPNElement> &tokens, const std::string &token, PiSDFGraph *graph) {
    if (token.empty()) {
        return;
    }
    if (isOperator(token)) {
        tokens.push_back(RPNElement());
        /* == Function case == */
        setOperatorElement(&tokens.back(), getOperatorTypeFromString(token));
    } else {
        auto pos = token.find_first_of(',', 0);
        if (pos != std::string::npos) {
            /* == Double operand case == */
            addToken(tokens, token.substr(0, pos), graph);
            pos += 1;
            addToken(tokens, token.substr(pos, (token.size() - pos)), graph);
        } else {
            tokens.push_back(RPNElement());
            /* == Operand case == */
            setOperandElement(&tokens.back(), token, graph);
        }
    }
}

static void retrieveExprTokens(std::string &inFixExpr, Spider::vector<RPNElement> &tokens, PiSDFGraph *graph) {
    auto pos = inFixExpr.find_first_of(operators(), 0);
    std::uint32_t lastPos = 0;
    while (pos != std::string::npos) {
        /* == Operand or Function token (can be empty) == */
        auto token = inFixExpr.substr(lastPos, pos - lastPos);
        addToken(tokens, token, graph);

        /* == Operator == */
        token = inFixExpr.substr(pos, 1);
        tokens.push_back(RPNElement());
        setOperatorElement(&tokens.back(), getOperatorTypeFromString(token));
        pos += 1;
        lastPos = pos;
        pos = inFixExpr.find_first_of(operators(), pos);
    }

    /* == Potential left over (if expression ends with an operand) == */
    if (lastPos != inFixExpr.size()) {
        std::string token = inFixExpr.substr(lastPos, pos - lastPos);
        addToken(tokens, token, graph);
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
        if (elt.subType == RPNElementSubType::PARAMETER) {
            fprintf(stderr, "%s\n", elt.element.param->name().c_str());
        } else {
            fprintf(stderr, "%lf\n", elt.element.value);
        }
    }
    printExpressionTreeNode(node->right, depth + 1);
    printExpressionTreeNode(node->left, depth + 1);
}

/**
 * @brief In place replace of all occurrences of substring in a string.
 * @param s     String on which we are working.
 * @param pattern  Substring to find.
 * @param replace    Substring to replace found matches.
 * @return Modified string (same as s).
 */
static std::string &stringReplace(std::string &s, const std::string &pattern, const std::string &replace) {
    if (!pattern.empty()) {
        for (size_t pos = 0; (pos = s.find(pattern, pos)) != std::string::npos; pos += replace.size()) {
            s.replace(pos, pattern.size(), replace);
        }
    }
    return s;
}

/* === Method(s) implementation === */

RPNConverter::RPNConverter(std::string inFixExpr, PiSDFGraph *graph) : infixExprString_{std::move(inFixExpr)},
                                                                          graph_{graph} {
    if (missMatchParenthesis()) {
        throwSpiderException("Expression with miss matched parenthesis: %s", infixExprString_.c_str());
    }
    if (infixExprString_.empty()) {
        throwSpiderException("Empty expression !");
    }

    /* == Format properly the expression == */
    cleanInfixExpression();

    /* == Check for incoherence == */
    checkInfixExpression();

    /* == Build the postfix expression == */
    buildPostFix();

    /* == Build and reduce the expression tree for fast resolving == */
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

double RPNConverter::evaluate() const {
    return evaluateNode(expressionTree_);
}

const std::string &RPNConverter::toString() {
    if (postfixExprString_.empty() || !static_) {
        postfixExprString_ = "";
        for (auto &t : postfixExprStack_) {
            if (t.type == RPNElementType::OPERATOR) {
                postfixExprString_ += getStringFromOperatorType(t.element.op) + " ";
            } else if (t.subType == RPNElementSubType::PARAMETER) {
                postfixExprString_ += t.element.param->name() + " ";
            } else {
                postfixExprString_ += std::to_string(t.element.value) + std::string(" ");
            }
        }
    }
    return postfixExprString_;
}

/* === Private method(s) === */

void RPNConverter::cleanInfixExpression() {
    /* == Clean the inFix expression by removing all white spaces == */
    infixExprString_.erase(std::remove(infixExprString_.begin(), infixExprString_.end(), ' '), infixExprString_.end());

    /* == Convert the infix to lowercase == */
    std::transform(infixExprString_.begin(), infixExprString_.end(), infixExprString_.begin(), ::tolower);

    /* == Clean the inFix expression by adding '*' for #valueY -> #value * Y  == */
    std::string tmp{std::move(infixExprString_)};
    infixExprString_.reserve(tmp.size() * 2); /*= Worst case is actually 1.5 = */
    std::uint32_t i = 0;
    bool ignore = false;
    for (const auto &c:tmp) {
        infixExprString_ += c;
        auto next = tmp[++i];
        if (!ignore && ((std::isdigit(c) &&
                         (std::isalpha(next) || next == '(')) ||
                        (c == ')' &&
                         (next == '(' || std::isdigit(next) || std::isalpha(next))))) {
            infixExprString_ += '*';
        }
        ignore = std::isalpha(c) && std::isdigit(next);
    }

    /* == Make sure that double operand functions have parenthesis == */
    auto pos = infixExprString_.find_first_of(',', 0);
    auto lastPos = 0;
    while (pos != std::string::npos) {
        auto lowerPos = infixExprString_.find_last_of('(', pos - lastPos);
        auto substr = infixExprString_.substr(lowerPos, (pos - lowerPos));
        stringReplace(infixExprString_, substr, "(" + substr + ")");
        pos += 3;
        auto upperPos = infixExprString_.find_first_of(')', pos);
        substr = infixExprString_.substr(pos, (upperPos - pos));
        stringReplace(infixExprString_, substr, "(" + substr + ")");
        lastPos = pos;
        pos = infixExprString_.find_first_of(',', pos);
    }

    /* == Clean the inFix expression by replacing every occurrence of PI to its value == */
    stringReplace(infixExprString_, "pi", "3.1415926535");
}

void RPNConverter::checkInfixExpression() const {
    std::string restrictedOperators{"*/+-%^"};
    std::uint32_t i = 0;
    for (const auto &c: infixExprString_) {
        i += 1;
        if (restrictedOperators.find(c) != std::string::npos) {
            auto next = infixExprString_[i];
            if (restrictedOperators.find(next) != std::string::npos) {
                throwSpiderException("Expression ill formed. Two operators without operands between: %c -- %c", c,
                                     next);
            } else if (&c == &infixExprString_.front() ||
                       &c == &infixExprString_.back()) {
                throwSpiderException("Expression ill formed. Operator [%c] expecting two operands.", c);
            }
        }
    }
}

void RPNConverter::buildPostFix() {
    /* == Retrieve tokens == */
    Spider::vector<RPNElement> tokens;
    retrieveExprTokens(infixExprString_, tokens, graph_);

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
                        postfixExprStack_.push_back(elt);
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
                        postfixExprStack_.push_back(elt);
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
            postfixExprStack_.push_back(t);
        }
    }

    while (!operatorStack.empty()) {
        auto frontOPType = operatorStack.front();
        RPNElement elt;
        setOperatorElement(&elt, frontOPType);
        postfixExprStack_.push_back(elt);
        operatorStack.pop_front();
    }
}

void RPNConverter::buildExpressionTree() {
    expressionTree_ = Spider::allocate<ExpressionTreeNode>(StackID::GENERAL, postfixExprStack_.size());
    std::uint16_t nodeIx = 0;
    Spider::construct(expressionTree_, nodeIx++, nullptr);
    auto *node = expressionTree_;
    for (auto elt = postfixExprStack_.rbegin(); elt != postfixExprStack_.rend(); ++elt) {
        node = insertExpressionTreeNode(node, &(*elt), nodeIx);
    }
}

ExpressionTreeNode *RPNConverter::insertExpressionTreeNode(ExpressionTreeNode *node,
                                                           RPNElement *elt,
                                                           std::uint16_t &nodeIx) {
    node->elt = *elt;
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
                auto valLeft = left->elt.element.value;
                auto valRight = right ? right->elt.element.value : 0.;
                auto evalValue = getOperator(node->elt.element.op).eval(valLeft,
                                                                        valRight);
                setOperandElement(&node->elt, evalValue);
                node->left = nullptr;
                node->right = nullptr;
            }
            node = node->parent;
        }
    }
    return node;
}

double RPNConverter::evaluateNode(ExpressionTreeNode *node) const {
    auto &elt = node->elt;
    if (elt.type == RPNElementType::OPERAND) {
        if (elt.subType == RPNElementSubType::PARAMETER) {
            return elt.element.param->value();
        }
        return elt.element.value;
    }
    auto valLeft = evaluateNode(node->left);
    auto valRight = node->right ? evaluateNode(node->right) : 0.;
    return getOperator(elt.element.op).eval(valLeft, valRight);
}