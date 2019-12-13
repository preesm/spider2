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

#include <api/archi-api.h>
#include <archi/MemoryInterface.h>
#include <archi/MemoryUnit.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/PE.h>

/* === Methods implementation === */

/* === General Platform related API === */

spider::Platform *&spider::platform() {
    static Platform *platform = nullptr;
    return platform;
}

spider::Platform *spider::api::createPlatform(size_t clusterCount, size_t totalPECount) {
    auto *&platform = spider::platform();
    if (!platform) {
        platform = make<Platform, StackID::ARCHI>(clusterCount, totalPECount);
    } else {
        throwSpiderException("platform already exists!");
    }
    return platform;
}

void spider::api::setSpiderGRTPE(PE *grtPE) {
    auto *&platform = spider::platform();
    if (platform) {
        platform->setSpiderGRTPE(grtPE);
    }
}

/* === MemoryUnit related API === */

spider::MemoryUnit *spider::api::createMemoryUnit(uint64_t size) {
    return make<MemoryUnit, StackID::ARCHI>(size);
}

spider::MemoryInterface *spider::api::createMemoryInterface(spider::MemoryUnit *memoryUnit) {
    if (!memoryUnit) {
        throwSpiderException("nullptr MemoryUnit");
    }
    auto *interface = make<MemoryInterface, StackID::ARCHI>();
    interface->setMemoryUnit(memoryUnit);
    return interface;
}

std::pair<spider::MemoryInterface *, spider::MemoryInterface *>
spider::api::createMemoryInterface(spider::Cluster *clusterA, spider::Cluster *clusterB) {
    if (!clusterA || !clusterB) {
        throwSpiderException("nullptr for Cluster");
    }
    /* == Create MemoryInterface for cluster A side == */
    auto *memoryUnitA = clusterA->memoryUnit();
    auto *memoryInterfaceA = make<MemoryInterface, StackID::ARCHI>();
    memoryInterfaceA->setMemoryUnit(memoryUnitA);

    /* == Create MemoryInterface for cluster B side == */
    auto *memoryUnitB = clusterB->memoryUnit();
    auto *memoryInterfaceB = make<MemoryInterface, StackID::ARCHI>();
    memoryInterfaceB->setMemoryUnit(memoryUnitB);
    return std::make_pair(memoryInterfaceA, memoryInterfaceB);
}

void spider::api::setMemoryInterfaceWriteRoutine(spider::MemoryInterface *interface,
                                                 spider::MemoryWriteRoutine routine) {
    if (!interface) {
        throwSpiderException("nullptr MemoryInterface");
    }
    interface->setWriteRoutine(routine);
}

void spider::api::setMemoryInterfaceReadCostRoutine(spider::MemoryInterface *interface,
                                                    spider::MemoryExchangeCostRoutine routine) {
    if (!interface) {
        throwSpiderException("nullptr MemoryInterface");
    }
    interface->setReadCostRoutine(routine);

}

void spider::api::setMemoryInterfaceWriteCostRoutine(spider::MemoryInterface *interface,
                                                     spider::MemoryExchangeCostRoutine routine) {
    if (!interface) {
        throwSpiderException("nullptr MemoryInterface");
    }
    interface->setWriteCostRoutine(routine);

}

void spider::api::setMemoryInterfaceAllocateRoutine(spider::MemoryInterface *interface,
                                                    spider::MemoryAllocateRoutine routine) {
    if (!interface) {
        throwSpiderException("nullptr MemoryInterface");
    }
    interface->setAllocateRoutine(routine);

}

void spider::api::setMemoryInterfaceDeallocateRoutine(spider::MemoryInterface *interface,
                                                      spider::MemoryDeallocateRoutine routine) {
    if (!interface) {
        throwSpiderException("nullptr MemoryInterface");
    }
    interface->setDeallocateRoutine(routine);

}

/* === Cluster related API === */

spider::Cluster *spider::api::createCluster(size_t PECount, MemoryUnit *memoryUnit, MemoryInterface *memoryInterface) {
    auto *&platform = spider::platform();
    if (!platform) {
        throwSpiderException("Can not create cluster for empty platform.");
    }
    auto *cluster = make<Cluster, StackID::ARCHI>(PECount, memoryUnit, memoryInterface);
    return cluster;
}

/* === PE related API === */

spider::PE *spider::api::createPE(uint32_t hwType, uint32_t hwID, Cluster *cluster, std::string name, PEType type) {
    auto *PE = make<spider::PE, StackID::ARCHI>(hwType, hwID, cluster, std::move(name), type);
    PE->enable();
    return PE;
}

void spider::api::setPESpiderPEType(PE *PE, spider::PEType type) {
    PE->setSpiderPEType(type);
}

void spider::api::setPEName(PE *PE, std::string name) {
    if (PE) {
        PE->setName(std::move(name));
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
