/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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

#include <graphs/sched/SyncSchedVertex.h>
#include <graphs/sched/SchedGraph.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <archi/MemoryBus.h>
#include <runtime/common/RTKernel.h>

/* === Static function === */

/* === Method(s) implementation === */

u64 spider::sched::SyncVertex::timingOnPE(const spider::PE *pe) const {
    if (!bus_ || pe != mappedPe()) {
        return UINT64_MAX;
    }
    if (type_ == SyncType::SEND) {
        return bus_->writeSpeed() / Vertex::inputEdge(0)->getAlloc().size_;
    } else {
        return bus_->readSpeed() / Vertex::outputEdge(0)->getAlloc().size_;
    }
}

/* === Private method(s) === */

u32 spider::sched::SyncVertex::getKernelIx() const {
    if (type_ == SyncType::SEND) {
        return static_cast<u32>(bus_->sendKernel()->ix());
    } else {
        return static_cast<u32>(bus_->receiveKernel()->ix());
    }
}

spider::unique_ptr<i64> spider::sched::SyncVertex::buildInputParams() const {
    auto params = spider::allocate<i64, StackID::RUNTIME>(4u);
#ifndef NDEBUG
    if (!params) {
        throwNullptrException();
    }
#endif
    if (type_ == SyncType::SEND) {
        const auto *fstLRT = Vertex::mappedLRT();
        const auto *sndLRT = Vertex::outputEdge(0)->sink()->mappedLRT();
        const auto size = Vertex::inputEdge(0)->getAlloc().size_;
        params[0u] = static_cast<i64>(fstLRT->cluster()->ix());
        params[1u] = static_cast<i64>(sndLRT->cluster()->ix());
        params[2u] = static_cast<i64>(size);
        params[3u] = 0;
    } else {
        const auto *fstLRT = inputEdge(0)->source()->mappedLRT();
        const auto *sndLRT = Vertex::mappedLRT();
        const auto size = Vertex::outputEdge(0)->getAlloc().size_;
        params[0u] = static_cast<i64>(fstLRT->cluster()->ix());
        params[1u] = static_cast<i64>(sndLRT->cluster()->ix());
        params[2u] = static_cast<i64>(size);
        params[3u] = static_cast<i64>(Vertex::inputEdge(0)->getAlloc().virtualAddress_);
    }
    return make_unique(params);
}
