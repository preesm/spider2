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

        /* === Forward declaration(s) === */

        class Graph;

        /* === Class definition === */

        class Param {
        public:

            explicit Param(std::string name, int64_t value) : Param(std::move(name)) {
                value_ = value;
            }

            Param(std::string name, const Expression &expression) : Param(std::move(name)) {
                if (expression.dynamic()) {
                    throwSpiderException("STATIC parameter [%s] should have static expression.", name_.c_str());
                }
                value_ = expression.value();
            }

            virtual ~Param() = default;

            Param(const Param &) = default;

            Param(Param &&) noexcept = default;

            Param &operator=(const Param &) = delete;

            Param &operator=(Param &&) = delete;

            /* === Method(s) === */

            inline virtual void visit(Visitor *visitor) {
                visitor->visit(this);
            }

            /* === Getter(s) === */

            inline Graph *graph() const {
                return graph_;
            }

            inline const std::string &name() const {
                return name_;
            }

            inline size_t ix() const {
                return ix_;
            }

            virtual inline int64_t value() const {
                return value_;
            }

            virtual inline int64_t value(const vector<std::shared_ptr<Param>> &) const {
                return value_;
            }

            virtual inline ParamType type() const {
                return ParamType::STATIC;
            }

            virtual inline bool dynamic() const {
                return false;
            }

            virtual inline Param *parent() const {
                return nullptr;
            }

            virtual inline Expression expression() const {
                return Expression(value_);
            }

            /* === Setter(s) === */

            inline void setIx(size_t ix) {
                ix_ = ix;
            }

            virtual inline void setValue(int64_t) {
                throwSpiderException("Can not set value on non-DYNAMIC parameter type.");
            }

            /**
             * @brief Set the graph of the param.
             * @remark This method changes current value.
             * @remark If graph is nullptr, nothing happen.
             * @param graph  Graph to set.
             */
            void setGraph(Graph *graph) {
                if (graph) {
                    graph_ = graph;
                }
            }

        protected:
            Graph *graph_ = nullptr;   /* = Containing Graph (can be nullptr) = */
            std::string name_ = "";    /* = Name of the Param. It is transformed to lower case on construction = */
            size_t ix_ = SIZE_MAX;     /* = Index of the Param in the Graph = */
            int64_t value_ = 0;        /* = Value of the Param. = */

            /* == Protected ctor == */
            explicit Param(std::string name) : name_{ std::move(name) } {
                std::transform(std::begin(name_), std::end(name_), std::begin(name_),
                               [](char c) { return static_cast<char>(::tolower(c)); });
                if (name_ == "pi") {
                    throwSpiderException("ambiguous name for parameter: pi is a math constant.");
                }
            }
        };
    }
}
#endif //SPIDER2_PARAM_H
