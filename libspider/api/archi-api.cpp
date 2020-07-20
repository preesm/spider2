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
/* === Includes === */

#include <api/archi-api.h>
#include <archi/MemoryBus.h>
#include <archi/MemoryInterface.h>
#include <archi/InterMemoryBus.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/PE.h>

/* === Methods implementation === */

/* === General Platform related API === */

spider::Platform *&spider::archi::platform() {
    static Platform *platform = nullptr;
    return platform;
}

spider::Platform *spider::api::createPlatform(size_t clusterCount, size_t totalPECount) {
    auto *&platform = archi::platform();
    if (!platform) {
        platform = make<Platform, StackID::ARCHI>(clusterCount, totalPECount);
    } else {
        throwSpiderException("platform already exists!");
    }
    return platform;
}

void spider::api::setSpiderGRTPE(PE *grtProcessingElement) {
    auto *&platform = archi::platform();
    if (platform) {
        platform->setSpiderGRTPE(grtProcessingElement);
    }
}

/* === MemoryUnit related API === */

spider::MemoryInterface *spider::api::createMemoryInterface(uint64_t size) {
    auto *interface = make<MemoryInterface, StackID::ARCHI>(size);
    return interface;
}

void spider::api::setMemoryInterfaceAllocateRoutine(MemoryInterface *interface, MemoryAllocateRoutine routine) {
    if (interface) {
        interface->setAllocateRoutine(std::move(routine));
    }
}

void spider::api::setMemoryInterfaceDeallocateRoutine(MemoryInterface *interface, MemoryDeallocateRoutine routine) {
    if (interface) {
        interface->setDeallocateRoutine(std::move(routine));
    }
}

spider::MemoryBus *spider::api::createMemoryBus(MemoryBusRoutine sendRoutine, MemoryBusRoutine receiveRoutine) {
    auto *bus = make<MemoryBus, StackID::ARCHI>();
    if (bus) {
        bus->setSendRoutine(std::move(sendRoutine));
        bus->setReceiveRoutine(std::move(receiveRoutine));
    }
    return bus;
}

void spider::api::setMemoryBusSendCostRoutine(MemoryBus *bus, MemoryExchangeCostRoutine routine) {
    if (bus) {
        bus->setSendCostRoutine(std::move(routine));
    }
}

void spider::api::setMemoryBusReceiveCostRoutine(MemoryBus *bus, MemoryExchangeCostRoutine routine) {
    if (bus) {
        bus->setReceiveCostRoutine(std::move(routine));
    }
}


void spider::api::setMemoryBusWriteSpeed(MemoryBus *bus, uint64_t value) {
    if (bus) {
        bus->setWriteSpeed(value);
    }
}

void spider::api::setMemoryBusReadSpeed(MemoryBus *bus, uint64_t value) {
    if (bus) {
        bus->setReadSpeed(value);
    }
}

spider::InterMemoryBus *
spider::api::createInterClusterMemoryBus(Cluster *clusterA, Cluster *clusterB, MemoryBus *busAToB, MemoryBus *busBToA) {
    if (!clusterA || !clusterB) {
        throwSpiderException("nullptr for Cluster: use spider::api::createCluster() first.");
    }
    if (!archi::platform()) {
        throwSpiderException("nullptr for platform(): use spider::api::createPlatform() first.");
    }
    /* == Create InterMemoryBus == */
    auto *interMemoryBus = make<InterMemoryBus, StackID::ARCHI>(clusterA, clusterB, busAToB, busBToA);

    /* == Set the interface in the platform == */
    archi::platform()->setClusterToClusterMemoryBus(clusterA, clusterB, interMemoryBus);
    return interMemoryBus;
}

/* === Cluster related API === */

spider::Cluster *spider::api::createCluster(size_t PECount, MemoryInterface *memoryInterface) {
    auto *&platform = archi::platform();
    if (!platform) {
        throwSpiderException("Can not create cluster for empty platform.");
    }
    auto *cluster = make<Cluster, StackID::ARCHI>(PECount, memoryInterface);
    platform->addCluster(cluster);
    return cluster;
}

/* === PE related API === */

spider::PE *
spider::api::createProcessingElement(uint32_t hwType,
                                     uint32_t hwID,
                                     Cluster *cluster,
                                     std::string name,
                                     PEType type,
                                     int32_t affinity) {
    if (!cluster) {
        throwSpiderException("nullptr for Cluster");
    }
    auto *processingElement = make<PE, StackID::ARCHI>(hwType, hwID, cluster, std::move(name), type, affinity);
    processingElement->enable();
    cluster->addPE(processingElement);
    return processingElement;
}

void spider::api::attachPEToLRT(spider::PE *pe, spider::PE *lrt) {
    if (!pe || !lrt) {
        return;
    }
    if ((pe->attachedLRT() == pe) || (pe->isLRT())) {
        return;
    }
    if (!lrt->isLRT()) {
        throwSpiderException("can not attached PE [%s] to PE [%s]: not an LRT.", pe->name().c_str(), lrt->name().c_str());
    }
    pe->setAttachedLRT(lrt);
}

void spider::api::setPESpiderPEType(PE *processingElement, PEType type) {
    processingElement->setSpiderPEType(type);
}

void spider::api::setPEName(PE *processingElement, std::string name) {
    if (processingElement) {
        processingElement->setName(std::move(name));
    }
}

void spider::api::enablePE(PE *processingElement) {
    if (processingElement) {
        processingElement->enable();
    }
}

void spider::api::disablePE(PE *processingElement) {
    if (processingElement) {
        processingElement->disable();
    }
}
