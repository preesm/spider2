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
#ifndef SPIDER2_RPNCONVERTER_H
#define SPIDER2_RPNCONVERTER_H

/* === Includes === */

#include <algorithm>
#include <cstdint>
#include <containers/StlContainers.h>

/* === Function pointer declaration == */

using evalFunction = double (*)(const double &, const double &);

/* === Enum declaration(s) === */

/**
 * @brief Primary type of an @refitem RPNElement.
 */
enum class RPNElementType : std::uint16_t {
    OPERATOR, /*! Operator element */
    OPERAND,  /*! Operand element */
};

/**
 * @brief Secondary type of an @refitem RPNElement
 */
enum class RPNElementSubType : std::uint16_t {
    VALUE,      /*! Value (digit) */
    PARAMETER,  /*! Value coming from a parameter */
    FUNCTION,   /*! Operator is a function */
    OPERATOR,   /*! Operator is an elementary operator */
};

/**
 * @brief Enumeration of the supported operators by the parser.
 */
enum class RPNOperatorType : std::uint32_t {
    ADD,        /*! Addition operator */
    SUB,        /*! Subtraction operator */
    MUL,        /*! Multiplication operator */
    DIV,        /*! Division operator */
    MOD,        /*! Modulo operator */
    POW,        /*! Power operator */
    LEFT_PAR,   /*! Left parenthesis */
    RIGHT_PAR,  /*! Right parenthesis */
    COS,        /*! Cosine function */
    SIN,        /*! Sinus function */
    TAN,        /*! Tangent function */
    EXP,        /*! Exponential function */
    LOG,        /*! Logarithm (base 10) function */
    LOG2,       /*! Logarithm (base 2) function */
    CEIL,       /*! Ceil function */
    FLOOR,      /*! Floor function */
    SQRT,       /*! Square root function */
    MAX,        /*! Max function */
    MIN,        /*! Min function */
};

/**
 * @brief Operator structure.
 */
struct RPNOperator {
    RPNOperatorType type;     /*! Operator type (see @refitem RPNOperatorType) */
    std::uint16_t precedence; /*! Precedence value level of the operator */
    bool isRighAssociative;   /*! Right associativity property of the operator */
    std::string label;        /*! Label of the operator */
    evalFunction eval;        /*! Associated function of the operator */
    std::uint32_t argCount;   /*! Number of argument of the operator */
};

/* === Structure definition(s) === */

/**
 * @brief Structure defining an element for the Reverse Polish Notation (RPN)
 * conversion.
 */
struct RPNElement {
    RPNElementType type = RPNElementType::OPERATOR;
    RPNElementSubType subtype = RPNElementSubType::OPERATOR;
    std::string token;

    RPNElement() = default;

    RPNElement(const RPNElement &o) = default;

    RPNElement(RPNElement &&other) noexcept = default;

    RPNElement(RPNElementType type, RPNElementSubType subtype, std::string token) : type{type},
                                                                                    subtype{subtype},
                                                                                    token{std::move(token)} { }
};

namespace RPNConverter {
    constexpr auto operator_count = static_cast<std::uint32_t>(RPNOperatorType::RIGHT_PAR) + 1;

    /* == Value of the @refitem RPNOperatorType::COS (first function) == */
    constexpr auto function_offset = static_cast<std::uint32_t>(RPNOperatorType::COS);

    /* == Value of the @refitem RPNOperatorType::MIN (last function) == */
    constexpr auto function_count = static_cast<std::uint32_t>(RPNOperatorType::MIN) - function_offset + 1;

    /**
     * @brief Build the expression infix string from the stack of postfix elements.
     * @param postfixStack  Stack of postfix elements.
     * @return infix expression string.
     */
    std::string infixString(const Spider::vector<RPNElement> &postfixStack);

    /**
     * @brief Build the expression postfix string from the stack of postfix elements.
     * @param postfixStack  Stack of postfix elements.
     * @return postfix expression string.
     */
    std::string postfixString(const Spider::vector<RPNElement> &postfixStack);

    /**
     * @brief Extract the infix expression tokens.
     * @remark This function will perform several checks on the input string and will clean it before treating it.
     *         For instance: expr = "( sin(4pi))" will become cleanExpr = "(sin(4*3.1415926535))".
     * @param inFixExpr  Infix expression to evaluate.
     * @return vector of @refitem RPNElement in the infix order.
     * @throws @refitem Spider::Exception if expression is ill formed.
     */
    Spider::vector<RPNElement> extractInfixElements(std::string inFixExpr);

    /**
     * @brief Extract the different elements (operand and operators) and build the post fix elements stack.
     * @remark This function calls @refitem extractInfixElements then build the postfix stack from its result.
     * @param infixExpression   Input infix notation string.
     * @return vector of @refitem RPNElement in the postfix order.
     */
    Spider::vector<RPNElement> extractPostfixElements(std::string infixExpression);

    /**
     * @brief Get the operator corresponding to the ix (value of the enum @refitem RPNOperatorType).
     * @param ix  ix of the enum.
     * @return @refitem RPNOperator.
     * @throws std::out_of_range if bad ix is passed.
     */
    const RPNOperator &getOperator(std::uint32_t ix);

    /**
     * @brief Return the operator associated to the operator type.
     * @param type  Operator type.
     * @return Associated operator.
     */
    const RPNOperator &getOperatorFromOperatorType(RPNOperatorType type);

    /**
     * @brief Get the string label of an operator from its type.
     * @param type  Operator type.
     * @return string label of the operator.
     */
    const std::string &getStringFromOperatorType(RPNOperatorType type);

    /**
     * @brief Retrieve the @refitem RPNOperatorType corresponding to a given string.
     * @param operatorString input string corresponding to the operator.
     * @return RPNOperatorType
     */
    RPNOperatorType getOperatorTypeFromString(const std::string &operatorString);
}
#endif // SPIDER2_RPNCONVERTER_H
