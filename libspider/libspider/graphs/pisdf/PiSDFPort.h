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
#ifndef SPIDER2_PISDFPORT_H
#define SPIDER2_PISDFPORT_H

/* === Include(s) === */

#include <cstdint>
#include <string>
#include <graphs-tools/expression-parser/Expression.h>

/* === Forward declaration(s) === */

class PiSDFEdge;

/* === Class definition === */

class PiSDFPort {
public:

    PiSDFPort(PiSDFGraph *graph, const std::string &expression);

    explicit PiSDFPort(std::int64_t rate);

    ~PiSDFPort() = default;

    /* === Method(s) === */

    /**
     * @brief Rate of the port. Calls @refitem Expression::evaluate
     * @return rate value.
     * @throws @refitem SpiderException if port is not connected to an edge.
     */
    inline std::uint64_t rate() const;

    /**
     * @brief Disconnect the edge associated to the port.
     */
    inline void disconnectEdge();

    /* === Getter(s) === */

    /**
     * @brief Index of the port.
     * @return index value.
     */
    inline std::uint16_t ix() const;

    /**
     * @brief Get the edge connected to the port.
     * @return @refitem PiSDFEdge associated to the port, nullptr if no edge.
     */
    inline const PiSDFEdge *edge() const;

    /* === Setter(s) === */

    /**
     * @brief Connect an edge to the port.
     * @param edge Edge to set.
     * @param ix   Index of the port to set.
     * @throws @refitem SpiderException if port already has an edge or if edge is nullptr.
     */
    void connectEdge(PiSDFEdge *edge, std::uint16_t ix);

private:
    PiSDFEdge *edge_ = nullptr;
    std::uint16_t ix_ = UINT16_MAX;
    Expression expression_;
};

/* === Inline method(s) === */

std::uint64_t PiSDFPort::rate() const {
    if (!edge_) {
        throwSpiderException("Invalid rate evaluation: PiSDFPort not connected to an edge.");
    }
    return expression_.evaluate();
}

std::uint16_t PiSDFPort::ix() const {
    return ix_;
}

const PiSDFEdge *PiSDFPort::edge() const {
    return edge_;
}


#endif //SPIDER2_PISDFPORT_H
