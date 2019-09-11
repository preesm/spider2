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
#ifndef SPIDER2_PLATFORM_H
#define SPIDER2_PLATFORM_H

/* === Include(s) === */

#include <cstdint>
#include <containers/Array.h>
#include <containers/StlContainers.h>

/* === Forward declaration(s) === */

class Cluster;

class ProcessingElement;

class MemoryUnit;

/* === Class definition === */

class Platform {
public:

    explicit Platform(std::uint32_t clusterCount);

    ~Platform() = default;

    /* === Method(s) === */

    void addCluster(Cluster *cluster);

    /**
     * @brief Find a processing element in the platform from its name.
     * @param name  Name of the PE to find.
     * @return reference to the @refitem ProcessingElement found.
     * @throws Spider::Exception if there is no PE with given name in the platform.
     */
    ProcessingElement &findPE(const std::string &name) const;

    /**
     * @brief Find a processing element in the platform from its virtual ix (S-LAM user ix).
     * @param virtualIx  Virtual ix of the PE to find.
     * @return reference to the @refitem ProcessingElement found.
     * @throws Spider::Exception if there is no PE with given virtual ix in the platform.
     */
    ProcessingElement &findPE(std::uint32_t virtualIx) const;

    /**
     * @brief Find the processing element in the platform from its cluster ix and its PE ix.
     * @param clusterIx   Ix of the cluster of the PE.
     * @param PEIx        Ix of the PE inside the cluster.
     * @return reference to the @refitem ProcessingElement found.
     * @throws Spider::Exception if there is no PE with given cluster and PE ix in the platform.
     */
    ProcessingElement &findPE(std::uint32_t clusterIx, std::uint32_t PEIx) const;

    /**
     * @brief Activate a processing element.
     * @param pe Pointer to the PE to be enabled.
     */
    inline void enablePE(ProcessingElement *PE) const;

    /**
     * @brief Deactivate a processing element.
     * @param pe Pointer to the PE to be disabled.
     * @throws Spider::Exception if PE is the GRT.
     */
    inline void disablePE(ProcessingElement *PE) const;

    /* === Getter(s) === */

    /**
     * @brief Get the processing element on which the GRT runs (in master-slave mode).
     * @return pointer to the @refitem ProcessingElement of the GRT, nullptr if no GRT is set.
     */
    inline ProcessingElement *spiderGRTPE() const;

    /**
     * @brief Get the cluster ix of the GRT.
     * @return ix of the cluster of the GRT PE, -1 if no GRT is set.
     */
    std::int32_t spiderGRTClusterIx() const;

    /**
     * @brief Get the ix of the GRT PE inside its cluster.
     * @return ix of the PE inside its cluster, -1 if no GRT is set.
     */
    std::int32_t spiderGRTPEIx() const;

    /**
     * @brief Get the number of cluster in the platform.
     * @return Number of @refitem Cluster.
     */
    inline std::uint32_t clusterCount() const;

    /**
     * @brief Get the number of memory unit in the platform (equivalent to the cluster count)
     * @return Number of @refitem MemoryUnit
     */
    inline std::uint32_t memUnitCount() const;

    /**
     * @brief Get the total number of PE in the platform (go through each cluster)
     * @return total number of @refitem ProcessingElement in the platform.
     */
    std::uint32_t PECount() const;

    /**
     * @brief Get the total number of local runtimes in the platform (go though each cluster)
     * @return total number of @refitem ProcessingElement with the LRT_* @refitem Spider::PEType
     */
    std::uint32_t LRTCount() const;

    /* === Setter(s) === */

    /**
     * @brief Set the processing element of the GRT (in master-slave mode).
     * @remark This method replaces current GRT if it was set.
     * @param PE  Processing element to set as GRT.
     */
    inline void setSpiderGRTPE(ProcessingElement *PE);

private:
    Spider::Array<Cluster *> clusterArray_;
    std::uint32_t clusterCount_ = 0;
    ProcessingElement *grtPE_ = nullptr;

    /* === Private method(s) === */
};

/* === Inline method(s) === */

ProcessingElement *Platform::spiderGRTPE() const {
    return grtPE_;
}

std::uint32_t Platform::clusterCount() const {
    return clusterCount_;
}

std::uint32_t Platform::memUnitCount() const {
    return clusterCount_;
}

void Platform::setSpiderGRTPE(ProcessingElement *PE) {
    grtPE_ = PE;
}

#endif //SPIDER2_PLATFORM_H
