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

namespace spider {
    namespace pisdf {

        /* === Class definition === */

        class Param final {
        public:

            explicit Param(std::string name) : expression_{ 0 }, type_{ ParamType::DYNAMIC } {
                setName(std::move(name));
            }

            explicit Param(std::string name, int64_t value) : expression_{ value }, type_{ ParamType::STATIC } {
                setName(std::move(name));
            }

            Param(std::string name, Expression expression) : expression_{ std::move(expression) } {
                setName(std::move(name));
                if (expression_.dynamic()) {
                    type_ = ParamType::DYNAMIC_DEPENDANT;
                } else {
                    type_ = ParamType::STATIC;
                }
            }

            Param(std::string name, std::shared_ptr<Param> parent) : parent_{ std::move(parent) },
                                                                     type_{ ParamType::INHERITED } {
                setName(std::move(name));
                if (!parent_) {
                    throwSpiderException("Inherited parameter can not have nullptr parent.");
                }
            }

            ~Param() = default;

            Param(const Param &) = default;

            Param(Param &&) noexcept = default;

            Param &operator=(const Param &) = default;

            Param &operator=(Param &&) = default;

            /* === Method(s) === */

            inline virtual void visit(Visitor *visitor) {
                visitor->visit(this);
            }

            /* === Getter(s) === */

            inline const std::string &name() const { return name_; }

            inline size_t ix() const { return ix_; }

            inline int64_t value() const {
                if (parent_) {
                    return parent_->value();
                }
                return expression_.value();
            }

            inline int64_t value(const vector <std::shared_ptr<Param>> &params) const {
                if (parent_) {
                    return parent_->value(params);
                }
                return expression_.evaluate(params);
            }

            inline ParamType type() const { return type_; }

            inline bool dynamic() const {
                if (parent_) {
                    return parent()->dynamic();
                }
                return (type_ == ParamType::DYNAMIC) || (type_ == ParamType::DYNAMIC_DEPENDANT);
            }

            inline Param *parent() const { return parent_.get(); }

            inline Expression expression() const { return expression_; }

            /* === Setter(s) === */

            inline void setIx(size_t ix) { ix_ = ix; }

            inline void setValue(int64_t value) {
                if (dynamic()) {
                    expression_ = Expression(value);
                } else {
                    throwSpiderException("Can not set value on non-DYNAMIC parameter type.");
                }
            }

        private:
            Expression expression_;          /* = Expression of the Param. = */
            std::string name_{
                    "" };                /* = Name of the Param. It is transformed to lower case on construction = */
            size_t ix_{ SIZE_MAX };               /* = Index of the Param in the Graph = */
            std::shared_ptr<Param> parent_;
            ParamType type_{ ParamType::STATIC }; /* = Type of the parameter = */

            /* === Private method(s) === */

            inline void setName(std::string name) {
                name_ = std::move(name);
                std::transform(std::begin(name_), std::end(name_), std::begin(name_),
                               [](char c) { return ::tolower(c); });
                if (name_ == "pi") {
                    throwSpiderException("ambiguous name for parameter: pi is a math constant.");
                }
            }
        };
    }
}
#endif //SPIDER2_PARAM_H
