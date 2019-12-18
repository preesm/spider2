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

/* === Include(s) === */

#include <archi/Platform.h>
#include <archi/MemoryInterface.h>
#include <archi/Cluster.h>
#include <archi/PE.h>
#include <numeric>

/* === Static variable(s) === */

static size_t getClusterMemoryInterfaceCount(size_t count) {
    return (count * (count - 1)) / 2;
}

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::Platform::Platform(size_t clusterCount, size_t peCount) :
        clusterArray_{ clusterCount, nullptr, StackID::ARCHI },
        peArray_{ peCount, nullptr, StackID::ARCHI },
        cluster2ClusterMemoryIF_{ getClusterMemoryInterfaceCount(clusterCount),
                                  InterMemoryInterface(),
                                  StackID::ARCHI },
        preComputedClusterIx_{ clusterCount, SIZE_MAX, StackID::ARCHI } {
    /* == Pre-compute the cluster to cluster ix == */
    /*
     * Index to find the pair of memory interface between two clusters.
     * Ex: 5 clusters, need of (5*(5-1))/2 = 10 pairs to store all the communications.
     * Let [ixA, ixB] be the index of the cluster we want the communication pair.
     *     [0,1] = [1,0] -> 0, [0,2] = [2,0] -> 1, ... , [1,2] = [2,1] -> 4, ... , [3,4] = [4,3] -> 9.
     *
     * with N = cluster count
     * Let's define a bucket as the set of index of pair associated with the minimum value of the cluster ix of a given pair.
     * In our example we have 4 buckets: [0:3] for 0, [4:6] for 1, [7:8] for 2 and [9,9] for 3.
     * (no bucket needed for ix=4 as it already appears in all others)
     * Let's define the bucket size of a given bucket i as |B(i)| = (N - (i + 1)).
     * Then we have for any cluster ix n:
     *                               n              i*(i+1)
     *   [start,end], with start = sum(N-i) = n*N - -------    ; end = start + |B(i)| - 1
     *                              i=1                2
     *
     * ==> To get the index of the pair, for any (ixA,ixB):
     *
     *                    j*(j + 1)
     *  index  = j * N - ----------- - (j + 1) + k
     *                        2
     *
     *  with j = min(ixA, ixB), k = max(ixA, ixB)
     *  (the -(j + 1) term is the offset of the selected bucket)
     */
    size_t j = 0;
    for (auto &ix : preComputedClusterIx_) {
        ix = j * clusterCount + ((j * (j + 1)) / 2) - (j + 1);
    }
}

spider::Platform::~Platform() {
    /* == Avoid dangling pointer and reset memory space associate to it == */
    for (auto &pe : peArray_) {
        pe = nullptr;
    }

    /* == Destroy the Cluster == */
    for (auto &cluster : clusterArray_) {
        destroy(cluster);
    }

    /* == Destroy the inter-cluster MemoryInterface == */
    for (auto &pair : cluster2ClusterMemoryIF_) {
        destroy(pair.first);
        destroy(pair.second);
    }
}

void spider::Platform::addCluster(Cluster *cluster) {
    clusterArray_.at(clusterCount_) = cluster;
    cluster->setIx(clusterCount_);
    clusterCount_++;

    /* == Add the PE to the proper place in the global array == */
    for (const auto &pe : cluster->array()) {
        setPE(pe);
    }
}

size_t spider::Platform::LRTCount() const {
    return std::accumulate(clusterArray_.begin(), clusterArray_.end(), static_cast<size_t>(0),
                           [&](size_t a, Cluster *cluster) -> size_t { return a + cluster->LRTCount(); });
}

spider::Platform::InterMemoryInterface
spider::Platform::getClusterToClusterMemoryInterface(spider::Cluster *clusterA, spider::Cluster *clusterB) {
    if (clusterA == clusterB) {
        return std::make_pair(clusterA->memoryInterface(), clusterA->memoryInterface());
    }
    const auto &index = getCluster2ClusterIndex(clusterA->ix(), clusterB->ix());
    const auto &interface = cluster2ClusterMemoryIF_.at(index);
    if (interface.first->memoryUnit() == clusterA->memoryUnit()) {
        return interface;
    }
    /* == We need to invert the order so that it can appear to be transparent to the user == */
    return std::make_pair(interface.second, interface.first);
}

void spider::Platform::setPE(spider::PE *pe) {
    if (pe) {
        peArray_.at(peCount_) = pe;
        pe->setVirtualIx(peCount_++);
    }
}

void spider::Platform::setClusterToClusterMemoryInterface(spider::Cluster *clusterA,
                                                          spider::Cluster *clusterB,
                                                          spider::Platform::InterMemoryInterface interface) {
    if (clusterA == clusterB) {
        return;
    }
    const auto &index = getCluster2ClusterIndex(clusterA->ix(), clusterB->ix());
    cluster2ClusterMemoryIF_.at(index) = std::move(interface);
}


uint64_t spider::Platform::dataCommunicationCostPEToPE(PE *peSrc, PE *peSnk, uint64_t dataSize) {
    /* == Get the interface between cluster if needed == */
    if (peSrc->cluster() != peSnk->cluster()) {
        /* == For inter cluster communication, cost is a bit more complicated to compute == */
        auto interComInterfaces = getClusterToClusterMemoryInterface(peSrc->cluster(), peSnk->cluster());
        auto *srcMemoryInterface = peSrc->cluster()->memoryInterface();
        /* == Total write cost is the sum of PEsrc -> srcMemoryInterface and ClusterSrc -> snkMemoryInterface == */
        const auto &writeCost = math::saturateAdd(srcMemoryInterface->writeCost(dataSize),
                                                  interComInterfaces.first->writeCost(dataSize));

        /* == Total read cost is the sum of srcMemoryInterface -> ClusterSnk and snkMemoryInterface -> PEsnk == */
        auto *snkMemoryInterface = peSnk->cluster()->memoryInterface();
        const auto &readCost = math::saturateAdd(interComInterfaces.second->readCost(dataSize),
                                                 snkMemoryInterface->readCost(dataSize));
        return math::saturateAdd(writeCost, readCost);
    }
    /* == Intra cluster communication == */
    auto *memoryInterface = peSnk->cluster()->memoryInterface();
    return math::saturateAdd(memoryInterface->readCost(dataSize), memoryInterface->writeCost(dataSize));
}


size_t spider::Platform::getCluster2ClusterIndex(size_t ixA, size_t ixB) const {
    return ixA > ixB ? (preComputedClusterIx_[ixB] + ixA) : (preComputedClusterIx_[ixA] + ixB);
}
