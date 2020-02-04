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
#include <containers/stack.h>
#include <common/Exception.h>
#include <cctype>

/* === Static variable definition(s) === */

/**
 * @brief String containing all supported operators (should not be edited).
 */
static const std::string &supportedBasicOperators() {
    static std::string operators{ "+-*/%^!()" };
    return operators;
}


/* === Static Function(s) === */

static bool isOperator(const std::string &s) {
    bool found = supportedBasicOperators().find_first_of(s) != std::string::npos;
    for (auto i = spider::rpn::FUNCTION_OFFSET; !found && i < spider::rpn::OPERATOR_COUNT; ++i) {
        found |= (spider::rpn::getOperator(i).label == s);
    }
    return found;
}

/**
 * @brief Check if an @refitem RPNOperatorType is a function or a base operator (ie. +,-,*,/,%,^)
 * @param type  Operator type.
 * @return true if type is a function, false else.
 */
static bool isFunction(RPNOperatorType type) {
    return static_cast<uint32_t >(type) >= spider::rpn::FUNCTION_OFFSET;
}

/**
 * @brief Check for miss match in the number of parenthesis
 * @return true if there is a miss match, false else.
 */
template<class It1, class It2>
static bool missMatchParenthesis(It1 first, It2 last) {
    uint32_t nLeftPar = 0;
    uint32_t nRightPar = 0;
    for (; first != last; ++first) {
        nLeftPar += ((*first) == '(');
        nRightPar += ((*first) == ')');
    }
    return nLeftPar != nRightPar;
}

/**
 * @brief Check for inconsistencies in the infix expression.
 * @param infixExprString String to evaluate.
 */
