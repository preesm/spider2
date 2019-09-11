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
#ifndef SPIDER2_PROCESSINGELEMENT_H
#define SPIDER2_PROCESSINGELEMENT_H

/* === Include(s) === */

#include <cstdint>
#include <string>
#include <spider-api/archi.h>

/* === Forward declaration(s) === */

class Cluster;

/* === Class definition === */

class ProcessingElement {
public:

    ProcessingElement(std::uint32_t hwType,
                      std::uint32_t hwIx,
                      std::uint32_t virtIx,
                      Cluster *cluster,
                      std::string name = "unnamed-PE",
                      Spider::PEType spiderPEType = Spider::PEType::LRT_PE,
                      Spider::HWType spiderHWType = Spider::HWType::PHYS_PE);

    ~ProcessingElement() = default;

    /* === Method(s) === */

    /* === Getter(s) === */

    /**
     * @brief Hardware type of the PE.
     * @return hardware type.
     */
    inline std::uint32_t hardwareType() const;

    /**
     * @brief Hardware ix on which the PE runs.
     * @return hardward ix.
     */
    inline std::uint32_t hardwareIx() const;

    /**
     * @brief Ix set by the user in the architecture description of the platform.
     * @return virtual ix.
     */
    inline std::uint32_t virtualIx() const;

    /**
     * @brief Get the name of the processing element.
     * @return name of the processing element, "unnamed-pe" if no name was provided.
     */
    inline std::string name() const;

    /**
     * @brief Get the cluster associated to the processing element.
     * @return @refitem Cluster to which the PE belong.
     */
    inline Cluster *cluster() const;

    /**
     * @brief Get the type of PE (processing, LRT)
     * @return @refitem Spider::PEType of the PE.
     */
    inline Spider::PEType spiderPEType() const;

    /**
     * @brief Get the hw type of the PE (virtual or physical)
     * @return @refitem Spider::HWType of the PE.
     */
    inline Spider::HWType spiderHardwareType() const;

    /**
     * @brief Get the ix of the PE inside its cluster.
     * @return ix of the PE inside the cluster.
     */
    inline std::uint32_t clusterIx() const;

    /**
     * @brief Get the state of the PE.
     * @return true if the PE is enabled, false else.
     */
    inline bool enabled() const;

    /**
     * @brief Fetch the LRT property of the PE.
     * @return true if the PE is an LRT, false else.
     */
    inline bool isLRT() const;

    /* === Setter(s) === */

    /**
     * @brief Enable the PE.
     * @remark update the number of enabled PE in the associated Cluster.
     */
    void enable();

    /**
     * @brief Disable the PE.
     * @remark update the number of enabled PE in the associated Cluster.
     */
    void disable();

    inline void setClusterIx(std::uint32_t ix);

private:

    /* === Core properties === */

    std::uint32_t hwType_ = 0;        /* = S-LAM user hardware type = */
    std::uint32_t hwIx_ = 0;          /* = Hardware on which PE runs (core ix) = */
    std::uint32_t virtIx_ = 0;        /* = S-LAM user ix = */
    std::string name_ = "unnamed-pe"; /* = S-LAM user name of the PE = */

    /* === Spider properties === */

    Cluster *cluster_ = nullptr;  /* = Cluster to which the PE belong = */
    std::uint32_t clusterIx_ = 0; /* = Ix inside the cluster (used internally by spider) = */
    Spider::PEType spiderPEType_ = Spider::PEType::LRT_PE;
    Spider::HWType spiderHWType_ = Spider::HWType::PHYS_PE;
    bool enabled_ = false;

    /* === Private method(s) === */
};

/* === Inline method(s) === */

std::uint32_t ProcessingElement::hardwareType() const {
    return hwType_;
}

std::uint32_t ProcessingElement::hardwareIx() const {
    return hwIx_;
}

std::uint32_t ProcessingElement::virtualIx() const {
    return virtIx_;
}

std::string ProcessingElement::name() const {
    return name_;
}

Cluster *ProcessingElement::cluster() const {
    return cluster_;
}

Spider::PEType ProcessingElement::spiderPEType() const {
    return spiderPEType_;
}

Spider::HWType ProcessingElement::spiderHardwareType() const {
    return spiderHWType_;
}

std::uint32_t ProcessingElement::clusterIx() const {
    return clusterIx_;
}

bool ProcessingElement::enabled() const {
    return enabled_;
}

bool ProcessingElement::isLRT() const {
    return spiderPEType_ != Spider::PEType::PE_ONLY;
}

void ProcessingElement::setClusterIx(std::uint32_t ix) {
    clusterIx_ = ix;
}

#endif //SPIDER2_PROCESSINGELEMENT_H
