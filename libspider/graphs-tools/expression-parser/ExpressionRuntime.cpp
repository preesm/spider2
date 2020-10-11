/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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
#ifndef _SPIDER_JIT_EXPRESSION

/* === Include(s) === */

#include <graphs-tools/expression-parser/ExpressionRuntime.h>
#include <graphs-tools/expression-parser/helper/ExpressionNumeric.h>
#include <graphs/pisdf/Param.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::Expression::Expression(int64_t value) : expr_{ static_cast<double>(value) },
                                                symbolTable_{ nullptr } {
}

spider::Expression::Expression(std::string expression, const param_table_t &params) {
    /* == Get the postfix expression stack == */
    auto postfixStack = rpn::extractPostfixElements(std::move(expression));
    if (log::enabled<log::EXPR>()) {
        log::verbose<log::EXPR>("infix expression: [%s].\n", rpn::infixString(postfixStack).c_str());
        log::verbose<log::EXPR>("postfix expression: [%s].\n", rpn::postfixString(postfixStack).c_str());
    }
    /* == Reorder the postfix stack elements to increase the number of static evaluation done on construction == */
//    rpn::reorderPostfixStack(postfixStack);

    /* == Perform partial evaluation of the expression (if possible) and compile the expression stack == */
    compile(postfixStack, params);
}

spider::Expression::Expression(const spider::Expression &rhs) : expr_{ rhs.expr_ } {
    if (rhs.symbolTable_) {
        symbolTable_ = make<symbol_table_t, StackID::EXPRESSION>(*rhs.symbolTable_);
    }
}

spider::Expression::~Expression() {
    destroy(symbolTable_);
}

/* === Private method(s) implementation === */

spider::Expression::param_t
spider::Expression::findParameter(const param_table_t &params, const std::string &name) {
    for (const auto &p : params) {
        if (p->name() == name) {
            return p.get();
        }
    }
    throwSpiderException("Did not find parameter [%s] for expression parsing.", name.c_str());
}

size_t spider::Expression::registerSymbol(param_t const param) {
    size_t i = 0;
    for (const auto &s : *symbolTable_) {
        if (s.first == param->name()) {
            return i;
        }
        i++;
    }
    symbolTable_->emplace_back(param->name(), 0.);
    return symbolTable_->size() - 1u;
}

void spider::Expression::updateSymbolTable(const param_table_t &params) const {
    for (auto &sym : *symbolTable_) {
        const auto &token = sym.first;
        for (const auto &p : params) {
            if (token == p->name()) {
                sym.second = static_cast<double>(p->value(params));
                break;
            }
        }
    }
}

double spider::Expression::evaluateImpl(const spider::vector<std::shared_ptr<pisdf::Param>> &params) const {
    /* == Update symbol table == */
    updateSymbolTable(params);
    /* == Evaluate stack == */
    return expr_(*symbolTable_);
}

void spider::Expression::compile(const spider::vector<RPNElement> &postfixStack, const param_table_t &params) {
    /* == Compile partially evaluated stack into evaluation stack == */
    symbolTable_ = make<symbol_table_t, StackID::EXPRESSION>();
    if (postfixStack.empty()) {
        expr_ = 0.;
    } else {
        auto iterator = postfixStack.rbegin();
        expr_ = compile(iterator, postfixStack.crend(), params);
    }
    if (expr_.type() == expr::Token::CONSTANT) {
        hash_ = std::hash<std::string>{ }(std::to_string(expr_.value_));
        destroy(symbolTable_);
    } else {
        hash_ = std::hash<std::string>{ }(rpn::postfixString(postfixStack));
    }
}

spider::expr::Token
spider::Expression::compile(spider::vector<RPNElement>::const_reverse_iterator &iterator,
                            const spider::vector<RPNElement>::const_reverse_iterator& end,
                            const param_table_t &params) {
    if (iterator == end) {
        throwSpiderException("invalid number of argument.");
    }
    const auto &elt = *(iterator++);
    if (elt.type_ == RPNElementType::OPERATOR) {
        const auto opType = rpn::getOperatorTypeFromString(elt.token_);
        const auto &op = rpn::getOperatorFromOperatorType(opType);
        switch (op.argCount) {
            case 1:
                return generate(opType, compile(iterator, end, params));
            case 2: {
                const auto right = compile(iterator, end, params);
                const auto left = compile(iterator, end, params);
                return generate(opType, left, right);
            }
            case 3: {
                const auto arg2 = compile(iterator, end, params);
                const auto arg1 = compile(iterator, end, params);
                const auto arg0 = compile(iterator, end, params);
                return generate(opType, arg0, arg1, arg2);
            }
            default:
                throwSpiderException("Invalid number of argument.");
        }
    } else if (elt.subtype_ == RPNElementSubType::PARAMETER) {
        const auto param = findParameter(params, elt.token_);
        if (param->dynamic()) {
            return { registerSymbol(param) };
        }
        return { static_cast<double>(param->value(params)) };
    }
    return { std::strtod(elt.token_.c_str(), nullptr) };
}