static void checkInfixExpression(const std::string &infixExprString) {
    static const auto &restrictedOperators = std::string{ "*/+-%^" };
    uint32_t i = 0;
    for (const auto &c: infixExprString) {
        i += 1;
        if (restrictedOperators.find(c) != std::string::npos) {
            auto next = infixExprString[i];
            if (restrictedOperators.find(next) != std::string::npos) {
                throwSpiderException("Expression ill formed. Two operators without operands between: %c -- %c", c,
                                     next);
            } else if (&c == &infixExprString.front() ||
                       &c == &infixExprString.back() || next == ')') {
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
 * @brief Perform clean and reformatting operations on the original infix expression.
 * @param infixExprString String to reformat.
 */
static std::string cleanInfixExpression(std::string infixExprString) {
    std::string cleanExpression;
    if (infixExprString.empty()) {
        return cleanExpression;
    }
    auto localInfixExpression = std::move(infixExprString);

    /* == Clean the inFix expression by removing all white spaces == */
    localInfixExpression.erase(std::remove(localInfixExpression.begin(), localInfixExpression.end(), ' '),
                               localInfixExpression.end());

    /* == Convert the infix to lowercase == */
    std::transform(localInfixExpression.begin(), localInfixExpression.end(), localInfixExpression.begin(), ::tolower);

    /* == Replace (+x) to (x) == */
    stringReplace(localInfixExpression, "(+", "(");

    /* == Replace (-x) to (0-x) == */
    stringReplace(localInfixExpression, "(-", "(0-");

    /* == Check if expression start with '-' == */
    if (localInfixExpression[0] == '-') {
        throwSpiderException("Expression starting with '-' detected. Please explicit parenthesis and multiplication.");
    }

    /* == Clean the inFix expression by adding '*' for #valueY -> #value * Y  == */
    /* = Worst case is actually (tmp.size() - 1) but we avoid dealing (size == 0) = */
    cleanExpression.reserve(localInfixExpression.size() * 2);
    uint32_t i = 0;
    bool ignore = false;
    for (const auto &c : localInfixExpression) {
        cleanExpression += c;
        auto next = localInfixExpression[++i];
        if (!ignore && ((std::isdigit(c) &&
                         (std::isalpha(next) || next == '(')) ||
                        (c == ')' &&
                         (next == '(' || std::isdigit(next) || std::isalpha(next))))) {
            cleanExpression += '*';
        }
        ignore = std::isalpha(c) && std::isdigit(next);
    }

    /* == In case there are functions with multiple operands, we add parenthesis to ensure proper evaluation == */
    auto positionComa = cleanExpression.find_first_of(',', 0);
    if (positionComa != std::string::npos) {
        /* == Replace ")" by "))" == */
        stringReplace(cleanExpression, ")", "))");

        /* == Replace "(" by "((" == */
        stringReplace(cleanExpression, "(", "((");

        /* == Replace "," by "),(" == */
        stringReplace(cleanExpression, ",", "),(");
    }

    /* == Clean the inFix expression by replacing every occurrence of PI to its value == */
    stringReplace(cleanExpression, "pi", "3.1415926535");
    return cleanExpression;
}

/**
 * @brief Add an @refitem RPNElement element to the current stack based on string token.
 * @param tokenStack Token stack.
 * @param token      String token to evaluate.
 */
static void addElementFromToken(spider::vector<RPNElement> &tokenStack, const std::string &token) {
    if (token.empty()) {
        return;
    }
    if (isOperator(token)) {
        /* == Function case == */
        const auto &opType = spider::rpn::getOperatorTypeFromString(token);
        const auto &subtype = isFunction(opType) ? RPNElementSubType::FUNCTION
                                                 : RPNElementSubType::OPERATOR;
        tokenStack.push_back(RPNElement(RPNElementType::OPERATOR, subtype, token));
    } else {
        auto pos = token.find_first_of(',', 0);
        if (pos != std::string::npos) {
            /* == Double operand case == */
            addElementFromToken(tokenStack, token.substr(0, pos++));
            addElementFromToken(tokenStack, token.substr(pos, (token.size() - pos)));
        } else {
            /* == Operand case == */
            char *end;
            std::strtod(token.c_str(), &end);
            auto subtype = (end == token.c_str() || (*end) != '\0') ? RPNElementSubType::PARAMETER
                                                                    : RPNElementSubType::VALUE;
            tokenStack.push_back(RPNElement(RPNElementType::OPERAND, subtype, token));
        }
    }
}

static bool trySwap(spider::vector<RPNElement> &stack,
                    const spider::vector<uint32_t> &left,
                    const spider::vector<uint32_t> &right) {
    if (stack[left.back()].token_ != stack[right.back()].token_) {
        return false;
    }
    const auto &token = stack[left.back()].token_;
    if (std::string("+-/*^").find(token) == std::string::npos) {
        return false;
    }
    bool swaped = false;

    /* == Operators "-/^" can not swap the most left elements == */
    auto it = left.begin() + (std::string("-/^").find(token) != std::string::npos);
    for (; it != left.end(); ++it) {
        if (stack[*it].subtype_ == RPNElementSubType::PARAMETER) {
            for (const auto &ixr: right) {
                if (stack[ixr].subtype_ == RPNElementSubType::VALUE) {
                    std::swap(stack[*it], stack[ixr]);
                    swaped = true;
                    break;
                }
            }
        }
    }
    return swaped;
}

/* === Function(s) implementation === */

std::string spider::rpn::infixString(const spider::vector<RPNElement> &postfixStack) {
    spider::stack<std::string> stack;
    stack.push("");
    for (const auto &element : postfixStack) {
        if (element.type_ == RPNElementType::OPERAND) {
            stack.push(element.token_);
        } else {
            const auto &op = getOperatorFromOperatorType(getOperatorTypeFromString(element.token_));
            std::string builtInfix;
            if (element.subtype_ == RPNElementSubType::FUNCTION) {
                builtInfix += (element.token_ + '(');
                spider::stack<std::string> reverseStack;
                for (uint32_t i = 0; i < op.argCount; ++i) {
                    reverseStack.push(std::move(stack.top()));
                    stack.pop();
                }
                builtInfix += (reverseStack.top());
                reverseStack.pop();
                for (uint32_t i = 1; i < op.argCount; ++i) {
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
                builtInfix += element.token_;
                builtInfix += (op2 + ')');
            }
            stack.push(builtInfix);
        }
    }
    return stack.top();
}

std::string spider::rpn::postfixString(const spider::vector<RPNElement> &postfixStack) {
    /* == Build the postfix string expression == */
    std::string postfixExpr;
    for (auto &t : postfixStack) {
        postfixExpr += t.token_ + " ";
    }
    postfixExpr.pop_back();
    return postfixExpr;
}

spider::vector<RPNElement> spider::rpn::extractInfixElements(std::string infixExpression) {
    if (missMatchParenthesis(infixExpression.begin(), infixExpression.end())) {
        throwSpiderException("Expression with miss matched parenthesis: %s", infixExpression.c_str());
    }

    /* == Format properly the expression == */
    auto infixExpressionLocal = cleanInfixExpression(std::move(infixExpression));

    /* == Check for incoherence(s) == */
    checkInfixExpression(infixExpressionLocal);

    auto tokens = factory::vector<RPNElement>(StackID::EXPRESSION);
    tokens.reserve(infixExpressionLocal.size());

    /* == Extract the expression elements == */
    auto pos = infixExpressionLocal.find_first_of(supportedBasicOperators(), 0);
    std::string::size_type lastPos = 0;
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

spider::vector<RPNElement> spider::rpn::extractPostfixElements(std::string infixExpression) {
    /* == Retrieve tokens == */
    auto infixStack = extractInfixElements(std::move(infixExpression));

    /* == Build the postfix expression == */
    auto operatorStack = factory::vector<std::pair<RPNOperatorType, RPNElement>>(StackID::EXPRESSION);

    /* == Actually, size will probably be inferior but this will avoid realloc == */
    auto postfixStack = factory::vector<RPNElement>(StackID::EXPRESSION);
    postfixStack.reserve(infixStack.size());
    for (auto &element : infixStack) {
        if (element.type_ == RPNElementType::OPERATOR) {
            const auto &operatorType = getOperatorTypeFromString(element.token_);
            if (element.subtype_ == RPNElementSubType::FUNCTION ||
                operatorType == RPNOperatorType::LEFT_PAR) {
                /* == Handle function and left parenthesis case == */
                operatorStack.emplace_back(std::make_pair(operatorType, std::move(element)));
            } else if (operatorType == RPNOperatorType::RIGHT_PAR) {
                /* == Handle right parenthesis case == */
                /* == This will not fail because miss match parenthesis is checked before == */
                while (operatorStack.back().first != RPNOperatorType::LEFT_PAR) {
                    /* == Move element to output stack == */
                    postfixStack.emplace_back(std::move(operatorStack.back().second));

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
                        postfixStack.emplace_back(std::move(operatorStack.back().second));

                        /* == Pop element from operator stack == */
                        operatorStack.pop_back();

                        /* == Update stop condition == */
                        stop = operatorStack.empty();
                    }
                }

                /* == Push current operator to the stack == */
                operatorStack.emplace_back(std::make_pair(operatorType, std::move(element)));
            }
        } else {
            /* == Handle operand == */
            postfixStack.emplace_back(std::move(element));
        }
    }

    /* == Pop the remaining elements in the operator stack == */
    for (auto it = operatorStack.rbegin(); it != operatorStack.rend(); ++it) {
        /* == Move element to output stack == */
        postfixStack.emplace_back(std::move((*it).second));
    }
    return postfixStack;
}

void spider::rpn::reorderPostfixStack(spider::vector<RPNElement> &postfixStack) {
    auto operationStackVector = factory::vector<spider::vector<uint32_t>>(StackID::EXPRESSION);
    operationStackVector.emplace_back(factory::vector<uint32_t>(StackID::EXPRESSION));
    operationStackVector[0].reserve(6);

    /* == Fill up the operation stack once == */
    uint32_t i = 0;
    for (const auto &elt : postfixStack) {
        operationStackVector.back().emplace_back(i++);
        if (elt.type_ == RPNElementType::OPERATOR) {
            if (operationStackVector.back().size() == 1) {
                break;
            }
            operationStackVector.emplace_back(factory::vector<uint32_t>(StackID::EXPRESSION));
            operationStackVector.back().reserve(6);
        }
    }

    /* == Iteratively try to reorder postfix expression stack based on operation stack == */
    bool swapped;
    do {
        swapped = false;

        /* == Try to swap element in operations == */
        auto it = operationStackVector.begin();
        for (; it != (operationStackVector.end() - 1); ++it) {
            swapped |= trySwap(postfixStack, (*it), (*(it + 1)));
        }
    } while (swapped);
}

const RPNOperator &spider::rpn::getOperator(uint32_t ix) {
    static std::array<RPNOperator, rpn::OPERATOR_COUNT>
            operatorArray{{
                                  { "+", RPNOperatorType::ADD, 1, 2, false },          /*! ADD operator */
                                  { "-", RPNOperatorType::SUB, 1, 2, false },          /*! SUB operator */
                                  { "*", RPNOperatorType::MUL, 2, 2, false },          /*! MUL operator */
                                  { "/", RPNOperatorType::DIV, 2, 2, false },          /*! DIV operator */
                                  { "%", RPNOperatorType::MOD, 3, 2, false },          /*! MOD operator */
                                  { "^", RPNOperatorType::POW, 3, 2, true },           /*! POW operator */
                                  { "!", RPNOperatorType::FACT, 4, 1, true },          /*! FACT operator */
                                  { "(", RPNOperatorType::LEFT_PAR, 1, 0, false },     /*! LEFT_PAR operator */
                                  { ")", RPNOperatorType::RIGHT_PAR, 1, 0, false },    /*! RIGHT_PAR operator */
                                  { "cos", RPNOperatorType::COS, 5, 1, false },        /*! COS function */
                                  { "sin", RPNOperatorType::SIN, 5, 1, false },        /*! SIN function */
                                  { "tan", RPNOperatorType::TAN, 5, 1, false },        /*! TAN function */
                                  { "cosh", RPNOperatorType::COSH, 5, 1, false },      /*! COSH function */
                                  { "sinh", RPNOperatorType::SINH, 5, 1, false },      /*! SINH function */
                                  { "tanh", RPNOperatorType::TANH, 5, 1, false },      /*! TANH function */
                                  { "exp", RPNOperatorType::EXP, 5, 1, false },        /*! EXP function */
                                  { "log", RPNOperatorType::LOG, 5, 1, false },        /*! LOG function */
                                  { "log2", RPNOperatorType::LOG2, 5, 1, false },      /*! LOG2 function */
                                  { "ceil", RPNOperatorType::CEIL, 5, 1, false },      /*! CEIL function */
                                  { "floor", RPNOperatorType::FLOOR, 5, 1, false },    /*! FLOOR function */
                                  { "abs", RPNOperatorType::ABS, 5, 1, false },        /*! ABS operator */
                                  { "sqrt", RPNOperatorType::SQRT, 5, 1, false },      /*! SQRT function */
                                  { "max", RPNOperatorType::MAX, 5, 2, false },        /*! MAX operator */
                                  { "min", RPNOperatorType::MIN, 5, 2, false },        /*! MIN operator */
                                  { "dummy", RPNOperatorType::DUMMY, 5, 1, false },    /*! Dummy operator */
                          }};
    return operatorArray.at(ix);
}

const RPNOperator &spider::rpn::getOperatorFromOperatorType(RPNOperatorType type) {
    return getOperator(static_cast<uint32_t >(type));
}

RPNOperatorType spider::rpn::getOperatorTypeFromString(const std::string &operatorString) {
    // TODO: implement it with a std::map<std::string, OperatorType> and try - catch block. see: Zero-Cost Exception model.
    bool found = false;
    uint32_t i = 0;
    for (; !found && i < rpn::OPERATOR_COUNT; ++i) {
        found |= (getOperator(i).label == operatorString);
    }
    if (!found) {
        throwSpiderException("Can not convert string [%s] to operator.", operatorString.c_str());
    }
    return getOperator(i - 1).type;
}

