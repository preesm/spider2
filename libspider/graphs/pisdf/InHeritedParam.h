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
#ifndef SPIDER2_INHERITEDPARAM_H
#define SPIDER2_INHERITEDPARAM_H

/* === Include(s) === */

#include <graphs/pisdf/Param.h>

namespace spider {
    namespace pisdf {

        /* === Class definition === */

        class InHeritedParam final : public Param {
        public:

            InHeritedParam(std::string name, Param *parent) : Param(std::move(name)),
                                                              parent_{ parent } {
                if (!parent) {
                    throwSpiderException("Inherited parameter can not have nullptr parent.");
                }
            }

            ~InHeritedParam() override = default;

            InHeritedParam(const InHeritedParam &other) : Param(other) {
                parent_ = other.parent_;
            }

            InHeritedParam(InHeritedParam &&other) noexcept: Param(std::move(other)) {
                std::swap(parent_, other.parent_);
            }

            /* === Method(s) === */

            inline void visit(Visitor *visitor) override {
                visitor->visit(this);
            }

            /* === Getter(s) === */

            inline int64_t value() const override {
                return parent_->value();
            }

            inline int64_t value(const spider::vector<std::shared_ptr<Param>> &params) const override {
                return parent_->value(params);
            }

            inline ParamType type() const override {
                return ParamType::INHERITED;
            }

            inline bool dynamic() const override {
                return parent_->dynamic();
            }

            inline Param *parent() const override {
                return parent_;
            }

            /* === Setter(s) === */

        private:
            Param *parent_ = nullptr; /* = Pointer to the corresponding parameter in the upper Graph = */
        };
    }
}


#endif //SPIDER2_INHERITEDPARAM_H
