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

#include <containers/LinkedList.h>
#include <containers/StlContainers.h>
#include <algorithm>
#include <cstdint>

/* === Defines === */

#define N_OPERATOR 10
#define N_FUNCTION 9
#define FUNCTION_OPERATOR_OFFSET (static_cast<std::uint32_t>(RPNOperatorType::COS)) /*! Value of the @refitem RPNOperatorType::COS */

/* === Forward declaration(s) === */

class PiSDFParam;

class PiSDFGraph;

/* === Function pointer declaration == */

using evalFunction = double (*)(double &, double &);

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
    ADD = 0,        /*! Addition operator */
    SUB = 1,        /*! Subtraction operator */
    MUL = 2,        /*! Multiplication operator */
    DIV = 3,        /*! Division operator */
    MOD = 4,        /*! Modulo operator */
    POW = 5,        /*! Power operator */
    MAX = 6,        /*! Max operator */
    MIN = 7,        /*! Min operator */
    LEFT_PAR = 8,   /*! Left parenthesis */
    RIGHT_PAR = 9,  /*! Right parenthesis */
    COS = 10,       /*! Cosine function */
    SIN = 11,       /*! Sinus function */
    TAN = 12,       /*! Tangent function */
    EXP = 13,       /*! Exponential function */
    LOG = 14,       /*! Logarithm (base 10) function */
    LOG2 = 15,      /*! Logarithm (base 2) function */
    CEIL = 16,      /*! Ceil function */
    FLOOR = 17,     /*! Floor function */
    SQRT = 18,      /*! Square root function */
};

/**
 * @brief Operator structure.
 */
struct RPNOperator {
    RPNOperatorType type;      /*! Operator type (see @refitem RPNOperatorType) */
    std::uint16_t precedence;  /*! Precedence value level of the operator */
    bool isRighAssociative;    /*! Right associativity property of the operator */
    std::string label;         /*! Label of the operator */
    evalFunction eval;         /*! Associated function of the operator */
};

/* === Structure definition(s) === */

/**
 * @brief Structure defining an element for the Reverse Polish Notation (RPN)
 * conversion.
 */
struct RPNElement {
    RPNElementType type = RPNElementType::OPERATOR;
    RPNElementSubType subType = RPNElementSubType::OPERATOR;
    double value = 0.;
    PiSDFParam *param = nullptr;
    RPNOperatorType op = RPNOperatorType::ADD;
};

/* === Class definition === */

class RPNConverter {
public:
    RPNConverter() = default;

    RPNConverter(const RPNConverter &other) = default;

    inline RPNConverter(RPNConverter &&other) noexcept : RPNConverter() {
        swap(*this, other);
    }

    RPNConverter(const PiSDFGraph *graph, std::string inFixExpr);

    ~RPNConverter() = default;

    /* === Operators === */

    inline friend void swap(RPNConverter &first, RPNConverter &second) noexcept {
        /* == Enable ADL (not necessary in our case, but good practice) == */
        using std::swap;

        /* == Swap members of both objects == */
        swap(first.graph_, second.graph_);
        swap(first.infixExprString_, second.infixExprString_);
        swap(first.postfixExprString_, second.postfixExprString_);
        swap(first.postfixExprStack_, second.postfixExprStack_);
    }

    /**
     * @brief Copy and swap idiom for the assignment operator.
     * @param temp  Assigned object (will use copy or move constructor depending on context).
     * @return *this
     */
    inline RPNConverter &operator=(RPNConverter temp) {
        swap(*this, temp);
        return *this;
    }

    /* === Getters === */

    /**
     * @brief Get the expression infix string.
     * @return infix expression string.
     */
    inline const std::string &infixString() const;

    /**
     * @brief Get the expression postfix string.
     * @return postfix expression string.
     */
    inline const std::string &postfixString() const;

    /**
     * @brief Get the expression postfix stack.
     * @return postfix expression stack.
     */
    inline const Spider::vector<RPNElement> &postfixStack() const;

    /* === Static method(s) === */

    /**
     * @brief Return the operator associated to the operator type.
     * @param type  Operator type.
     * @return Associated operator.
     */
    static const RPNOperator &getOperator(RPNOperatorType type);

    /**
     * @brief Get the string label of an operator from its type.
     * @param type  Operator type.
     * @return string label of the operator.
     */
    static const std::string &getStringFromOperatorType(RPNOperatorType type);

    /**
     * @brief Retrieve the @refitem RPNOperatorType corresponding to a given string.
     * @param operatorString input string corresponding to the operator.
     * @return RPNOperatorType
     */
    static RPNOperatorType getOperatorTypeFromString(const std::string &operatorString);

private:
    const PiSDFGraph *graph_ = nullptr;
    std::string infixExprString_{""};
    std::string postfixExprString_{""};
    Spider::vector<RPNElement> postfixExprStack_;

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
     * @brief Check for miss match in the number of parenthesis
     * @return true if there is a miss match, false else.
     */
    inline bool missMatchParenthesis() const;
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

const std::string &RPNConverter::infixString() const {
    return infixExprString_;
}

const std::string &RPNConverter::postfixString() const {
    return postfixExprString_;
}

const Spider::vector<RPNElement> &RPNConverter::postfixStack() const {
    return postfixExprStack_;
}

#endif // SPIDER2_RPNCONVERTER_H
