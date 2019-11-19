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

/* === Includes === */

#include <spider-api/archi.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/ProcessingElement.h>
#include <archi/MemoryUnit.h>

/* === Methods implementation === */

/* === General Platform related API === */

spider::Platform *&spider::platform() {
    static Platform *platform = nullptr;
    return platform;
}

spider::Platform *spider::api::createPlatform(std::uint32_t clusterCount) {
    auto *&platform = spider::platform();
    if (!platform) {
        platform = spider::allocate<Platform>(StackID::ARCHI);
        spider::construct(platform, clusterCount);
    }
    return platform;
}

void spider::api::setSpiderGRTPE(PE *grtPE) {
    auto *&platform = spider::platform();
    if (platform) {
        platform->setSpiderGRTPE(grtPE);
    }
}

void spider::api::setCluster2ClusterCommunicationCostRoutine(spider::CommunicationCostRoutineC2C routine) {
    platform()->setCluster2ClusterRoutine(routine);
}

/* === Cluster related API === */

spider::Cluster *spider::api::createCluster(std::uint32_t PECount, MemoryUnit *memoryUnit) {
    auto *cluster = spider::allocate<Cluster>(StackID::ARCHI);
    spider::construct(cluster, PECount, memoryUnit);
    return cluster;
}

void spider::api::setClusterWriteCostRoutine(Cluster *cluster, spider::CommunicationCostRoutine routine) {
    cluster->setWriteCostRoutine(routine);
}

void spider::api::setClusterReadCostRoutine(Cluster *cluster, spider::CommunicationCostRoutine routine) {
    cluster->setReadCostRoutine(routine);
}

/* === PE related API === */

spider::PE *spider::api::createPE(std::uint32_t hwType,
                                  std::uint32_t hwID,
                                  std::uint32_t virtID,
                                  Cluster *cluster,
                                  const std::string &name,
                                  spider::PEType spiderPEType,
                                  spider::HWType spiderHWType) {
    auto *PE = spider::allocate<spider::PE>(StackID::ARCHI);
    spider::construct(PE, hwType, hwID, virtID, cluster, name, spiderPEType, spiderHWType);
    PE->enable();
    return PE;
}

void spider::api::setPESpiderPEType(PE *PE, spider::PEType type) {
    PE->setSpiderPEType(type);
}

void spider::api::setPESpiderHWType(PE *PE, spider::HWType type) {
    PE->setSpiderHWType(type);
}

void spider::api::setPEName(PE *PE, const std::string &name) {
    if (PE) {
        PE->setName(name);
    }
}

void spider::api::enablePE(PE *PE) {
    if (PE) {
        PE->enable();
    }
}

void spider::api::disablePE(PE *PE) {
    if (PE) {
        PE->disable();
    }
}

/* === MemoryUnit related API === */

spider::MemoryUnit *spider::api::createMemoryUnit(void *base, std::uint64_t size) {
    auto *unit = spider::allocate<MemoryUnit>(StackID::ARCHI);
    spider::construct(unit, base, size);
    return unit;
}
