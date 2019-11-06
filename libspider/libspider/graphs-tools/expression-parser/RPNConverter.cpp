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
#include <graphs-tools/expression-parser/ParserFunctions.h>
#include <iostream>

/* === Static variable definition(s) === */

/**
 * @brief String containing all supported operators (should not be edited).
 */
static const std::string &supportedBasicOperators() {
    static std::string operators{"+-*/%^()"};
    return operators;
}


/* === Static Function(s) === */

static bool isOperator(const std::string &s) {
    bool found = false;
    for (std::uint32_t i = 0; !found && i < (RPNConverter::operator_count + RPNConverter::function_count); ++i) {
        found |= (RPNConverter::getOperator(i).label == s);
    }
    return found;
}

static bool isFunction(RPNOperatorType type) {
    return static_cast<std::uint32_t >(type) >= RPNConverter::function_offset;
}

/**
 * @brief Check for miss match in the number of parenthesis
 * @return true if there is a miss match, false else.
 */
template<class It1, class It2>
static bool missMatchParenthesis(It1 first, It2 last) {
    std::uint32_t nLeftPar = 0;
    std::uint32_t nRightPar = 0;
    for (; first != last; ++first) {
        nLeftPar += ((*first) == '(');
        nRightPar += ((*first) == ')');
    }
    return nLeftPar != nRightPar;
}

/**
 * @brief Check for inconsistencies in the infix expression.
 */
