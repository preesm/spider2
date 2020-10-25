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

#include <scheduling/task/SyncTask.h>
#include <scheduling/memory/FifoAllocator.h>
#include <archi/PE.h>
#include <archi/Platform.h>
#include <archi/MemoryBus.h>
#include <archi/Cluster.h>
#include <runtime/common/RTKernel.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::SyncTask::SyncTask(SyncType type, const MemoryBus *bus) : Task(), bus_{ bus }, type_{ type } {
}

/* === Virtual method(s) === */

u64 spider::sched::SyncTask::timingOnPE(const spider::PE *) const {
    if (!bus_) {
        return UINT64_MAX;
    }
    const auto busSpeed = type_ == SyncType::SEND ? bus_->writeSpeed() : bus_->readSpeed();
    return busSpeed / size_;
}

/* === Private method(s) === */

u32 spider::sched::SyncTask::getKernelIx() const {
    if (type_ == SyncType::SEND) {
        return static_cast<u32>(bus_->sendKernel()->ix());
    } else {
        return static_cast<u32>(bus_->receiveKernel()->ix());
    }
}

spider::unique_ptr<i64> spider::sched::SyncTask::buildInputParams() const {
    auto params = spider::allocate<i64, StackID::RUNTIME>(4u);
#ifndef NDEBUG
    if (!params) {
        throwNullptrException();
    }
#endif
    if (type_ == SyncType::SEND) {
        const auto *fstLRT = mappedLRT();
        const auto *sndLRT = successor_->mappedLRT();
        params[0u] = static_cast<i64>(fstLRT->cluster()->ix());
        params[1u] = static_cast<i64>(sndLRT->cluster()->ix());
        params[2u] = static_cast<i64>(size_);
        params[3u] = 0;
    } else {
        const auto *fstLRT = dependency_->mappedLRT();
        const auto *sndLRT = mappedLRT();
        params[0u] = static_cast<i64>(fstLRT->cluster()->ix());
        params[1u] = static_cast<i64>(sndLRT->cluster()->ix());
        params[2u] = static_cast<i64>(size_);
        params[3u] = static_cast<i64>(alloc_.size_);
    }
    return make_unique(params);
}

bool spider::sched::SyncTask::updateNotificationFlags(bool *flags, const Schedule *schedule) const {
    auto oneTrue = false;
    if (successor_->state() == TaskState::SKIPPED) {
        successor_->updateNotificationFlags(flags, schedule);
    }
    auto &currentFlag = flags[successor_->mappedLRT()->virtualIx()];
    if (!currentFlag) {
        currentFlag = true;
        for (size_t ix = 0; ix < successor_->dependencyCount(); ++ix) {
            auto *sourceTask = successor_->previousTask(ix, schedule);
            if (sourceTask && (sourceTask->mappedLRT() == mappedLRT()) && (sourceTask->jobExecIx() > jobExecIx())) {
                currentFlag = false;
                break;
            }
        }
    }
    oneTrue |= currentFlag;
    return oneTrue;
}

bool spider::sched::SyncTask::shouldBroadCast(const Schedule *) const {
    return !successor_ || (successor_->state() != TaskState::READY &&
                           successor_->state() != TaskState::SKIPPED);
}

std::shared_ptr<spider::JobFifos> spider::sched::SyncTask::buildJobFifos(const Schedule *) const {
    auto fifos = spider::make_shared<JobFifos, StackID::RUNTIME>(1, 1);
    /* == Create input fifo == */
    auto inputFifo = alloc_;
    inputFifo.count_ = 0;
    inputFifo.attribute_ = FifoAttribute::RW_OWN;
    fifos->setInputFifo(0, inputFifo);
    /* == Set output fifo == */
    auto outputFifo = alloc_;
    outputFifo.count_ = outputFifo.size_ ? 1 : 0;
    if (type_ == RECEIVE) {
        /* == The receive task should allocate memory in the other memory interface == */
        outputFifo.attribute_ = FifoAttribute::RW_OWN;
    }
    fifos->setOutputFifo(0, outputFifo);
    return fifos;
}
