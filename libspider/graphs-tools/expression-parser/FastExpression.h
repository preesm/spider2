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
#ifndef SPIDER2_FASTEXPRESSION_H
#define SPIDER2_FASTEXPRESSION_H

/* === Include(s) === */

#include <string>
#include <memory/memory.h>
#include <containers/vector.h>
#include <containers/unordered_map.h>
#include <graphs-tools/expression-parser/RPNConverter.h>

namespace spider {

    namespace pisdf {
        class Param;
    }

    /* === Class definition === */

    class FastExpression {
    public:

        explicit FastExpression(std::string expression,
                                const spider::vector<std::shared_ptr<pisdf::Param>> &params = { });

        explicit FastExpression(int64_t value);

        FastExpression() = default;

        FastExpression(const FastExpression &other);

        FastExpression(FastExpression &&other) noexcept: FastExpression() {
            swap(*this, other);
        };

        ~FastExpression();

        /* === Operator(s) === */

        inline friend void swap(FastExpression &first, FastExpression &second) noexcept {
            /* == Enable ADL == */
            using std::swap;

            /* == Swap members of both objects == */
            swap(first.symbols_, second.symbols_);
            swap(first.stack_, second.stack_);
            swap(first.value_, second.value_);
        }

        inline FastExpression &operator=(FastExpression temp) {
            swap(*this, temp);
            return *this;
        }

        inline bool operator==(const FastExpression &rhs) const {
            return hash_ == rhs.hash_;
        }

        inline bool operator!=(const FastExpression &rhs) const {
            return !(*this == rhs);
        }

        FastExpression &operator+=(const FastExpression &rhs);

        /* === Method(s) === */

        /**
         * @brief Evaluate the expression and return the value and cast result in int64_.
         * @return Evaluated value of the expression.
         */
        inline int64_t evaluate(const spider::vector<std::shared_ptr<pisdf::Param>> &params = { }) const {
            return dynamic() ? static_cast<int64_t>(evaluateImpl(params)) : value();
        }

        /**
         * @brief Evaluate the expression and return the value.
         * @return Evaluated value of the expression.
         */
        inline double evaluateDBL(const spider::vector<std::shared_ptr<pisdf::Param>> &params = { }) const {
            return dynamic() ? evaluateImpl(params) : value_;
        }

        /* === Getter(s) === */

        /**
         * @brief Get the last evaluated value (faster than evaluated on static expressions)
         * @return last evaluated value (default value, i.e no evaluation done, is 0)
         */
        inline int64_t value() const {
            return static_cast<int64_t>(value_);
        }

        /**
         * @brief Get the static property of the expression.
         * @return true if the expression is static, false else.
         */
        inline bool dynamic() const {
            return stack_ != nullptr;
        }

        /* === Setter(s) === */

    private:
        struct ExpressionNode {
            std::function<double()> func_;
            size_t lastArgIndex_;
            RPNElementSubType type_;

            ExpressionNode(const ExpressionNode &) = default;

            ExpressionNode &operator=(const ExpressionNode &) = default;

            ExpressionNode(ExpressionNode &&) = default;

            ExpressionNode &operator=(ExpressionNode &&) = default;

            ExpressionNode(std::function<double()> func, size_t index, RPNElementSubType type) :
                    func_{ std::move(func) }, lastArgIndex_{ index }, type_{ type } { }

            inline double operator()() const { return func_(); }
        };

        /* = Declaring stack_ and symbols_ has pointer allows for low memory overhead when expression is static == */
        mutable unordered_map<std::string, double> *symbols_ = nullptr;
        spider::vector<ExpressionNode> *stack_ = nullptr;
        double value_ = 0;
        size_t hash_ = SIZE_MAX; /* = used for equality test = */

        void compile(spider::vector<RPNElement> &postfixStack,
                     const spider::vector<std::shared_ptr<pisdf::Param>> &params);

        double evaluateImpl(const spider::vector<std::shared_ptr<pisdf::Param>> &params = { }) const;

        ExpressionNode createNode(RPNOperatorType operatorType, spider::vector<ExpressionNode> &stack) const;
    };
}

#endif //SPIDER2_FASTEXPRESSION_H
