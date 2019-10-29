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

#include <algorithm>
#include <graphs-tools/expression-parser/RPNConverter.h>
#include <containers/StlContainers.h>
#include <common/Exception.h>
#include <graphs/tmp/Graph.h>
#include <graphs/tmp/Param.h>
#include <graphs-tools/expression-parser/ParserFunctions.h>

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
            {RPNOperatorType::ADD,       2, false,
                    "+",     Spider::add},          /*! ADD operator */
            {RPNOperatorType::SUB,       2, false,
                    "-",     Spider::sub},          /*! SUB operator */
            {RPNOperatorType::MUL,       3, false,
                    "*",     Spider::mul},          /*! MUL operator */
            {RPNOperatorType::DIV,       3, false,
                    "/",     Spider::div},          /*! DIV operator */
            {RPNOperatorType::MOD,       4, false,
                    "%",     Spider::mod},          /*! MOD operator */
            {RPNOperatorType::POW,       4, true,
                    "^",     Spider::pow},          /*! POW operator */
            {RPNOperatorType::MAX,       3, false,
                    "max",   Spider::max},        /*! MAX operator */
            {RPNOperatorType::MIN,       3, false,
                    "min",   Spider::min},        /*! MIN operator */
            {RPNOperatorType::LEFT_PAR,  2, false,
                    "(",     Spider::dummyEval},    /*! LEFT_PAR operator */
            {RPNOperatorType::RIGHT_PAR, 2, false,
                    ")",     Spider::dummyEval},    /*! RIGHT_PAR operator */
            {RPNOperatorType::COS,       5, false,
                    "cos",   Spider::cos},        /*! COS function */
            {RPNOperatorType::SIN,       5, false,
                    "sin",   Spider::sin},        /*! SIN function */
            {RPNOperatorType::TAN,       5, false,
                    "tan",   Spider::tan},        /*! TAN function */
            {RPNOperatorType::EXP,       5, false,
                    "exp",   Spider::exp},        /*! EXP function */
            {RPNOperatorType::LOG,       5, false,
                    "log",   Spider::log},        /*! LOG function */
            {RPNOperatorType::LOG2,      5, false,
                    "log2",  Spider::log2},      /*! LOG2 function */
            {RPNOperatorType::CEIL,      5, false,
                    "ceil",  Spider::ceil},      /*! CEIL function */
            {RPNOperatorType::FLOOR,     5, false,
                    "floor", Spider::floor},    /*! FLOOR function */
            {RPNOperatorType::SQRT,      5, false,
                    "sqrt",  Spider::sqrt},    /*! SQRT function */
    };
    return rpnOperators[ix];
}

/* === Static Functions === */

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

static inline void setOperatorElement(RPNElement *elt, RPNOperatorType opType) {
    elt->type = RPNElementType::OPERATOR;
    elt->subType = isFunction(opType) ? RPNElementSubType::FUNCTION
                                      : RPNElementSubType::OPERATOR;
    elt->op = opType;
}

static inline void setOperandElement(RPNElement *elt, const std::string &token, const PiSDFGraph *graph) {
    elt->type = RPNElementType::OPERAND;
    char *end;
    auto value = std::strtod(token.c_str(), &end);
    if (end == token.c_str() || (*end) != '\0') {
        auto *param = graph->findParam(token);
        if (!param) {
            throwSpiderException("Did not find parameter [%s] for expression parsing.", token.c_str());
        }
        if (param->dynamic()) {
            elt->subType = RPNElementSubType::PARAMETER;
            elt->param = param;
        } else {
            elt->subType = RPNElementSubType::VALUE;
            elt->value = param->value();
        }
    } else {
        elt->subType = RPNElementSubType::VALUE;
        elt->value = value;
    }
}

static inline void addToken(Spider::vector<RPNElement> &tokens,
                            const std::string &token,
                            const PiSDFGraph *graph) {
    if (token.empty()) {
        return;
    }
    if (isOperator(token)) {
        tokens.push_back(RPNElement());

        /* == Function case == */
        setOperatorElement(&tokens.back(), RPNConverter::getOperatorTypeFromString(token));
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

static void retrieveExprTokens(std::string &inFixExpr,
                               Spider::vector<RPNElement> &tokens,
                               const PiSDFGraph *graph) {
    auto pos = inFixExpr.find_first_of(operators(), 0);
    std::uint32_t lastPos = 0;
    while (pos != std::string::npos) {
        /* == Operand or Function token (can be empty) == */
        auto token = inFixExpr.substr(lastPos, pos - lastPos);
        addToken(tokens, token, graph);

        /* == Operator == */
        token = inFixExpr.substr(pos, 1);
        tokens.push_back(RPNElement());
        setOperatorElement(&tokens.back(), RPNConverter::getOperatorTypeFromString(token));
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

/**
 * @brief In place replace of all occurrences of substring in a string.
 * @param s        String on which we are working.
 * @param pattern  Substring to find.
 * @param replace  Substring to replace found matches.
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

/* === Static method(s) === */

const RPNOperator &RPNConverter::getOperator(RPNOperatorType type) {
    return rpnOperators(static_cast<std::uint32_t >(type));
}

const std::string &RPNConverter::getStringFromOperatorType(RPNOperatorType type) {
    return getOperator(type).label;
}

RPNOperatorType RPNConverter::getOperatorTypeFromString(const std::string &operatorString) {
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

/* === Method(s) implementation === */

RPNConverter::RPNConverter(std::string inFixExpr, const PiSDFGraph *graph) : graph_{graph},
                                                                                       infixExprString_{
                                                                                               std::move(inFixExpr)} {
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

    /* == Build the postfix string expression == */
    for (auto &t : postfixExprStack_) {
        if (t.type == RPNElementType::OPERATOR) {
            postfixExprString_ += getStringFromOperatorType(t.op) + " ";
        } else if (t.subType == RPNElementSubType::PARAMETER) {
            postfixExprString_ += t.param->name() + " ";
        } else {
            postfixExprString_ += std::to_string(t.value) + std::string(" ");
        }
    }
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
            auto opType = t.op;
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
                           (op.precedence < frontOP.precedence ||
                            (op.precedence == frontOP.precedence && !frontOP.isRighAssociative))) {

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