spider::expr::Token
spider::Expression::generate(RPNOperatorType type, const expr::Token &arg) const {
    if (expr::Token::CONSTANT == arg.type()) {
        return { numeric::apply(type, arg.value_) };
    } else if (expr::Token::VARIABLE == arg.type()) {
        return makeUnaryFunction(type, arg.index_);
    }
    return makeUnaryFunction(type, arg.f_);
}

spider::expr::Token
spider::Expression::generate(RPNOperatorType type, const expr::Token &left, const expr::Token &right) const {
    if (expr::details::isConstConst(left, right)) {
        return { numeric::apply(type, left.value_, right.value_) };
    } else if (expr::details::isVarVar(left, right)) {
        return makeBinaryFunction(type, left.index_, right.index_);
    } else if (expr::details::isConstVar(left, right)) {
        return makeBinaryFunction(type, left.value_, right.index_);
    } else if (expr::details::isVarConst(left, right)) {
        return makeBinaryFunction(type, left.index_, right.value_);
    } else if (expr::details::isConstFunc(left, right)) {
        return makeBinaryFunction(type, left.value_, right.f_);
    } else if (expr::details::isFuncConst(left, right)) {
        return makeBinaryFunction(type, left.f_, right.value_);
    } else if (expr::details::isVarFunc(left, right)) {
        return makeBinaryFunction(type, left.index_, right.f_);
    } else if (expr::details::isFuncVar(left, right)) {
        return makeBinaryFunction(type, left.f_, right.index_);
    }
    return makeBinaryFunction(type, left.f_, right.f_);
}


spider::expr::Token
spider::Expression::generate(RPNOperatorType type,
                             const expr::Token &arg0,
                             const expr::Token &arg1,
                             const expr::Token &arg2) const {
    if (expr::details::isConstConst(arg0, arg1) && expr::Token::CONSTANT == arg2.type()) {
        return { numeric::apply(type, arg0.value_, arg1.value_, arg2.value_) };
    }
    return { [=](const symbol_table_t &t) { return numeric::apply(type, arg0(t), arg1(t), arg2(t)); }};
}

template<class... Args>
spider::Expression::functor_t
spider::Expression::makeUnaryFunction(RPNOperatorType type, Args &&... args) const {
    switch (type) {
        case RPNOperatorType::FACT:
            return function<numeric::details::fact>(std::forward<Args>(args)...);
        case RPNOperatorType::COS:
            return function<numeric::details::cos>(std::forward<Args>(args)...);
        case RPNOperatorType::SIN:
            return function<numeric::details::sin>(std::forward<Args>(args)...);
        case RPNOperatorType::TAN:
            return function<numeric::details::tan>(std::forward<Args>(args)...);
        case RPNOperatorType::COSH:
            return function<numeric::details::cosh>(std::forward<Args>(args)...);
        case RPNOperatorType::SINH:
            return function<numeric::details::sinh>(std::forward<Args>(args)...);
        case RPNOperatorType::TANH:
            return function<numeric::details::tanh>(std::forward<Args>(args)...);
        case RPNOperatorType::EXP:
            return function<numeric::details::exp>(std::forward<Args>(args)...);
        case RPNOperatorType::LOG:
            return function<numeric::details::log>(std::forward<Args>(args)...);
        case RPNOperatorType::LOG2:
            return function<numeric::details::log2>(std::forward<Args>(args)...);
        case RPNOperatorType::LOG10:
            return function<numeric::details::log10>(std::forward<Args>(args)...);
        case RPNOperatorType::CEIL:
            return function<numeric::details::ceil>(std::forward<Args>(args)...);
        case RPNOperatorType::FLOOR:
            return function<numeric::details::floor>(std::forward<Args>(args)...);
        case RPNOperatorType::ABS:
            return function<numeric::details::abs>(std::forward<Args>(args)...);
        case RPNOperatorType::SQRT:
            return function<numeric::details::sqrt>(std::forward<Args>(args)...);
        default:
            if (log::enabled<log::EXPR>()) {
                log::warning<log::EXPR>("Invalid operation.");
            }
            return spider::Expression::functor_t();
    }
}

