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
#ifndef SPIDER2_EXPRESSIONWINDOWS_H
#define SPIDER2_EXPRESSIONWINDOWS_H

/* === Include(s) === */

#include <graphs-tools/expression-parser/RPNConverter.h>
#include <graphs-tools/expression-parser/helper/ExpressionToken.h>

namespace spider {

    namespace pisdf {
        class Param;
    }

    /* === Class definition === */

    class Expression {
    public:
        using symbol_t = std::pair<std::string, double>;
        using symbol_table_t = spider::vector<symbol_t>;
        using param_t = pisdf::Param *;
        using param_table_t = spider::vector<std::shared_ptr<pisdf::Param>>;
        using functor_t = std::function<double(const symbol_table_t &)>;

        Expression() = default;

        explicit Expression(int64_t value);

        explicit Expression(std::string expression, const param_table_t &params = { });

        Expression(const Expression &other);

        Expression(Expression &&other) noexcept: Expression() { swap(*this, other); };

        ~Expression();

        /* === Operator(s) === */

        inline friend void swap(Expression &lhs, Expression &rhs) noexcept {
            /* == Enable ADL == */
            using std::swap;
            /* == Swap members of both objects == */
            swap(lhs.symbolTable_, rhs.symbolTable_);
            swap(lhs.expr_, rhs.expr_);
        }

        inline Expression &operator=(Expression temp) {
            swap(*this, temp);
            return *this;
        }

        inline bool operator==(const Expression &rhs) const { return hash_ == rhs.hash_; }

        inline bool operator!=(const Expression &rhs) const { return !(*this == rhs); }

        /* === Method(s) === */

        /**
         * @brief Evaluate the expression and return the value and cast result in int64_.
         * @return Evaluated value of the expression.
         */
        inline int64_t evaluate(const spider::vector<std::shared_ptr<pisdf::Param>> &params = { }) const {
            return static_cast<int64_t>(evaluateDBL(params));
        }

        /**
         * @brief Evaluate the expression and return the value.
         * @return Evaluated value of the expression.
         */
        inline double evaluateDBL(const spider::vector<std::shared_ptr<pisdf::Param>> &params = { }) const {
            return dynamic() ? evaluateImpl(params) : expr_.value_;
        }

        /* === Getter(s) === */

        /**
         * @brief Get the last evaluated value (faster than evaluated on static expressions)
         * @return last evaluated value (default value, i.e no evaluation done, is 0)
         */
        inline int64_t value() const { return static_cast<int64_t>(expr_.value_); }

        /**
         * @brief Get the static property of the expression.
         * @return true if the expression is static, false else.
         */
        inline bool dynamic() const { return symbolTable_ != nullptr; }

        /* === Setter(s) === */

    private:

        /* === Private member(s) === */

        expr::Token expr_;
        mutable symbol_table_t *symbolTable_ = nullptr;
        size_t hash_{ SIZE_MAX };

        /* === Private method(s) === */

        static param_t findParameter(const param_table_t &params, const std::string &name);

        size_t registerSymbol(const param_t param);

        void updateSymbolTable(const param_table_t &params) const;

        double evaluateImpl(const spider::vector<std::shared_ptr<pisdf::Param>> &params = { }) const;

        void compile(const spider::vector<RPNElement> &postfixStack, const param_table_t &params);

        expr::Token compile(spider::vector<RPNElement>::const_reverse_iterator &iterator, const param_table_t &params);

        expr::Token generate(RPNOperatorType type, const expr::Token &arg) const;

        expr::Token generate(RPNOperatorType type, const expr::Token &left, const expr::Token &right) const;

        expr::Token
        generate(RPNOperatorType type, const expr::Token &arg0, const expr::Token &arg1, const expr::Token &arg2) const;

        template<class ...Args>
        functor_t makeUnaryFunction(RPNOperatorType type, Args &&...args) const;

        template<class Operation>
        functor_t function(size_t v) const;

        template<class Operation>
        functor_t function(const functor_t &v) const;

        template<class ...Args>
        functor_t makeBinaryFunction(RPNOperatorType type, Args &&...args) const;

        template<class Operation>
        functor_t function(size_t v0, size_t v1) const;

        template<class Operation>
        functor_t function(double c, size_t v) const;

        template<class Operation>
        functor_t function(size_t v, double c) const;

        template<class Operation>
        functor_t function(size_t v, const functor_t &f) const;

        template<class Operation>
        functor_t function(const functor_t &f, size_t v) const;

        template<class Operation>
        functor_t function(double c, const functor_t &f) const;

        template<class Operation>
        functor_t function(const functor_t &f, double c) const;

        template<class Operation>
        functor_t function(const functor_t &f0, const functor_t &f1) const;

    };
}

#endif //SPIDER2_EXPRESSIONWINDOWS_H
