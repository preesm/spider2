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
#ifndef SPIDER2_PISDFINTERFACE_H
#define SPIDER2_PISDFINTERFACE_H

/* === Includes === */

#include <cstdint>
#include <string>
#include <common/containers/StlContainers.h>
#include <graphs/pisdf/PiSDFVertex.h>
#include "PiSDFTypes.h"

/* === Forward definition === */

class PiSDFGraph;

class PiSDFEdge;

/* === Class definition === */

class PiSDFInterface : public PiSDFVertex {
public:
    PiSDFInterface(PiSDFGraph *graph,
                   std::string name,
                   PiSDFInterfaceType type);

    ~PiSDFInterface() = default;

    /* === Methods === */

    void exportDot(FILE *file, const std::string &offset) const;

    std::uint16_t correspondingPortIx() const;

    /* === Getters === */

    inline PiSDFInterfaceType interfaceType() const;

    inline const PiSDFEdge *inputEdge() const;

    inline const PiSDFEdge *outputEdge() const;

    /* === Setters === */

private:
    PiSDFInterfaceType interfaceType_;
};

PiSDFInterfaceType PiSDFInterface::interfaceType() const {
    return interfaceType_;
}

const PiSDFEdge *PiSDFInterface::inputEdge() const {
    return this->inputEdges()[0];
}

const PiSDFEdge *PiSDFInterface::outputEdge() const {
    return this->outputEdges()[0];
}

#endif //SPIDER2_PISDFINTERFACE_H