template<class Operation>
spider::Expression::functor_t spider::Expression::function(size_t v) const {
    return [=](const symbol_table_t &t) { return Operation::apply(t[v].second); };
}

template<class Operation>
spider::Expression::functor_t spider::Expression::function(const functor_t &f) const {
    return [=](const symbol_table_t &t) { return Operation::apply(f(t)); };
}

template<class... Args>
spider::Expression::functor_t
spider::Expression::makeBinaryFunction(RPNOperatorType type, Args &&... args) const {
    switch (type) {
        case RPNOperatorType::ADD:
            return function<numeric::details::add>(std::forward<Args>(args)...);
        case RPNOperatorType::MUL:
            return function<numeric::details::mul>(std::forward<Args>(args)...);
        case RPNOperatorType::SUB:
            return function<numeric::details::sub>(std::forward<Args>(args)...);
        case RPNOperatorType::DIV:
            return function<numeric::details::div>(std::forward<Args>(args)...);
        case RPNOperatorType::MOD:
            return function<numeric::details::mod>(std::forward<Args>(args)...);
        case RPNOperatorType::POW:
            return function<numeric::details::pow>(std::forward<Args>(args)...);
        case RPNOperatorType::MAX:
            return function<numeric::details::max>(std::forward<Args>(args)...);
        case RPNOperatorType::MIN:
            return function<numeric::details::min>(std::forward<Args>(args)...);
        case RPNOperatorType::LOG_AND:
            return function<numeric::details::land>(std::forward<Args>(args)...);
        case RPNOperatorType::LOG_OR:
            return function<numeric::details::lor>(std::forward<Args>(args)...);
        case RPNOperatorType::GREATER:
            return function<numeric::details::gt>(std::forward<Args>(args)...);
        case RPNOperatorType::GEQ:
            return function<numeric::details::gte>(std::forward<Args>(args)...);
        case RPNOperatorType::LESS:
            return function<numeric::details::lt>(std::forward<Args>(args)...);
        case RPNOperatorType::LEQ:
            return function<numeric::details::lte>(std::forward<Args>(args)...);
        default:
            if (log::enabled<log::EXPR>()) {
                log::warning<log::EXPR>("Invalid operation.");
            }
            return spider::Expression::functor_t();
    }
}

template<class Operation>
spider::Expression::functor_t spider::Expression::function(size_t v0, size_t v1) const {
    return [=](const symbol_table_t &t) { return Operation::apply(t[v0].second, t[v1].second); };
}

template<class Operation>
spider::Expression::functor_t spider::Expression::function(double c, size_t v) const {
    return [=](const symbol_table_t &t) { return Operation::apply(c, t[v].second); };
}

template<class Operation>
spider::Expression::functor_t spider::Expression::function(size_t v, double c) const {
    return [=](const symbol_table_t &t) { return Operation::apply(t[v].second, c); };
}

template<class Operation>
spider::Expression::functor_t spider::Expression::function(size_t v, const functor_t &f) const {
    return [=](const symbol_table_t &t) { return Operation::apply(t[v].second, f(t)); };
}

template<class Operation>
spider::Expression::functor_t spider::Expression::function(const functor_t &f, size_t v) const {
    return [=](const symbol_table_t &t) { return Operation::apply(f(t), t[v].second); };
}

template<class Operation>
spider::Expression::functor_t spider::Expression::function(double c, const functor_t &f) const {
    return [=](const symbol_table_t &t) { return Operation::apply(c, f(t)); };
}

template<class Operation>
spider::Expression::functor_t spider::Expression::function(const functor_t &f, double c) const {
    return [=](const symbol_table_t &t) { return Operation::apply(f(t), c); };
}

template<class Operation>
spider::Expression::functor_t
spider::Expression::function(const functor_t &f0, const functor_t &f1) const {
    return [=](const symbol_table_t &t) { return Operation::apply(f0(t), f1(t)); };
}

#endif
