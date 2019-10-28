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

namespace Spider {
    namespace PiSDF {

        /* === Forward declaration(s) === */

        class Vertex;

        class Edge;

        /* === Class definition === */

        class Delay {
        public:

            Delay(Expression &&expression,
                  Edge *edge,
                  Vertex *setter,
                  std::uint32_t setterPortIx,
                  Vertex *getter,
                  std::uint32_t getterPortIx,
                  bool persistent = false);

            ~Delay() = default;

            /* === Method(s) === */

            /**
             * @brief Get the edge of the delay.
             * @return @refitem Spider::PiSDF::Edge associated to the delay.
             */
            std::string name() const;

            /* === Getter(s) === */

            inline Edge *edge() const;

            /**
             * @brief Get the setter vertex of the delay.
             * @return @refitem Spider::PiSDF::Vertex connected to the delay.
             */
            inline Vertex *setter() const;

            /**
             * @brief Get the getter vertex of the delay.
             * @return @refitem Spider::PiSDF::Vertex connected to the delay.
             */
            inline Vertex *getter() const;

            /**
             * @brief Return the port ix on which the delay is connected to the setter.
             * @return setter output port ix.
             */
            inline std::uint32_t setterPortIx() const;

            /**
             * @brief Return the port ix on which the delay is connected to the getter.
             * @return getter output port ix.
             */
            inline std::uint32_t getterPortIx() const;

            /**
             * @brief Get the virtual memory address (in the data memory space) of the delay.
             * @return virtual memory address value.
             */
            inline std::uint64_t memoryAddress() const;

            /* === Setter(s) === */

            /**
             * @brief Set the virtual memory address of the delay.
             * @param address
             * @remark Issue a warning if delay already has an address.
             */
            inline void setMemoryAddress(std::uint64_t address);

        private:
            Expression expression_;
            Edge *edge_ = nullptr;
            Vertex *setter_ = nullptr;
            std::uint32_t setterPortIx_ = 0;
            Vertex *getter_ = nullptr;
            std::uint32_t getterPortIx_ = 0;

            bool persistent_ = true;
            std::uint64_t memoryAddress_ = UINT64_MAX;

            /* === Private method(s) === */
        };

        /* === Inline method(s) === */

        Edge *Delay::edge() const {
            return edge_;
        }

        Vertex *Delay::setter() const {
            return setter_;
        }

        Vertex *Delay::getter() const {
            return getter_;
        }

        std::uint32_t Delay::setterPortIx() const {
            return setterPortIx_;
        }

        std::uint32_t Delay::getterPortIx() const {
            return getterPortIx_;
        }

        std::uint64_t Delay::memoryAddress() const {
            return memoryAddress_;
        }

        void Delay::setMemoryAddress(std::uint64_t address) {
            if (memoryAddress_ != UINT64_MAX) {
                Spider::Logger::printWarning(LOG_GENERAL, "Delay already has a memory address.\n");
            }
            memoryAddress_ = address;
        }
    }
}

#endif //SPIDER2_DELAY_H