static void checkInfixExpression(const std::string &infixExprString) {
    static const auto &restrictedOperators = std::string{"*/+-%^"};
    std::uint32_t i = 0;
    for (const auto &c: infixExprString) {
        i += 1;
        if (restrictedOperators.find(c) != std::string::npos) {
            auto next = infixExprString[i];
            if (restrictedOperators.find(next) != std::string::npos) {
                throwSpiderException("Expression ill formed. Two operators without operands between: %c -- %c", c,
                                     next);
            } else if (&c == &infixExprString.front() ||
                       &c == &infixExprString.back()) {
                throwSpiderException("Expression ill formed. Operator [%c] expecting two operands.", c);
            }
        }
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

/**
 * @brief Perform clean and reformatting operations on the original infix
 * expression.
 */
static void cleanInfixExpression(std::string &infixExprString) {
    if (infixExprString.empty()) {
        return;
    }
    /* == Clean the inFix expression by removing all white spaces == */
    infixExprString.erase(std::remove(infixExprString.begin(), infixExprString.end(), ' '), infixExprString.end());

    /* == Convert the infix to lowercase == */
    std::transform(infixExprString.begin(), infixExprString.end(), infixExprString.begin(), ::tolower);

    /* == Clean the inFix expression by adding '*' for #valueY -> #value * Y  == */
    std::string tmp{std::move(infixExprString)};
    infixExprString = std::string();
    infixExprString.reserve(tmp.size() * 2); /*= Worst case is actually (tmp.size() - 1) = */
    std::uint32_t i = 0;
    bool ignore = false;
    for (const auto &c:tmp) {
        infixExprString += c;
        auto next = tmp[++i];
        if (!ignore && ((std::isdigit(c) &&
                         (std::isalpha(next) || next == '(')) ||
                        (c == ')' &&
                         (next == '(' || std::isdigit(next) || std::isalpha(next))))) {
            infixExprString += '*';
        }
        ignore = std::isalpha(c) && std::isdigit(next);
    }

    auto positionComa = infixExprString.find_first_of(',', 0);
    if (positionComa != std::string::npos) {
        /* == Replace ")" by "))" == */
        stringReplace(infixExprString, ")", "))");

        /* == Replace "(" by "((" == */
        stringReplace(infixExprString, "(", "((");

        /* == Replace "," by "),(" == */
        stringReplace(infixExprString, ",", "),(");
    }

    /* == Clean the inFix expression by replacing every occurrence of PI to its value == */
    stringReplace(infixExprString, "pi", "3.1415926535");
}

static void addElementFromToken(Spider::vector<RPNElement> &tokens, const std::string &token) {
    if (token.empty()) {
        return;
    }
    if (isOperator(token)) {
        /* == Function case == */
        const auto &opType = RPNConverter::getOperatorTypeFromString(token);
        const auto &subtype = isFunction(opType) ? RPNElementSubType::FUNCTION
                                                 : RPNElementSubType::OPERATOR;
        tokens.push_back(RPNElement(RPNElementType::OPERATOR, subtype, token));
    } else {
        auto pos = token.find_first_of(',', 0);
        if (pos != std::string::npos) {
            /* == Double operand case == */
            addElementFromToken(tokens, token.substr(0, pos++));
            addElementFromToken(tokens, token.substr(pos, (token.size() - pos)));
        } else {
            /* == Operand case == */
            char *end;
            std::strtod(token.c_str(), &end);
            auto subtype = (end == token.c_str() || (*end) != '\0') ? RPNElementSubType::PARAMETER
                                                                    : RPNElementSubType::VALUE;
            tokens.push_back(RPNElement(RPNElementType::OPERAND, subtype, token));
        }
    }
}

/* === Function(s) implementation === */

std::string RPNConverter::infixString(const Spider::vector<RPNElement> &postfixStack) {
    Spider::stack<std::string> stack;
    for (const auto &element : postfixStack) {
        if (element.type == RPNElementType::OPERAND) {
            stack.push(element.token);
        } else {
            const auto &op = getOperatorFromOperatorType(getOperatorTypeFromString(element.token));
            std::string builtInfix;
            if (element.subtype == RPNElementSubType::FUNCTION) {
                builtInfix += (element.token + '(');
                Spider::stack<std::string> reverseStack;
                for (std::uint32_t i = 0; i < op.argCount; ++i) {
                    reverseStack.push(std::move(stack.top()));
                    stack.pop();
                }
                builtInfix += (reverseStack.top());
                reverseStack.pop();
                for (std::uint32_t i = 1; i < op.argCount; ++i) {
                    builtInfix += (',' + reverseStack.top());
                    reverseStack.pop();
                }
                builtInfix += ')';
            } else {
                auto op2 = stack.top();
                stack.pop();
                auto op1 = stack.top();
                stack.pop();
                builtInfix += ('(' + op1);
                builtInfix += element.token;
                builtInfix += (op2 + ')');
            }
            stack.push(builtInfix);
        }
    }
    return stack.top();
}

std::string RPNConverter::postfixString(const Spider::vector<RPNElement> &postfixStack) {
    /* == Build the postfix string expression == */
    std::string postfixExpr;
    for (auto &t : postfixStack) {
        postfixExpr += t.token + " ";
    }
    return postfixExpr;
}

Spider::vector<RPNElement> RPNConverter::extractInfixElements(std::string infixExpression) {
    auto infixExpressionLocal = std::move(infixExpression);
    if (missMatchParenthesis(infixExpressionLocal.begin(), infixExpressionLocal.end())) {
        throwSpiderException("Expression with miss matched parenthesis: %s", infixExpressionLocal.c_str());
    }

    /* == Format properly the expression == */
    cleanInfixExpression(infixExpressionLocal);

    /* == Check for incoherence(s) == */
    checkInfixExpression(infixExpressionLocal);

    /* == Extract the expression elements == */
    Spider::vector<RPNElement> tokens;
    tokens.reserve(infixExpressionLocal.size());
    auto pos = infixExpressionLocal.find_first_of(supportedBasicOperators(), 0);
    std::uint32_t lastPos = 0;
    while (pos != std::string::npos) {
        /* == Operand or Function token (can be empty) == */
        auto token = infixExpressionLocal.substr(lastPos, pos - lastPos);
        addElementFromToken(tokens, token);

        /* == Operator element == */
        token = infixExpressionLocal.substr(pos++, 1);
        addElementFromToken(tokens, token);

        /* == Update pos == */
        lastPos = pos;
        pos = infixExpressionLocal.find_first_of(supportedBasicOperators(), pos);
    }

    /* == Potential left over (if expression ends with an operand) == */
    if (lastPos != infixExpressionLocal.size()) {
        const auto &token = infixExpressionLocal.substr(lastPos, infixExpressionLocal.size() - lastPos);
        addElementFromToken(tokens, token);
    }
    return tokens;
}

Spider::vector<RPNElement> RPNConverter::extractPostfixElements(std::string infixExpression) {
    /* == Retrieve tokens == */
    auto infixStack = extractInfixElements(std::move(infixExpression));

    /* == Build the postfix expression == */
    Spider::vector<std::pair<RPNOperatorType, RPNElement>> operatorStack;

    /* == Actually, size will probably be inferior but this will avoid realloc == */
    Spider::vector<RPNElement> postfixStack;
    postfixStack.reserve(infixStack.size());
    for (auto &element : infixStack) {
        if (element.type == RPNElementType::OPERATOR) {
            const auto &operatorType = getOperatorTypeFromString(element.token);
            if (element.subtype == RPNElementSubType::FUNCTION ||
                operatorType == RPNOperatorType::LEFT_PAR) {
                /* == Handle function and left parenthesis case == */
                operatorStack.push_back(std::make_pair(operatorType, std::move(element)));
            } else if (operatorType == RPNOperatorType::RIGHT_PAR) {
                /* == Handle right parenthesis case == */
                /* == This will not fail because miss match parenthesis is checked before == */
                while (operatorStack.back().first != RPNOperatorType::LEFT_PAR) {
                    /* == Move element to output stack == */
                    postfixStack.push_back(std::move(operatorStack.back().second));

                    /* == Pop element from operator stack == */
                    operatorStack.pop_back();
                }

                /* == Pop left parenthesis == */
                operatorStack.pop_back();
            } else {
                /* == Handle general case == */
                const auto &currentOperator = getOperatorFromOperatorType(operatorType);
                bool stop = operatorStack.empty();
                while (!stop) {
                    stop = true;
                    const auto &frontOperatorType = operatorStack.back().first;
                    const auto &frontOP = getOperatorFromOperatorType(frontOperatorType);
                    if (frontOperatorType != RPNOperatorType::LEFT_PAR &&
                        (currentOperator.precedence < frontOP.precedence ||
                         (currentOperator.precedence == frontOP.precedence && !frontOP.isRighAssociative))) {

                        /* == Move element to output stack == */
                        postfixStack.push_back(std::move(operatorStack.back().second));

                        /* == Pop element from operator stack == */
                        operatorStack.pop_back();

                        /* == Update stop condition == */
                        stop = operatorStack.empty();
                    }
                }

                /* == Push current operator to the stack == */
                operatorStack.push_back(std::make_pair(operatorType, std::move(element)));
            }
        } else {
            /* == Handle operand == */
            postfixStack.push_back(std::move(element));
        }
    }

    /* == Pop the remaining elements in the operator stack == */
    for (auto it = operatorStack.rbegin(); it != operatorStack.rend(); ++it) {
        /* == Move element to output stack == */
        postfixStack.push_back(std::move((*it).second));
    }
    return postfixStack;
}

const RPNOperator &RPNConverter::getOperator(std::uint32_t ix) {
    static std::array<RPNOperator, (RPNConverter::operator_count + RPNConverter::function_count)>
            operatorArray{{
                                  {RPNOperatorType::ADD, 2, false,
                                          "+", Spider::add, 2},          /*! ADD operator */
                                  {RPNOperatorType::SUB, 2, false,
                                          "-", Spider::sub, 2},          /*! SUB operator */
                                  {RPNOperatorType::MUL, 3, false,
                                          "*", Spider::mul, 2},          /*! MUL operator */
                                  {RPNOperatorType::DIV, 3, false,
                                          "/", Spider::div, 2},          /*! DIV operator */
                                  {RPNOperatorType::MOD, 4, false,
                                          "%", Spider::mod, 2},          /*! MOD operator */
                                  {RPNOperatorType::POW, 4, true,
                                          "^", Spider::pow, 2},          /*! POW operator */
                                  {RPNOperatorType::LEFT_PAR, 2, false,
                                          "(", Spider::dummyEval, 0},    /*! LEFT_PAR operator */
                                  {RPNOperatorType::RIGHT_PAR, 2, false,
                                          ")", Spider::dummyEval, 0},    /*! RIGHT_PAR operator */
                                  {RPNOperatorType::COS, 5, false,
                                          "cos", Spider::cos, 1},        /*! COS function */
                                  {RPNOperatorType::SIN, 5, false,
                                          "sin", Spider::sin, 1},        /*! SIN function */
                                  {RPNOperatorType::TAN, 5, false,
                                          "tan", Spider::tan, 1},        /*! TAN function */
                                  {RPNOperatorType::EXP, 5, false,
                                          "exp", Spider::exp, 1},        /*! EXP function */
                                  {RPNOperatorType::LOG, 5, false,
                                          "log", Spider::log, 1},        /*! LOG function */
                                  {RPNOperatorType::LOG2, 5, false,
                                          "log2", Spider::log2, 1},      /*! LOG2 function */
                                  {RPNOperatorType::CEIL, 5, false,
                                          "ceil", Spider::ceil, 1},      /*! CEIL function */
                                  {RPNOperatorType::FLOOR, 5, false,
                                          "floor", Spider::floor, 1},    /*! FLOOR function */
                                  {RPNOperatorType::SQRT, 5, false,
                                          "sqrt", Spider::sqrt, 1},      /*! SQRT function */
                                  {RPNOperatorType::MAX, 5, false,
                                          "max", Spider::max, 2},        /*! MAX operator */
                                  {RPNOperatorType::MIN, 5, false,
                                          "min", Spider::min, 2},        /*! MIN operator */
                          }};
    return operatorArray.at(ix);
}

const RPNOperator &RPNConverter::getOperatorFromOperatorType(RPNOperatorType type) {
    return getOperator(static_cast<std::uint32_t >(type));
}

const std::string &RPNConverter::getStringFromOperatorType(RPNOperatorType type) {
    return getOperatorFromOperatorType(type).label;
}

RPNOperatorType RPNConverter::getOperatorTypeFromString(const std::string &operatorString) {
    // TODO: implement it with a std::map<std::string, OperatorType> and try - catch block. see: Zero-Cost Exception model.
    bool found = false;
    std::uint32_t i = 0;
    for (i = 0; !found && i < (RPNConverter::operator_count + RPNConverter::function_count); ++i) {
        found |= (getOperator(i).label == operatorString);
    }
    if (!found) {
        throwSpiderException("Can not convert string [%s] to operator.", operatorString.c_str());
    }
    return getOperator(i - 1).type;
}

