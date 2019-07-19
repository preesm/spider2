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
#include <common/containers/Set.h>
#include "PiSDFTypes.h"

/* === Forward definition === */

class PiSDFGraph;

class PiSDFEdge;

/* === Class definition === */

class PiSDFInterface : public Spider::SetElement {
public:
    PiSDFInterface(PiSDFGraph *graph,
                   std::string name,
                   PiSDFInterfaceType type,
                   std::uint32_t ix,
                   PiSDFEdge *inputEdge,
                   PiSDFEdge *outputEdge);

    ~PiSDFInterface() = default;

    /* === Methods === */

    void exportDot(FILE *file, const Spider::string &offset) const;

    /* === Getters === */

    inline std::uint32_t ix() const;

    inline const std::string &name() const;

    inline PiSDFInterfaceType type() const;

    inline const PiSDFEdge *inputEdge() const;

    inline const PiSDFEdge *outputEdge() const;

    inline const PiSDFGraph *containingGraph() const;

    /* === Setters === */

    inline void setInputEdge(PiSDFEdge *edge);

    inline void setOutputEdge(PiSDFEdge *edge);

private:
    PiSDFGraph *graph_;
    PiSDFInterfaceType type_;
    std::uint32_t ix_;
    PiSDFEdge *inputEdge_ = nullptr;
    PiSDFEdge *outputEdge_ = nullptr;
    std::string name_;
};


std::uint32_t PiSDFInterface::ix() const {
    return ix_;
}

const std::string &PiSDFInterface::name() const {
    return name_;
}

PiSDFInterfaceType PiSDFInterface::type() const {
    return type_;
}

const PiSDFEdge *PiSDFInterface::inputEdge() const {
    return inputEdge_;
}

const PiSDFEdge *PiSDFInterface::outputEdge() const {
    return outputEdge_;
}

void PiSDFInterface::setInputEdge(PiSDFEdge *edge) {
    inputEdge_ = edge;
}

void PiSDFInterface::setOutputEdge(PiSDFEdge *edge) {
    outputEdge_ = edge;
}

const PiSDFGraph *PiSDFInterface::containingGraph() const {
    return graph_;
}

#endif //SPIDER2_PISDFINTERFACE_H
