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
#include <archi/Cluster.h>
#include <archi/ProcessingElement.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::Platform::Platform(uint32_t clusterCount) : clusterArray_{ clusterCount, StackID::ARCHI } {
}

spider::Platform::~Platform() {
    for (auto &cluster : clusterArray_) {
        spider::destruct(cluster);
        spider::deallocate(cluster);
    }
}

void spider::Platform::addCluster(Cluster *cluster) {
    clusterArray_.at(clusterCount_) = cluster;
    cluster->setIx(clusterCount_);
    clusterCount_ += 1;
}

spider::PE &spider::Platform::findPE(const std::string &name) const {
    for (const auto &cluster : clusterArray_) {
        for (auto &pe : cluster->processingElements()) {
            if (pe->name() == name) {
                return *pe;
            }
        }
    }
    throwSpiderException("Unable to find PE of name: %s in any of the platform clusters.", name.c_str());
}

spider::PE &spider::Platform::findPE(uint32_t virtualIx) const {
    for (const auto &cluster : clusterArray_) {
        for (auto &pe : cluster->processingElements()) {
            if (pe->virtualIx() == virtualIx) {
                return *pe;
            }
        }
    }
    throwSpiderException("Unable to find PE of s-lam ix: %"
                                 PRIu32
                                 " in any of the platform clusters.", virtualIx);
}

spider::PE &spider::Platform::findPE(uint32_t clusterIx, uint32_t PEIx) const {
    auto *cluster = clusterArray_.at(clusterIx);
    return *(cluster->processingElements().at(PEIx));
}

size_t spider::Platform::PECount() const {
    size_t PECount = 0;
    for (auto &cluster : clusterArray_) {
        PECount += cluster->PECount();
    }
    return PECount;
}

uint32_t spider::Platform::LRTCount() const {
    uint32_t LRTCount = 0;
    for (auto &cluster : clusterArray_) {
        LRTCount += cluster->LRTCount();
    }
    return LRTCount;
}

int32_t spider::Platform::spiderGRTClusterIx() const {
    if (grtPE_) {
        return static_cast<int32_t>(grtPE_->cluster()->ix());
    }
    return -1;
}

int32_t spider::Platform::spiderGRTPEIx() const {
    if (grtPE_) {
        return static_cast<int32_t>(grtPE_->clusterPEIx());
    }
    return -1;
}

void spider::Platform::enablePE(PE *const PE) const {
    if (PE) {
        PE->enable();
    }
}

void spider::Platform::disablePE(PE *const PE) const {
    if (PE && PE == grtPE_) {
        throwSpiderException("Can not disable GRT PE: %s.", PE->name().c_str());
    }
    if (PE) {
        PE->disable();
    }
}

uint64_t spider::Platform::dataCommunicationCostPEToPE(PE *PESrc,
                                                       PE *PESnk,
                                                       uint64_t dataSize) {
    /* == Test if it is an intra or inter cluster communication == */
    if (PESrc->cluster()->ix() == PESnk->cluster()->ix()) {
        /* == Intra cluster, communication cost is the read / write to the cluster memory cost == */
        auto *cluster = PESrc->cluster();
        return spider::math::saturateAdd(cluster->writeCostRoutine()(dataSize), cluster->readCostRoutine()(dataSize));
    }

    /* == For inter cluster communication, cost is a bit more complicated to compute == */
    auto *clusterSrc = PESrc->cluster();
    auto *clusterSnk = PESnk->cluster();
    const auto &readWriteCost = spider::math::saturateAdd(clusterSrc->writeCostRoutine()(dataSize),
                                                          clusterSnk->readCostRoutine()(dataSize));
    return spider::math::saturateAdd(readWriteCost,
                                     cluster2ClusterComCostRoutine_(clusterSrc->ix(), clusterSnk->ix(), dataSize));
}
