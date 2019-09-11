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

Platform *&Spider::platform() {
    static Platform *platform = nullptr;
    return platform;
}

Platform *Spider::API::createPlatform(std::uint32_t clusterCount) {
    auto *&platform = Spider::platform();
    platform = Spider::allocate<Platform>(StackID::ARCHI);
    Spider::construct(platform, clusterCount);
    return platform;
}

void Spider::API::setSpiderGRTPE(ProcessingElement *grtPE) {
    auto *&platform = Spider::platform();
    if (platform) {
        platform->setSpiderGRTPE(grtPE);
    }
}

/* === Cluster related API === */

Cluster *Spider::API::createCluster(std::uint32_t PECount, MemoryUnit *memoryUnit) {
    auto *cluster = Spider::allocate<Cluster>(StackID::ARCHI);
    Spider::construct(cluster, PECount, memoryUnit, Spider::platform());
    return cluster;
}

/* === PE related API === */

ProcessingElement *Spider::API::createPE(std::uint32_t hwType,
                                         std::uint32_t hwID,
                                         std::uint32_t virtID,
                                         Cluster *cluster,
                                         const std::string &name,
                                         Spider::PEType spiderPEType,
                                         Spider::HWType spiderHWType) {
    auto *PE = Spider::allocate<ProcessingElement>(StackID::ARCHI);
    Spider::construct(PE, hwType, hwID, virtID, cluster, name, spiderPEType, spiderHWType);
    PE->enable();
    return PE;
}

void Spider::API::setPESpiderPEType(ProcessingElement *PE, Spider::PEType type) {
    PE->setSpiderPEType(type);
}

void Spider::API::setPESpiderHWType(ProcessingElement *PE, Spider::HWType type) {
    PE->setSpiderHWType(type);
}

void Spider::API::setPEName(ProcessingElement *PE, const std::string &name) {
    if (PE) {
        PE->setName(name);
    }
}

void Spider::API::enablePE(ProcessingElement *PE) {
    if (PE) {
        PE->enable();
    }
}

void Spider::API::disablePE(ProcessingElement *PE) {
    if (PE) {
        PE->disable();
    }
}

/* === MemoryUnit related API === */

MemoryUnit *Spider::API::createMemoryUnit(char *base, std::uint64_t size) {
    return nullptr;
}
