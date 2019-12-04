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
#ifndef SPIDER2_DELAY_H
#define SPIDER2_DELAY_H

/* === Include(s) === */

#include <string>
#include <graphs-tools/expression-parser/Expression.h>

namespace spider {
    namespace pisdf {

        /* === Forward declaration(s) === */

        class Edge;

        /* === Class definition === */

        class Delay {
        public:

            Delay(Expression &&expression,
                  Edge *edge,
                  ExecVertex *setter,
                  uint32_t setterPortIx,
                  Expression &&setterRateExpression,
                  ExecVertex *getter,
                  uint32_t getterPortIx,
                  Expression &&getterRateExpression,
                  bool persistent = false,
                  StackID stack = StackID::PISDF);

            ~Delay() = default;

            friend class Edge;

            /* === Method(s) === */

            /**
             * @brief Get the edge of the delay.
             * @return @refitem Spider::PiSDF::Edge associated to the delay.
             */
            std::string name() const;

            /* === Getter(s) === */

            inline Edge *edge() const {
                return edge_;
            }

            /**
             * @brief Get the setter vertex of the delay.
             * @return @refitem Spider::PiSDF::ExecVertex connected to the delay.
             */
            inline ExecVertex *setter() const {
                return setter_;
            }

            /**
             * @brief Get the getter vertex of the delay.
             * @return @refitem Spider::PiSDF::ExecVertex connected to the delay.
             */
            inline ExecVertex *getter() const {
                return getter_;
            }

            /**
             * @brief Return the port ix on which the delay is connected to the setter.
             * @return setter output port ix.
             */
            inline uint32_t setterPortIx() const {
                return setterPortIx_;
            }

            /**
             * @brief Return the port ix on which the delay is connected to the getter.
             * @return getter output port ix.
             */
            inline uint32_t getterPortIx() const {
                return getterPortIx_;
            }

            /**
             * @brief Get the virtual vertex associated to the Delay.
             * @return @refitem ExecVertex.
             */
            inline const DelayVertex *vertex() const {
                return vertex_;
            }

            /**
             * @brief Get the virtual memory address (in the data memory space) of the delay.
             * @return virtual memory address value.
             */
            inline uint64_t memoryAddress() const {
                return memoryAddress_;
            }

            /**
             * @brief Return the value of the delay. Calls @refitem Expression::evaluate method.
             * @remark by default, this method use the containing graph parameters of the delay to evaluate.
             * @return value of the delay.
             * @warning If value of the delay is set by dynamic parameter, it is user responsability to ensure proper
             * order of call.
             */
            int64_t value() const;

            /**
             * @brief Return the value of the delay. Calls @refitem Expression::evaluate method.
             * @param params Vector of parameters.
             * @return value of the delay.
             * @warning If value of the delay is set by dynamic parameter, it is user responsability to ensure proper
             * order of call.
             */
            int64_t value(const spider::vector<Param *> &params) const;

            inline bool isPersistent() const {
                return persistent_;
            }

            /* === Setter(s) === */

            /**
             * @brief Set the virtual memory address of the delay.
             * @param address
             * @remark Issue a warning if delay already has an address.
             */
            inline void setMemoryAddress(uint64_t address) {
                if (memoryAddress_ != UINT64_MAX && log_enabled()) {
                    spider::log::warning("Delay [%s] already has a memory address.\n", name().c_str());
                }
                memoryAddress_ = address;
            }

        private:
            Expression expression_;
            Edge *edge_ = nullptr;
            ExecVertex *setter_ = nullptr;
            uint32_t setterPortIx_ = 0;
            ExecVertex *getter_ = nullptr;
            uint32_t getterPortIx_ = 0;
            DelayVertex *vertex_ = nullptr;

            bool persistent_ = true;
            uint64_t memoryAddress_ = UINT64_MAX;
        };
    }
}

#endif //SPIDER2_DELAY_H
