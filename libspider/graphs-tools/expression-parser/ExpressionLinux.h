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
#ifndef SPIDER2_EXPRESSIONLINUX_H
#define SPIDER2_EXPRESSIONLINUX_H

#ifdef __linux__

/* === Include(s) === */

#include <memory/memory.h>
#include <graphs-tools/expression-parser/RPNConverter.h>
#include <graphs-tools/expression-parser/helper/ExpressionToken.h>
#include <graphs-tools/expression-parser/helper/CompiledExpression.h>

namespace spider {

    namespace pisdf {
        class Param;
    }

    /* === Class definition === */

    class Expression {
    public:

        Expression() = default;

        Expression(const Expression &) = default;

        Expression(Expression &&) = default;

        ~Expression() = default;

        explicit Expression(int64_t value);

        explicit Expression(std::string expression,
                            const spider::vector<std::shared_ptr<pisdf::Param>> &params = { });

        /* === Operator(s) === */

        Expression &operator=(const Expression &) = default;

        Expression &operator=(Expression &&) = default;

        inline bool operator==(const Expression &rhs) const {
            if (expr_) {
                return rhs.expr_ && ((*expr_.get()) == (*rhs.expr_.get()));
            }
            return !rhs.expr_ && (value_ == rhs.value_);
        }

        inline bool operator!=(const Expression &rhs) const  { return !(*this == rhs); }

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
            return dynamic() ? expr_->evaluate(params) : value_;
        }

        /* === Getter(s) === */

        /**
         * @brief Get the last evaluated value (faster than evaluated on static expressions)
         * @return last evaluated value (default value, i.e no evaluation done, is 0)
         */
        inline int64_t value() const { return static_cast<int64_t>(value_); }

        /**
         * @brief Get the static property of the expression.
         * @return true if the expression is static, false else.
         */
        inline bool dynamic() const { return expr_.operator bool(); }

        /* === Setter(s) === */

    private:
        using param_t = pisdf::Param *;
        using param_table_t = spider::vector<std::shared_ptr<pisdf::Param>>;
        using functor_t = double (*)(const double *);

        /* === Private member(s) === */
        std::shared_ptr<expr::CompiledExpression> expr_;
        double value_{ };

        /* === Private method(s) === */

        double evaluateStatic(spider::vector<RPNElement>::const_reverse_iterator &iterator,
                              const spider::vector<RPNElement>::const_reverse_iterator end,
                              const param_table_t &params);

        static param_t findParameter(const param_table_t &params, const std::string &name);
    };
}
#endif
#endif //SPIDER2_EXPRESSION_H
