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
#ifndef SPIDER2_RPNCONVERTER_H
#define SPIDER2_RPNCONVERTER_H

/* === Includes === */

#include <common/containers/LinkedList.h>
#include <common/containers/StlContainers.h>
#include <algorithm>
#include <cstdint>

/* === Defines === */

#define N_OPERATOR 8
#define N_FUNCTION 8
#define FUNCTION_OPERATOR_OFFSET 6 /*! Value of the @refitem RPNOperatorType::COS */

/* === Forward declaration(s) === */

class PiSDFParam;

class PiSDFGraph;

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
    VALUE,     /*! Value (digit) */
    PARAMETER, /*! Value coming from a parameter */
//    LEFT_PAR,  /*! Operator is a left parenthesis */
//    RIGHT_PAR, /*! Operator is a right parenthesis */
            FUNCTION,  /*! Operator is a function */
    OPERATOR,  /*! Operator is an elementary operator */
};

/**
 * @brief Enumeration of the supported operators by the parser.
 */
enum class RPNOperatorType : std::uint32_t {
    ADD = 0,        /*! Addition operator */
    SUB = 1,        /*! Subtraction operator */
    MUL = 2,        /*! Multiplication operator */
    DIV = 3,        /*! Division operator */
    MOD = 4,        /*! Modulo operator */
    POW = 5,        /*! Power operator */
    COS = 6,        /*! Cosine function */
    SIN = 7,        /*! Sinus function */
    TAN = 8,        /*! Tangent function */
    EXP = 9,        /*! Exponential function */
    LOG = 10,       /*! Logarithm (base 10) function */
    LOG2 = 11,      /*! Logarithm (base 2) function */
    CEIL = 12,      /*! Ceil function */
    FLOOR = 13,     /*! Floor function */
    LEFT_PAR = 14,  /*! Left parenthesis */
    RIGHT_PAR = 15, /*! Right parenthesis */
};

/**
 * @brief Operator structure.
 */
struct RPNOperator {
    RPNOperatorType type;      /*! Operator type (see @refitem RPNOperatorType) */
    std::uint16_t precendence; /*! Precedence value level of the operator */
    bool isRighAssociative;    /*! Right associativity property of the operator */
};

/* === Structure definition(s) === */

/**
 * @brief Structure defining an element for the Reverse Polish Notation (RPN)
 * conversion.
 */
struct RPNElement {
    RPNElementType type = RPNElementType::OPERATOR;
    RPNElementSubType subType = RPNElementSubType::OPERATOR;
    union {
        double value = 0.;
        PiSDFParam *param;
        RPNOperatorType op;
    } element;
};


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

class RPNConverter {
public:
    explicit RPNConverter(std::string inFixExpr, PiSDFGraph *graph);

    ~RPNConverter();

    /* === Methods === */

    /**
     * @brief Get the expression postfix string
     * @return
     */
    std::string toString();

    /**
     * @brief Get the expression infix string
     * @return
     */
    inline std::string infixString() const;

    void printExpressionTree();

    /**
     * @brief Evaluate the expression (optimized)
     * @return Result of the evaluated expression
     */
    double evaluate() const;

    /* === Getters === */

    /**
     * @brief Get the static property of the expression.
     * @return true if the expression is static, false else.
     */
    inline bool isStatic() const;

private:

    std::string infixExprString_;
    std::string postfixExprString_{""};
    PiSDFGraph *graph_ = nullptr;
    bool static_ = false;
    Spider::deque<RPNElement> postfixExprStack_;
    ExpressionTreeNode *expressionTree_ = nullptr;

    /* === Private Methods === */

    /**
     * @brief Perform clean and reformatting operations on the original infix
     * expression.
     */
    void cleanInfixExpression();

    /**
     * @brief Check for inconsistencies in the infix expression.
     */
    void checkInfixExpression() const;

    /**
     * @brief Build the postfix expression.
     */
    void buildPostFix();

    /**
     * @brief Build and reduce the expression tree parser.
     */
    void buildExpressionTree();

    /**
     * @brief Check for miss match in the number of parenthesis
     * @return true if there is a miss match, false else.
     */
    inline bool missMatchParenthesis() const;

    /**
     * @brief In place replace of all occurrences of substring in a string.
     * @param s     String on which we are working.
     * @param pattern  Substring to find.
     * @param replace    Substring to replace found matches.
     * @return Modified string (same as s).
     */
    std::string &replace(std::string &s, const std::string &pattern,
                         const std::string &replace);
};

/* === Inline methods === */

bool RPNConverter::missMatchParenthesis() const {
    std::uint32_t nLeftPar = 0;
    std::uint32_t nRightPar = 0;
    for (auto &t : infixExprString_) {
        nLeftPar += (t == '(');
        nRightPar += (t == ')');
    }
    return nLeftPar != nRightPar;
}

bool RPNConverter::isStatic() const {
    return static_;
}


std::string RPNConverter::infixString() const {
    return infixExprString_;
}

#endif // SPIDER2_RPNCONVERTER_H
