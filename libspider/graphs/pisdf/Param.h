/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
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
#ifndef SPIDER2_PARAM_H
#define SPIDER2_PARAM_H

/* === Include(s) === */

#include <common/Exception.h>
#include <graphs-tools/expression-parser/Expression.h>
#include <graphs-tools/helper/visitors/PiSDFVisitor.h>
#include <variant>

namespace spider {
    namespace pisdf {

        /* === Class definition === */

        class Param final {
        public:

            explicit Param(std::string name) : internal_{ 0 },
                                               type_{ ParamType::DYNAMIC } {
                setName(std::move(name));
            }

            explicit Param(std::string name, int64_t value) : internal_{ value },
                                                              type_{ ParamType::STATIC } {
                setName(std::move(name));
            }

            Param(std::string name, Expression expression) {
                setName(std::move(name));
                if (expression.dynamic()) {
                    type_ = ParamType::DYNAMIC_DEPENDANT;
                    internal_ = std::move(expression);
                } else {
                    type_ = ParamType::STATIC;
                    internal_ = expression.value();
                }
            }

            Param(std::string name, std::shared_ptr<Param> parent) : type_{ ParamType::INHERITED } {
                setName(std::move(name));
                if (!parent) {
                    throwSpiderException("Inherited parameter can not have nullptr parent.");
                }
                internal_ = std::move(parent);
            }

            ~Param() = default;

            Param(const Param &) = default;

            Param(Param &&) noexcept = default;

            inline Param &operator=(const Param &) = default;

            Param &operator=(Param &&) = default;

            /* === Method(s) === */

            inline void visit(Visitor *visitor) {
                visitor->visit(this);
            }

            /* === Getter(s) === */

            inline const std::string &name() const { return name_; }

            inline size_t ix() const { return ix_; }

            inline int64_t value() const {
                if (std::holds_alternative<param_t>(internal_)) {
                    return std::get<param_t>(internal_)->value();
                } else if (std::holds_alternative<Expression>(internal_)) {
                    return std::get<Expression>(internal_).value();
                }
                return std::get<int64_t>(internal_);
            }

            inline int64_t value(const vector <std::shared_ptr<Param>> &params) const {
                if (std::holds_alternative<param_t>(internal_)) {
                    return std::get<param_t>(internal_)->value();
                } else if (std::holds_alternative<Expression>(internal_)) {
                    return std::get<Expression>(internal_).evaluate(params);
                }
                return std::get<int64_t>(internal_);
            }

            inline ParamType type() const { return type_; }

            inline bool dynamic() const {
                if (std::holds_alternative<param_t>(internal_)) {
                    return std::get<param_t>(internal_)->dynamic();
                }
                return (type_ == ParamType::DYNAMIC) || (type_ == ParamType::DYNAMIC_DEPENDANT);
            }

            inline Param *parent() const {
                if (std::holds_alternative<param_t>(internal_)) {
                    return std::get<param_t>(internal_).get();
                }
                return nullptr;
            }

            inline Expression expression() const {
                if (std::holds_alternative<param_t>(internal_)) {
                    return std::get<param_t>(internal_)->expression();
                } else if (std::holds_alternative<Expression>(internal_)) {
                    return std::get<Expression>(internal_);
                }
                return Expression(std::get<int64_t>(internal_));
            }

            /* === Setter(s) === */

            inline void setIx(size_t ix) { ix_ = ix; }

            inline void setValue(int64_t value) {
                if (dynamic()) {
                    internal_ = value;
                } else {
                    throwSpiderException("Can not set value on non-DYNAMIC parameter type.");
                }
            }

        private:
            using param_t = std::shared_ptr<Param>;
            using type_t = std::variant<int64_t, Expression, param_t>;
            std::string name_;                    /* = Name of the Param. It is transformed to lower case on construction = */
            type_t internal_;                     /* = Internal storage of the parameter = */
            size_t ix_{ SIZE_MAX };               /* = Index of the Param in the Graph = */
            ParamType type_{ ParamType::STATIC }; /* = Type of the parameter = */

            /* === Private method(s) === */

            inline void setName(std::string name) {
                name_ = std::move(name);
                std::transform(std::begin(name_), std::end(name_), std::begin(name_),
                               [](char c) { return static_cast<char>(::tolower(static_cast<int>(c))); });
                if (name_ == "pi") {
                    throwSpiderException("ambiguous name for parameter: pi is a math constant.");
                }
            }
        };
    }
}
#endif //SPIDER2_PARAM_H
