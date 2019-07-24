/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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
#ifndef SPIDER2_PISDFDELAY_H
#define SPIDER2_PISDFDELAY_H

/* === Include(s) === */

#include <cstdint>
#include <string>
#include <graphs-tools/expression-parser/Expression.h>
#include <common/Logger.h>

/* === Forward declaration(s) === */

class PiSDFEdge;

class PiSDFVertex;

/* === Class definition === */

class PiSDFDelay {
public:

    PiSDFDelay(PiSDFEdge *edge,
               const std::string &expression,
               bool persistent = true,
               PiSDFVertex *setter = nullptr,
               PiSDFVertex *getter = nullptr);

    PiSDFDelay(PiSDFEdge *edge,
               std::int64_t value,
               bool persistent = true,
               PiSDFVertex *setter = nullptr,
               PiSDFVertex *getter = nullptr);

    /* === Method(s) === */

    /**
     * @brief Return the value of the delay. Calls @refitem Expression::evaluate method.
     * @return value of the delay.
     * @warning If value of the delay is set by dynamic parameter, it is user responsability to ensure proper
     * order of call.
     */
    inline std::int64_t value() const;

    /* === Getter(s) === */

    /**
     * @brief Get the edge of the delay.
     * @return @refitem PiSDFEdge associated to the delay.
     */
    inline const PiSDFEdge *edge() const;

    /**
     * @brief Get the setter vertex of the delay.
     * @return @refitem PiSDFVertex connected to the delay.
     */
    inline const PiSDFVertex *setter() const;

    /**
     * @brief Get the getter vertex of the delay.
     * @return @refitem PiSDFVertex connected to the delay.
     */
    inline const PiSDFVertex *getter() const;

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
    PiSDFEdge *edge_ = nullptr;
    PiSDFVertex *setter_ = nullptr;
    PiSDFVertex *getter_ = nullptr;
    Expression expression_;
    bool persistent_ = true;
    std::uint64_t memoryAddress_ = UINT64_MAX;

    /* === Private method(s) === */
};

/* === Inline method(s) === */

const PiSDFEdge *PiSDFDelay::edge() const {
    return edge_;
}

const PiSDFVertex *PiSDFDelay::setter() const {
    return setter_;
}

const PiSDFVertex *PiSDFDelay::getter() const {
    return getter_;
}

std::int64_t PiSDFDelay::value() const {
    return expression_.evaluate();
}

std::uint64_t PiSDFDelay::memoryAddress() const {
    return memoryAddress_;
}

void PiSDFDelay::setMemoryAddress(std::uint64_t address) {
    if (memoryAddress_ != UINT64_MAX) {
        Spider::Logger::print(LOG_GENERAL, LOG_WARNING, "Delay already has a memory address.\n");
    }
    memoryAddress_ = address;
}



#endif //SPIDER2_PISDFDELAY_H
