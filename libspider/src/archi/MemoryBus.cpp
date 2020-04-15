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
/* === Include(s) === */

#include <archi/MemoryBus.h>
#include <api/runtime-api.h>
#include <runtime/platform/RTPlatform.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <common/Types.h>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::MemoryBus::MemoryBus() {
    sendCostRoutine_ = [](u64) -> u64 { return 0; };
    receiveCostRoutine_ = [](u64) -> u64 { return 0; };
    sendRoutine_ = [](i64, i32, void *) {};
    receiveRoutine_ = [](i64, i32, void *) {};
}

uint64_t spider::MemoryBus::sendCost(uint64_t size) const {
    return sendCostRoutine_(size);
}

uint64_t spider::MemoryBus::receiveCost(uint64_t size) const {
    return receiveCostRoutine_(size);
}

void spider::MemoryBus::dataSend(i64 size, i32 packetIx, void *buffer) {
    sendRoutine_(size, packetIx, buffer);
}

void spider::MemoryBus::dataReceive(i64 size, i32 packetIx, void *buffer) {
    receiveRoutine_(size, packetIx, buffer);
}

spider::RTKernel *spider::MemoryBus::sendKernel() const {
    auto *platform = rt::platform();
    if (platform) {
        return platform->getKernel(sendKernelIx_);
    }
    return nullptr;
}

spider::RTKernel *spider::MemoryBus::receiveKernel() const {
    auto *platform = rt::platform();
    if (platform) {
        return platform->getKernel(recvKernelIx_);
    }
    return nullptr;
}

uint64_t spider::MemoryBus::writeSpeed() const {
    return writeSpeed_;
}

uint64_t spider::MemoryBus::readSpeed() const {
    return readSpeed_;
}

void spider::MemoryBus::setSendCostRoutine(MemoryExchangeCostRoutine routine) {
    sendCostRoutine_ = routine;
}

void spider::MemoryBus::setReceiveCostRoutine(MemoryExchangeCostRoutine routine) {
    receiveCostRoutine_ = routine;
}

void spider::MemoryBus::setSendRoutine(MemoryBusRoutine routine) {
    sendRoutine_ = routine;
    auto *platform = rt::platform();
    if (platform) {
        auto *kernel = make<RTKernel, StackID::ARCHI>(send);
        sendKernelIx_ = platform->addKernel(kernel);
    }
}

void spider::MemoryBus::setReceiveRoutine(MemoryBusRoutine routine) {
    receiveRoutine_ = routine;
    auto *platform = rt::platform();
    if (platform) {
        auto *kernel = make<RTKernel, StackID::ARCHI>(receive);
        recvKernelIx_ = platform->addKernel(kernel);
    }
}

void spider::MemoryBus::send(const int64_t *paramsIN, int64_t *, void *in[], void *[]) {
    auto *clusterA = archi::platform()->cluster(static_cast<size_t>(paramsIN[0])); /* = source = */
    auto *clusterB = archi::platform()->cluster(static_cast<size_t>(paramsIN[1])); /* = target = */
    auto *bus = archi::platform()->getClusterToClusterMemoryBus(clusterA, clusterB);
    bus->dataSend(paramsIN[2], static_cast<i32>(paramsIN[3]), in);
}

void spider::MemoryBus::receive(const int64_t *paramsIN, int64_t *, void *[], void *out[]) {
    auto *clusterA = archi::platform()->cluster(static_cast<size_t>(paramsIN[0])); /* = source = */
    auto *clusterB = archi::platform()->cluster(static_cast<size_t>(paramsIN[1])); /* = target = */
    auto *bus = archi::platform()->getClusterToClusterMemoryBus(clusterA, clusterB);
    bus->dataReceive(paramsIN[2], static_cast<i32>(paramsIN[3]), out);
}

void spider::MemoryBus::setWriteSpeed(uint64_t value) {
    writeSpeed_ = value;
}

void spider::MemoryBus::setReadSpeed(uint64_t value) {
    readSpeed_ = value;
}
