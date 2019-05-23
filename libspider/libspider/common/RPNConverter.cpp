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

#include <sstream>
#include <iostream>
#include <map>
#include "RPNConverter.h"
#include <common/containers/StlContainers.h>
#include <common/SpiderException.h>
#include <graphs/pisdf/PiSDFParam.h>

/* === Static variable definition(s) === */

static RPNOperator rpnOperators[N_OPERATOR]{
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

static std::map<std::string, RPNOperatorType> &getStringToOperatorMap() {
    static std::map<std::string, RPNOperatorType> stringToOperatorMap = {
            {"+", RPNOperatorType::ADD},
            {"-", RPNOperatorType::SUB},
            {"*", RPNOperatorType::MUL},
            {"/", RPNOperatorType::DIV},
            {"%", RPNOperatorType::MOD},
            {"^", RPNOperatorType::POW},
            {"(", RPNOperatorType::LEFT_PAR},
            {")", RPNOperatorType::RIGHT_PAR},
    };
    return stringToOperatorMap;
}

static std::map<std::string, RPNOperatorType> &getStringToFunctionMap() {
    static std::map<std::string, RPNOperatorType> stringToFunctionMap = {
            {"cos",   RPNOperatorType::COS},
            {"sin",   RPNOperatorType::SIN},
            {"log",   RPNOperatorType::LOG},
            {"log2",  RPNOperatorType::LOG2},
            {"tan",   RPNOperatorType::TAN},
            {"exp",   RPNOperatorType::EXP},
            {"ceil",  RPNOperatorType::CEIL},
            {"floor", RPNOperatorType::FLOOR},
    };
    return stringToFunctionMap;
}

static std::map<RPNOperatorType, std::string> &getOperatorToStringMap() {
    static std::map<RPNOperatorType, std::string> operatorToStringMap = {
            {RPNOperatorType::ADD,       "+"},
            {RPNOperatorType::SUB,       "-"},
            {RPNOperatorType::MUL,       "*"},
            {RPNOperatorType::DIV,       "/"},
            {RPNOperatorType::MOD,       "%"},
            {RPNOperatorType::POW,       "^"},
            {RPNOperatorType::LEFT_PAR,  "("},
            {RPNOperatorType::RIGHT_PAR, ")"},
            {RPNOperatorType::COS,       "cos"},
            {RPNOperatorType::SIN,       "sin"},
            {RPNOperatorType::LOG,       "log"},
            {RPNOperatorType::LOG2,      "log2"},
            {RPNOperatorType::TAN,       "tan"},
            {RPNOperatorType::EXP,       "exp"},
            {RPNOperatorType::CEIL,      "ceil"},
            {RPNOperatorType::FLOOR,     "floor"},
    };
    return operatorToStringMap;
}

static RPNOperator &getOperator(RPNOperatorType type) {
    return rpnOperators[static_cast<std::uint32_t >(type)];
}


/* === Methods implementation === */

RPNConverter::RPNConverter(std::string inFixExpr) : infixExpr_{std::move(inFixExpr)},
                                                    postfixExpr_(StackID::EXPR_PARSER) {
    if (missMatchParenthesis()) {
        throwSpiderException("Expression with miss matched parenthesis: %s", infixExpr_.c_str());
    }
    fprintf(stderr, "INFO: original expression:  %s\n", infixExpr_.c_str());

    /* == Format properly the expression == */
    cleanInfixExpression();
    fprintf(stderr, "INFO: formatted expression: %s\n", infixExpr_.c_str());

    /* == Build the postfix expression == */
    buildPostFix();
}

RPNConverter::~RPNConverter() {
    if (postfixExpr_.size()) {
        postfixExpr_.setOnValue(postfixExpr_.head());
        auto *element = postfixExpr_.current();
        do {
            auto *&value = element->value;
            Spider::destroy(value);
            value = nullptr;
            element = postfixExpr_.next();
        } while (postfixExpr_.current() != postfixExpr_.head());
    }
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

    /* == Clean the inFix expression by adding '*' for #valueY -> #value * Y  == */
    std::string tmp{std::move(infixExpr_)};
    infixExpr_.reserve(tmp.size() * 2); /*= Worst case is actually 1.5 = */
    std::uint32_t i = 0;
    for (const auto &c:tmp) {
        infixExpr_ += c;
        auto next = tmp[++i];
        if ((std::isdigit(c) && std::isalpha(next)) ||
            (c == ')' && next == '(')) {
            infixExpr_ += '*';
        }
    }

    /* == Clean the inFix expression by replacing every occurrence of PI to its value == */
    replace(infixExpr_, std::string("pi"), std::string("3.1415926535"));
    replace(infixExpr_, std::string("PI"), std::string("3.1415926535"));

}

void RPNConverter::buildPostFix() {

    /* == Check for incoherence == */
    std::string restrictedOperators{"*/+-%^"};
    std::uint32_t i = 0;
    for (const auto &c: infixExpr_) {
        auto next = infixExpr_[++i];
        if (restrictedOperators.find(c) != std::string::npos &&
            restrictedOperators.find(next) != std::string::npos) {
            throwSpiderException("Expression ill formed. Two operators without operands: %s", infixExpr_.c_str());
        }
    }

    /* == Retrieve tokens == */
    std::uint32_t nTokens = 0;
    std::string operators{"*/+-%^)("};
    for (const auto &c: infixExpr_) {
        if (operators.find(c) != std::string::npos) {
            nTokens += (2 - (c == '(' || c == ')'));
        }
    }
    i = 0;
    std::uint32_t last = 0;
    tokens_.reserve(nTokens);
    for (const auto &c: infixExpr_) {
        if (operators.find(c) != std::string::npos) {
            if ((i - last) > 0) {
                tokens_.push_back(infixExpr_.substr(last, i - last));
            }
            std::string tokenOp{c};
            tokens_.push_back(tokenOp);
            last = i + 1;
        }
        i++;
    }
    if ((i - last) > 0) {
        tokens_.push_back(infixExpr_.substr(last, i - last));
    }

    std::string postfix;
    for (const auto &t : tokens_) {
        if (isFunction(t)) {
            auto opType = getStringToFunctionMap()[t];
            operatorStack_.push_front(opType);
        } else if (isOperator(t)) {
            /* == Handle operator == */
            auto opType = getStringToOperatorMap()[t];

            /* == Handle right parenthesis case == */
            if (opType == RPNOperatorType::RIGHT_PAR) {
                auto frontOPType = operatorStack_.front();

                /* == This should not fail because miss match parenthesis is checked on constructor == */
                while (frontOPType != RPNOperatorType::LEFT_PAR) {
                    /* == Put operator to the output == */
                    postfix += getOperatorToStringMap()[operatorStack_.front()] + std::string(" ");
                    operatorStack_.pop_front();
                    if (operatorStack_.empty()) {
                        break;
                    }

                    /* == Pop operator from the stack == */
                    frontOPType = operatorStack_.front();
                }

                /* == Pop left parenthesis == */
                operatorStack_.pop_front();
                continue;
            }

            /* == Handle left parenthesis case == */
            if (opType == RPNOperatorType::LEFT_PAR) {
                operatorStack_.push_front(opType);
                continue;
            }

            /* == Handle general case == */
            if (!operatorStack_.empty()) {
                auto op = getOperator(opType);
                auto frontOPType = operatorStack_.front();
                auto frontOP = getOperator(frontOPType);
                while (frontOPType != RPNOperatorType::LEFT_PAR &&
                       (op.precendence < frontOP.precendence ||
                        (op.precendence == frontOP.precendence && !frontOP.isRighAssociative))) {
                    /* == Put operator to the output == */
                    postfix += getOperatorToStringMap()[operatorStack_.front()] + std::string(" ");
                    operatorStack_.pop_front();
                    if (operatorStack_.empty()) {
                        break;
                    }

                    /* == Pop operator from the stack == */
                    frontOPType = operatorStack_.front();
                    frontOP = getOperator(frontOPType);
                }
            }

            /* == Push current operator to the stack == */
            operatorStack_.push_front(opType);
        } else {
            /* == Handle operand == */
            postfix += t + std::string(" ");
        }
    }
    while (!operatorStack_.empty()) {
        postfix += getOperatorToStringMap()[operatorStack_.front()] + std::string(" ");
        operatorStack_.pop_front();
    }
    fprintf(stderr, "INFO: postfix expression: %s\n", postfix.c_str());
}
