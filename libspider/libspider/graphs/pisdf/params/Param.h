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
#ifndef SPIDER2_PARAM_H
#define SPIDER2_PARAM_H

/* === Include(s) === */

#include <common/Exception.h>
#include <graphs-tools/expression-parser/Expression.h>
#include <graphs/pisdf/visitors/Visitor.h>

namespace Spider {
    namespace PiSDF {

        /* === Forward declaration(s) === */

        class Graph;

        /* === Class definition === */

        class Param {
        public:

            Param(std::string name, Graph *graph, std::int64_t value = 0);

            Param(std::string name, Graph *graph, Expression &&expression);

            virtual ~Param() = default;

            /* === Method(s) === */

            inline virtual void visit(Visitor *visitor);

            /* === Getter(s) === */

            inline Graph *containingGraph() const;

            inline const std::string &name() const;

            inline std::uint32_t ix() const;

            virtual inline std::int64_t value() const;

            virtual inline ParamType type() const;

            virtual inline bool dynamic() const;

            /* === Setter(s) === */

            inline void setIx(std::uint32_t ix);

            virtual inline void setValue(std::int64_t);

            /**
             * @brief Set the graph of the param.
             * @remark This method changes current value.
             * @remark If graph is nullptr, nothing happen.
             * @param graph  Graph to set.
             */
            void setGraph(Graph *graph);

        protected:
            Graph *graph_ = nullptr;
            std::uint32_t ix_ = UINT32_MAX;
            std::string name_ = "";
            std::int64_t value_ = 0;

            /* === Private method(s) === */
        };

        /* === Inline method(s) === */

        void Param::visit(Visitor *visitor) {
            visitor->visit(this);
        }

        Graph *Param::containingGraph() const {
            return graph_;
        }

        const std::string &Param::name() const {
            return name_;
        }

        std::uint32_t Param::ix() const {
            return ix_;
        }

        std::int64_t Param::value() const {
            return value_;
        }

        ParamType Param::type() const {
            return ParamType::STATIC;
        }

        bool Param::dynamic() const {
            return false;
        }

        void Param::setIx(std::uint32_t ix) {
            ix_ = ix;
        }

        void Param::setValue(std::int64_t) {
            throwSpiderException("Can not set value on non-DYNAMIC parameter type.");
        }

    }
}


#endif //SPIDER2_PARAM_H
