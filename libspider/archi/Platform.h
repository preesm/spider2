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
#include <containers/array.h>
#include <containers/containers.h>
#include <api/archi-api.h>

namespace spider {

    /* === Forward declaration(s) === */

    class Cluster;

    class PE;

    class MemoryUnit;

    /* === Class definition === */

    class Platform {
    public:
        using InterMemoryInterface = std::pair<MemoryInterface *, MemoryInterface *>;

        Platform(size_t clusterCount, size_t peCount);

        ~Platform();

        /* === Method(s) === */

        void addCluster(Cluster *cluster);

        /**
         * @brief Find a processing element in the platform from its virtual ix (S-LAM user ix).
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
        uint64_t dataCommunicationCostPEToPE(PE *peSrc, PE *peSnk, uint64_t dataSize);


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
         * @brief Get the total number of PE type in the platform (go through each cluster).
         * @remark This is equivalent to clusterCount() because all PE inside a cluster share the same PE type.
         * @return total number of PE type in the platform.
         */
        inline size_t PETypeCount() const {
            return clusterCount();
        }

        /**
         * @brief Get the total number of local runtimes in the platform (go through each cluster)
         * @return total number of @refitem ProcessingElement with the LRT_* @refitem Spider::PEType
         */
        size_t LRTCount() const;

        /**
         * @brief Get the pair of @refitem MemoryInterface between two clusters.
         * @remark The pair is returned with first being A write <-> read B and second B write <-> read A.
         * @param clusterA  Cluster A.
         * @param clusterB  Cluster B.
         * @return @refitem InterMemoryInterface
         * @throws std::out_of_range if not found.
         */
        InterMemoryInterface getClusterToClusterMemoryInterface(Cluster *clusterA, Cluster *clusterB);

        /* === Setter(s) === */

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
         * @brief Set the pair of @refitem MemoryInterface between two cluster.
         * @remark This will overwrite current value.
         * @param clusterA   Cluster A.
         * @param clusterB   Cluster B.
         * @param interface  Pair of interfaces.
         * @throws std::out_of_range if out of bound
         */
        void setClusterToClusterMemoryInterface(Cluster *clusterA, Cluster *clusterB, InterMemoryInterface interface);

    private:
        spider::array<Cluster *> clusterArray_;
        spider::array<PE *> peArray_;
        spider::array<InterMemoryInterface> cluster2ClusterMemoryIF_;
        spider::array<size_t> preComputedClusterIx_;
        size_t clusterCount_ = 0;
        size_t peCount_ = 0;
        PE *grt_ = nullptr;

        /* === Private method(s) === */

        size_t getCluster2ClusterIndex(size_t ixA, size_t ixB) const;
    };

}
#endif //SPIDER2_PLATFORM_H
