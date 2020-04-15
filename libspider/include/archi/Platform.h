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
#ifndef SPIDER2_PLATFORM_H
#define SPIDER2_PLATFORM_H

/* === Include(s) === */

#include <cstdint>
#include <containers/array.h>
#include <containers/vector.h>
#include <api/archi-api.h>

namespace spider {

    /* === Forward declaration(s) === */

    class Cluster;

    class PE;

    class MemoryBus;

    class InterMemoryBus;

    /* === Class definition === */

    class Platform {
    public:

        Platform(size_t clusterCount, size_t peCount);

        ~Platform();

        /* === Method(s) === */

        /**
         * @brief Adds a Cluster to the platform.
         * @param cluster Pointer to the Cluster to add.
         */
        void addCluster(Cluster *cluster);

        /**
         * @brief Returns the processing element in the platform matching the virtual ix.
         * @param virtualIx  Virtual ix of the PE to find.
         * @return pointer to the @refitem PE found.
         * @throws std::out_of_range
         */
        inline PE *peFromVirtualIx(size_t virtualIx) const {
            return peArray_.at(virtualIx);
        }

        /**
         * @brief Compute the data communication cost between two processing elements.
         * @param peSrc      Processing Element sending the data.
         * @param peSnk      Processing Element receiving the data.
         * @param dataSize   Size (in byte) of the data to send / receive.
         * @return communication cost (UINT64_MAX if communication is not possible).
         */
        uint64_t dataCommunicationCostPEToPE(PE *peSrc, PE *peSnk, uint64_t dataSize) const;

        /**
         * @brief Register a new external buffer.
         * @param buffer Pointer to the buffer
         * @return index of the buffer if success, SIZE_MAX else.
         */
        size_t registerExternalBuffer(void *buffer);

        /* === Getter(s) === */

        /**
         * @brief Get the clusters of the platform.
         * @return const reference to the @refitem spider::array of clusters.
         */
        inline const spider::array<Cluster *> &clusters() const {
            return clusterArray_;
        }

        /**
         * @brief Get a specific cluster in the platform.
         * @param ix Ix of the cluster to get.
         * @return pointer to @refitem spider::Cluster.
         * @throws std::out_of_range if ix is out of bound.
         */
        inline Cluster *cluster(size_t ix) const {
            return clusterArray_.at(ix);
        }

        /**
         * @brief Get the processing element on which the GRT runs (in master-slave mode).
         * @return pointer to the @refitem ProcessingElement of the GRT, nullptr if no GRT is set.
         */
        inline PE *spiderGRTPE() const {
            return grt_;
        }

        /**
         * @brief Get the ix of the GRT.
         * @return ix of the GRT, SIZE_MAX if failed
         */
        size_t getGRTIx() const;

        /**
         * @brief Get the number of cluster in the platform.
         * @return Number of @refitem Cluster.
         */
        inline size_t clusterCount() const {
            return clusterArray_.size();
        }

        /**
         * @brief Get the total number of PE in the platform (go through each cluster)
         * @return total number of @refitem ProcessingElement in the platform.
         */
        inline size_t PECount() const {
            return peCount_;
        }

        /**
         * @brief Get the total number of Hardware type in the platform.
         * @remark this value should only be looked once the platform is finalized.
         * @return total number of HW type in the platform.
         */
        inline size_t HWTypeCount() const {
            return hwTypeCount_;
        }

        /**
         * @brief Get the total number of local runtimes in the platform (go through each cluster)
         * @return total number of @refitem ProcessingElement with the LRT_* @refitem Spider::PEType
         */
        size_t LRTCount() const;

        /**
         * @brief Get the @refitem MemoryBus between two clusters in the direction A to B.
         * @param clusterA  Pointer to cluster A.
         * @param clusterB  Pointer to cluster B.
         * @return @refitem MemoryBus if cluster A != cluster B, nullptr else.
         * @throws std::out_of_range if not found.
         */
        MemoryBus *getClusterToClusterMemoryBus(Cluster *clusterA, Cluster *clusterB) const;

        /**
         * @brief Returns the linear array of processing element (order is not guaranteed to respect Cluster order).
         * @return @refitem spider::array of pointer of @refitem PE.
         */
        inline const spider::array<PE *> &peArray() const {
            return peArray_;
        }

        /**
         * @brief Returns pointer to the processing element of index ix.
         * @param ix   Value of the index of the processing element.
         * @return pointer to @refitem PE.
         * @throw std::out_of_range if ix is invalid.
         */
        inline PE *processingElement(size_t ix) const {
            return peArray_.at(ix);
        }

        /**
         * @brief Returns vector of LRT.
         * @return const reference to the vector of local runtimes.
         */
        inline const spider::vector<PE *> &lrtVector() const {
            return lrtVector_;
        }

        /**
         * @brief Returns the external buffer associated with this index.
         * @param index Index of the buffer.
         * @return buffer;
         * @throws std::out_of_range if index is invalid.
         */
        inline void *getExternalBuffer(size_t index) const {
            return externBuffersVector_.at(index);
        }

        /* === Setter(s) === */

        /**
         * @brief Sets the PE in the global linear PE array.
         * @param pe  Pointer to the PE to add.
         * @param ix  Virtual ix of the PE.
         * @throws std::out_of_range
         */
        void setPE(PE *pe);

        /**
         * @brief Set the processing element of the GRT (in master-slave mode).
         * @remark This method replaces current GRT if it was set.
         * @param pe  Processing element to set as GRT.
         */
        inline void setSpiderGRTPE(PE *pe) {
            if (pe) {
                grt_ = pe;
            }
        }

        /**
         * @brief Set the @refitem InterMemoryBus between two cluster.
         * @remark This will overwrite current value.
         * @param clusterA   Pointer to cluster A.
         * @param clusterB   Pointer to cluster B.
         * @param bus        Pointer to the @refitem InterMemoryBus.
         * @throws std::out_of_range if out of bound
         */
        void setClusterToClusterMemoryBus(Cluster *clusterA, Cluster *clusterB, InterMemoryBus *bus);

    private:
        array<Cluster *> clusterArray_;                      /* = Array of Cluster in the Platform = */
        array<PE *> peArray_;                                /* = Array of PE in the Platform = */
        array<InterMemoryBus *> interClusterMemoryBusArray_; /* = Array of inter Cluster MemoryBus = */
        array<size_t> preComputedClusterIx_;                 /* = Array of pre-computed index value for fast inter Cluster communication = */
        vector<PE *> lrtVector_;                             /* = Vector of the LRT of the platform (does not hold any memory) = */
        vector<void *> externBuffersVector_;                 /* = Vector of external buffers = */
        size_t clusterCount_ = 0;                            /* = Number of currently added Cluster in the Platform = */
        size_t peCount_ = 0;                                 /* = Number of currently added PE in the Platform = */
        size_t hwTypeCount_ = 0;                             /* = Number of currently added PE Types in the Platform = */
        PE *grt_ = nullptr;                                  /* = Pointer to the PE used as Global Runtime = */

        /* === Private method(s) === */

        size_t getCluster2ClusterIndex(size_t ixA, size_t ixB) const;
    };

}
#endif //SPIDER2_PLATFORM_H
