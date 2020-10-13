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
#ifndef SPIDER2_DELAY_H
#define SPIDER2_DELAY_H

/* === Include(s) === */

#include <string>
#include <graphs-tools/expression-parser/Expression.h>

namespace spider {

    /* === Forward declaration(s) === */

    class MemoryInterface;

    namespace pisdf {

        class Edge;

        /* === Class definition === */

        class Delay {
        public:

            Delay(int64_t value,
                  Edge *edge,
                  Vertex *setter = nullptr, size_t setterPortIx = 0, Expression setterRateExpression = Expression{ },
                  Vertex *getter = nullptr, size_t getterPortIx = 0, Expression getterRateExpression = Expression{ },
                  bool persistent = true);

            ~Delay();

            Delay(Delay &&) = default;

            Delay(const Delay &) = delete;

            Delay &operator=(Delay &&) = default;

            Delay &operator=(const Delay &) = delete;

            /* === Method(s) === */

            /**
             * @brief Get the edge of the delay.
             * @return @refitem Spider::PiSDF::Edge associated to the delay.
             */
            std::string name() const;

            /* === Getter(s) === */

            inline Edge *edge() const { return edge_; }

            /**
             * @brief Get the setter vertex of the delay.
             * @return @refitem Spider::PiSDF::Vertex connected to the delay.
             */
            const Vertex *setter() const;

            /**
             * @brief Get the getter vertex of the delay.
             * @return @refitem Spider::PiSDF::Vertex connected to the delay.
             */
            const Vertex *getter() const;

            /**
             * @brief Return the port ix on which the delay is connected to the setter.
             * @return setter output port ix.
             */
            inline size_t setterPortIx() const { return setterPortIx_; }

            /**
             * @brief Get the setter rate of this delay.
             * @param params Vector of parameters to use for the evaluation of the expression.
             * @return value of the setter rate.
             */
            int64_t setterRate(const spider::vector<std::shared_ptr<Param>> &params = { }) const;

            /**
             * @brief Return the port ix on which the delay is connected to the getter.
             * @return getter output port ix.
             */
            inline size_t getterPortIx() const { return getterPortIx_; }

            /**
             * @brief Get the getter rate of this delay.
             * @param params Vector of parameters to use for the evaluation of the expression.
             * @return value of the getter rate.
             */
            int64_t getterRate(const spider::vector<std::shared_ptr<Param>> &params = { }) const;

            /**
             * @brief Get the virtual vertex associated to the Delay.
             * @return @refitem ExecVertex.
             */
            inline const Vertex *vertex() const { return vertex_; }

            /**
             * @brief Get the virtual memory address (in the data memory space) of the delay.
             * @return virtual memory address value.
             */
            inline uint64_t memoryAddress() const { return memoryAddress_; }

            /**
             * @brief Return the value of the delay. Calls @refitem Expression::evaluate method.
             * @return value of the delay.
             * @warning If value of the delay is set by dynamic parameter, it is user responsability to ensure proper
             * order of call.
             */
            int64_t value() const;

            inline bool isPersistent() const { return persistent_; }

            /* === Setter(s) === */

            /**
             * @brief Set the virtual memory address of the delay.
             * @param address
             * @remark Issue a warning if delay already has an address.
             */
            void setMemoryAddress(uint64_t address);

            /**
             * @brief Set the memory interface on which memory has been allocated (for persistent delays)
             * @param interface Pointer to the interface.
             */
            void setMemoryInterface(MemoryInterface *interface);

        private:
            uint64_t memoryAddress_{ UINT64_MAX }; /* = Memory address associated to this Delay (if persistent) = */
            int64_t value_{ 0 };                   /* = Value of the Delay = */
            MemoryInterface *memoryInterface_{ nullptr }; /* = Memory interface on which the delay is allocated = */
            Edge *edge_{ nullptr };                /* = Edge associated to the Delay = */
            Vertex *vertex_{ nullptr };            /* = Virtual vertex created for consistency evaluation = */
            size_t setterPortIx_{ 0 };             /* = Input port ix of the setter connected to the Delay = */
            size_t getterPortIx_{ 0 };             /* = Ouput port ix of the getter connected to the Delay = */
            bool persistent_{ true };              /* = Persistence property of the Delay = */
        };
    }
}

#endif //SPIDER2_DELAY_H
