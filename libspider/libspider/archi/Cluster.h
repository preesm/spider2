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
#ifndef SPIDER2_CLUSTER_H
#define SPIDER2_CLUSTER_H

/* === Include(s) === */

#include <cstdint>
#include <containers/Array.h>
#include <containers/StlContainers.h>

/* === Forward declaration(s) === */

class Platform;

class ProcessingElement;

class MemoryUnit;

/* === Class definition === */

class Cluster {
public:

    Cluster(std::uint32_t PECount, MemoryUnit *memoryUnit, Platform *platform);

    ~Cluster();

    /* === Method(s) === */

    /**
     * @brief Add a processing element to the cluster.
     * @param PE Processing element to add.
     * @throws @refitem Spider::Exception if cluster is already full.
     */
    void addPE(ProcessingElement *PE);

    /**
     * @brief Set the state (enabled or disabled) of a processing element in the cluster.
     * @param ix      Ix of the processing element.
     * @param status  Status of the PE to set (true = enabled, false = disabled).
     * @throws Spider::Exception if PE ix is out of bound.
     */
    inline void setPEStatus(std::uint32_t ix, bool status);

    /* === Getter(s) === */

    /**
     * @brief Get the array of processing element of the cluster.
     * @return const reference to the @refitem Spider::Array of @refitem ProcessingElement of the cluster.
     */
    inline const Spider::Array<ProcessingElement *> &processingElements() const;

    /**
     * @brief Get the memory unit of the cluster.
     * @return reference to the @refitem MemoryUnit of the cluster.
     */
    inline MemoryUnit &memoryUnit() const;

    /**
     * @brief Get a given processing element from the cluster.
     * @param ix  Ix of the processing element in the cluster.
     * @return const reference of the @refitem ProcessingElement
     * @throws @refitem Spider::Exception if ix is out of bound
     */
    inline const ProcessingElement &processingElement(std::uint32_t ix) const;

    /**
     * @brief Get the number of processing element inside the cluster.
     * @return number of @refitem ProcessingElement inside the cluster.
     */
    inline std::uint32_t PECount() const;

    /**
     * @brief Get the number of local runtime in the cluster.
     * @return number of local runtime inside the cluster.
     */
    inline std::uint32_t LRTCount() const;

    /**
     * @brief  Get the cluster ix (unique among clusters).
     * @return Ix of the cluster.
     */
    inline std::uint32_t ix() const;

    /**
     * @brief Get the platform of the cluster.
     * @return @refitem Platform of the cluster.
     */
    inline Platform &platform() const;

    /**
     * @brief Get the number of processing element enabled in the cluster.
     * @return number of enabled PE.
     */
    inline std::uint32_t enabledPECount() const;

    /* === Setter(s) === */

    inline void setIx(std::uint32_t ix);

private:

    /* === Core properties === */

    Spider::Array<ProcessingElement *> PEArray_;
    std::vector<bool> PEEnabledVector_;
    Platform *platform_ = nullptr;
    MemoryUnit *memoryUnit_ = nullptr;
    std::uint32_t PECount_ = 0;

    /* === Spider properties === */

    std::uint32_t LRTCount_ = 0;
    std::uint32_t enabledPECount_ = 0;
    std::uint32_t ix_ = 0;

    /* === Private method(s) === */
};

/* === Inline method(s) === */

void Cluster::setPEStatus(std::uint32_t ix, bool status) {
    PEEnabledVector_.at(ix) = status;
    enabledPECount_ = std::count(PEEnabledVector_.begin(), PEEnabledVector_.end(), true);
}

const Spider::Array<ProcessingElement *> &Cluster::processingElements() const {
    return PEArray_;
}

MemoryUnit &Cluster::memoryUnit() const {
    return *memoryUnit_;
}

const ProcessingElement &Cluster::processingElement(std::uint32_t ix) const {
    return *PEArray_.at(ix);
}

std::uint32_t Cluster::PECount() const {
    return PECount_;
}

std::uint32_t Cluster::LRTCount() const {
    return LRTCount_;
}

std::uint32_t Cluster::ix() const {
    return ix_;
}

Platform &Cluster::platform() const {
    return *platform_;
}

void Cluster::setIx(std::uint32_t ix) {
    ix_ = ix;
}

std::uint32_t Cluster::enabledPECount() const {
    return enabledPECount_;
}

#endif //SPIDER2_CLUSTER_H
