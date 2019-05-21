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
#include <regex>
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
        {.type = RPNOperatorType::CEIL, .precendence = 1, .isRighAssociative = false},  /*! CEIL operator */
        {.type = RPNOperatorType::FLOOR, .precendence = 1, .isRighAssociative = false}, /*! FLOOR operator */
        {.type = RPNOperatorType::LOG, .precendence = 1, .isRighAssociative = false},   /*! LOG operator */
        {.type = RPNOperatorType::LOG2, .precendence = 1, .isRighAssociative = false},  /*! LOG2 operator */
        {.type = RPNOperatorType::COS, .precendence = 1, .isRighAssociative = false},   /*! COS operator */
        {.type = RPNOperatorType::SIN, .precendence = 1, .isRighAssociative = false},   /*! SIN operator */
        {.type = RPNOperatorType::TAN, .precendence = 1, .isRighAssociative = false},   /*! TAN operator */
        {.type = RPNOperatorType::EXP, .precendence = 1, .isRighAssociative = false},   /*! EXP operator */
};

static std::map<std::string, RPNOperatorType> createMap() {
    std::map<std::string, RPNOperatorType> m;
    m["+"] = RPNOperatorType::ADD;
    m["-"] = RPNOperatorType::SUB;
    m["*"] = RPNOperatorType::MUL;
    m["/"] = RPNOperatorType::DIV;
    m["%"] = RPNOperatorType::MOD;
    m["^"] = RPNOperatorType::POW;
    m["cos"] = RPNOperatorType::COS;
    m["sin"] = RPNOperatorType::SIN;
    m["log"] = RPNOperatorType::LOG;
    m["log2"] = RPNOperatorType::LOG2;
    m["tan"] = RPNOperatorType::TAN;
    m["exp"] = RPNOperatorType::EXP;
    m["ceil"] = RPNOperatorType::CEIL;
    m["floor"] = RPNOperatorType::FLOOR;
    m["("] = RPNOperatorType::LEFT_PAR;
    m[")"] = RPNOperatorType::RIGHT_PAR;
    return m;
}

static std::map<RPNOperatorType, std::string> createReverseMap() {
    std::map<RPNOperatorType, std::string> m;
    m[RPNOperatorType::ADD] = "+";
    m[RPNOperatorType::SUB] = "-";
    m[RPNOperatorType::MUL] = "*";
    m[RPNOperatorType::DIV] = "/";
    m[RPNOperatorType::MOD] = "%";
    m[RPNOperatorType::POW] = "^";
    m[RPNOperatorType::COS] = "cos";
    m[RPNOperatorType::SIN] = "sin";
    m[RPNOperatorType::LOG] = "log";
    m[RPNOperatorType::LOG2] = "log2";
    m[RPNOperatorType::TAN] = "tan";
    m[RPNOperatorType::EXP] = "exp";
    m[RPNOperatorType::CEIL] = "ceil";
    m[RPNOperatorType::FLOOR] = "floor";
    return m;
}


static std::map<std::string, RPNOperatorType> stringToOperatorMap = createMap();
static std::map<RPNOperatorType, std::string> operatorToStringMap = createReverseMap();

/* === Methods implementation === */

RPNConverter::RPNConverter(std::string inFixExpr) : infixExpr_{std::move(inFixExpr)},
                                                    postfixExpr_(StackID::EXPR_PARSER) {
    if (missMatchParenthesis()) {
        throwSpiderException("Expression with miss matched parenthesis: %s", infixExpr_.c_str());
    }
    fprintf(stderr, "INFO: original expression:%s\n", infixExpr_.c_str());

    /* == Format properly the expression == */
    cleanInfixExpression();
    fprintf(stderr, "INFO: formatted expression:%s\n", infixExpr_.c_str());

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

void RPNConverter::cleanInfixExpression() {
    /* == Clean the inFix expression by removing all white spaces == */
    infixExpr_.erase(std::remove(infixExpr_.begin(), infixExpr_.end(), ' '), infixExpr_.end());

    /* == Clean the inFix expression by adding '*' for #valueY -> #value * Y  == */
    std::regex multReplacer("([0-9]+)([a-zA-Z])",
                            std::regex_constants::icase);
    infixExpr_ = std::regex_replace(infixExpr_, multReplacer, "$1*$2");

    /* == Clean the inFix expression by replacing every occurrence of PI to its value == */
    std::regex picleaner("pi", std::regex_constants::icase);
    infixExpr_ = std::regex_replace(infixExpr_, picleaner, "3.1415926535979");
}

void RPNConverter::buildPostFix() {

    /* == Retrieve operands == */
    std::regex operandsRegex("[^*/)(+^]+");
    auto words_begin = std::sregex_iterator(infixExpr_.begin(), infixExpr_.end(), operandsRegex);
    auto words_end = std::sregex_iterator();
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        operands_.push_back((*i).str());
        std::cerr << (*i).str() << std::endl;
    }

    /* == Retrieve operators == */
    std::regex operatorsRegex("[*/)(+^]");
    words_begin = std::sregex_iterator(infixExpr_.begin(), infixExpr_.end(), operatorsRegex);
    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        auto operatorString = (*i).str();
        operators_.push_back(operatorString);
        std::cerr << operatorString << std::endl;
    }

    std::string postfix;
    std::uint32_t i = 0;
    for (auto &s : operands_) {
        postfix += s + std::string(" ");
        if (i < operators_.size() &&
            stringToOperatorMap.find(operators_[i]) != stringToOperatorMap.end()) {
            auto opType = stringToOperatorMap[operators_[i]];
            if (!operatorStack_.empty()) {
                auto currentOPType = operatorStack_.front();
                auto op = rpnOperators[static_cast<std::uint32_t >(opType)];
                auto currentOP = rpnOperators[static_cast<std::uint32_t >(currentOPType)];
                while (opType != RPNOperatorType::LEFT_PAR &&
                       !operatorStack_.empty() &&
                       (op.precendence < currentOP.precendence ||
                        (op.precendence == currentOP.precendence && !currentOP.isRighAssociative))) {
                    postfix += operatorToStringMap[operatorStack_.front()] + std::string(" ");
                    operatorStack_.pop_front();
                    currentOPType = operatorStack_.front();
                    currentOP = rpnOperators[static_cast<std::uint32_t >(currentOPType)];
                }
            }
            operatorStack_.push_front(opType);
            i++;
        }
    }
    while (!operatorStack_.empty()) {
        postfix += operatorToStringMap[operatorStack_.front()] + std::string(" ");
        operatorStack_.pop_front();
    }
    std::cerr << postfix << std::endl;
}
