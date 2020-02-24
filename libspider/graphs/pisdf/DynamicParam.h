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
#ifndef SPIDER2_DYNAMICPARAM_H
#define SPIDER2_DYNAMICPARAM_H

/* === Include(s) === */

#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Graph.h>

namespace spider {
    namespace pisdf {

        /* ==+ Forward declaration(s) === */

        class Vertex;

        /* === Class definition === */

        class DynamicParam final : public Param {
        public:

            explicit DynamicParam(std::string name) : Param(std::move(name)) {
                value_ = 0;
            }

            DynamicParam(std::string name, Expression expression) : Param(std::move(name)) {
                expression_ = make<Expression, StackID::PISDF>(std::move(expression));
            }

            DynamicParam(const DynamicParam &other) : Param(other) {
                expression_ = make<Expression, StackID::PISDF>(other.expression());
            }

            DynamicParam(DynamicParam &&other) noexcept : Param(std::move(other)) {
                using std::swap;
                swap(expression_, other.expression_);
            }

            ~DynamicParam() override {
                destroy(expression_);
            }

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override {
                visitor->visit(this);
            }

            /* === Getter(s) === */

            inline int64_t value() const override {
                return expression_ ? expression_->evaluate() : value_;
            }

            inline int64_t value(const spider::vector<std::shared_ptr<Param>> &params) const override {
                return expression_ ? expression_->evaluate(params) : value_;
            }

            inline ParamType type() const override {
                return ParamType::DYNAMIC;
            }

            inline bool dynamic() const override {
                return true;
            }

            inline Expression expression() const {
                return expression_ ? (*expression_) : Expression(value_);
            }

            /* === Setter(s) === */

            inline void setValue(int64_t value) override {
                value_ = value;
                destroy(expression_);
            }

        private:
            Expression *expression_ = nullptr; /* = Expression of the value of the Param (can be parameterized) = */
        };
    }
}

#endif //SPIDER2_DYNAMICPARAM_H
