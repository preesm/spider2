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

spider::sched::SyncTask::SyncTask(SyncType type) : Task(), type_{ type } {
    fifos_ = spider::make_shared<JobFifos, StackID::SCHEDULE>(0, 1);
    dependencies_ = spider::make_unique(spider::allocate<Task *, StackID::SCHEDULE>(1u));
    dependencies_.get()[0u] = nullptr;
}

void spider::sched::SyncTask::allocate(FifoAllocator *allocator) {
    allocator->allocate(this);
}

#ifndef NDEBUG

spider::sched::AllocationRule spider::sched::SyncTask::allocationRuleForInputFifo(size_t ix) const {
    if (ix >= 1u) {
        throwSpiderException("index out of bound.");
    }
#else
    spider::sched::AllocationRule spider::sched::SyncTask::allocationRuleForInputFifo(size_t) const {
#endif
    if (type_ == SyncType::SEND) {
        return { static_cast<u32>(size_), 0, inputPortIx_, 0, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
    }
    return { };
}

#ifndef NDEBUG

spider::sched::AllocationRule spider::sched::SyncTask::allocationRuleForOutputFifo(size_t ix) const {
    if (ix >= 1u) {
        throwSpiderException("index out of bound.");
    }
#else
    spider::sched::AllocationRule spider::sched::SyncTask::allocationRuleForOutputFifo(size_t) const {
#endif
    if (type_ == SyncType::SEND) {
        return { static_cast<u32>(size_), 0, 0, 0, AllocType::SAME_IN, FifoAttribute::RW_OWN };
    } else {
        return { static_cast<u32>(size_), 0, UINT32_MAX, 0, AllocType::SAME_IN, FifoAttribute::RW_OWN };
    }
}

u32 spider::sched::SyncTask::color() const {
    /* ==  SEND    -> vivid tangerine color == */
    /* ==  RECEIVE -> Studio purple color == */
    return type_ == SyncType::SEND ? 0xff9478 : 0x8e44ad;
}

std::string spider::sched::SyncTask::name() const {
    return type_ == SyncType::SEND ? "send" : "receive";
}

void spider::sched::SyncTask::setSuccessor(spider::sched::Task *successor) {
    if (successor && type_ == SyncType::SEND) {
        successor_ = successor;
    }
}

void spider::sched::SyncTask::setSize(size_t size) {
    size_ = size;
}

void spider::sched::SyncTask::setInputPortIx(u32 ix) {
    inputPortIx_ = ix;
}

spider::JobMessage spider::sched::SyncTask::createJobMessage() const {
    auto message = Task::createJobMessage();
    /* == Set core properties == */
    const auto &kernel = type_ == SyncType::SEND ? bus_->sendKernel() : bus_->receiveKernel();
    message.kernelIx_ = static_cast<u32>(kernel->ix());
    /* == Set the params == */
    const auto *fstLRT = type_ == SyncType::SEND ? mappedLRT() : dependencies_.get()[0u]->mappedLRT();
    const auto *sndLRT = type_ == SyncType::SEND ? successor_->mappedLRT() : mappedLRT();
    message.inputParams_ = make_unique(spider::allocate<i64, StackID::RUNTIME>(4u));
    message.inputParams_.get()[0u] = static_cast<i64>(fstLRT->cluster()->ix());
    message.inputParams_.get()[1u] = static_cast<i64>(sndLRT->cluster()->ix());
    message.inputParams_.get()[2u] = static_cast<i64>(size_);
    if (type_ == SyncType::RECEIVE) {
        const auto *dependency = dependencies_.get()[0u];
        const auto &outputFifo = dependency->fifos().outputFifo(0u);
        message.inputParams_.get()[3u] = static_cast<i64>(outputFifo.virtualAddress_);
        message.fifos_ = spider::make_shared<JobFifos, StackID::SCHEDULE>(0, 1);
    } else {
        message.inputParams_.get()[3u] = 0;
        message.fifos_ = spider::make_shared<JobFifos, StackID::SCHEDULE>(1, 1);
        message.fifos_->setInputFifo(0, previousTask(0)->fifos().outputFifo(inputPortIx_));
    }
    message.fifos_->setOutputFifo(0, fifos().outputFifo(0));
    return message;
}

std::pair<ufast64, ufast64> spider::sched::SyncTask::computeCommunicationCost(const spider::PE *mappedPE) const {
    const auto *platform = archi::platform();
    ufast64 externDataToReceive = 0u;
    /* == Compute communication cost == */
    ufast64 communicationCost = 0;
    const auto *taskSource = previousTask(0u);
    if (size_ && taskSource) {
        const auto *mappedPESource = taskSource->mappedPe();
        communicationCost += platform->dataCommunicationCostPEToPE(mappedPESource, mappedPE, size_);
        if (mappedPE->cluster() != mappedPESource->cluster()) {
            externDataToReceive += size_;
        }
    }
    return { communicationCost, externDataToReceive };
}

u64 spider::sched::SyncTask::timingOnPE(const spider::PE *) const {
    if (!bus_) {
        return UINT64_MAX;
    }
    const auto busSpeed = type_ == SyncType::SEND ? bus_->writeSpeed() : bus_->readSpeed();
    return busSpeed / size_;
}

spider::sched::DependencyInfo spider::sched::SyncTask::getDependencyInfo(size_t) const {
    return { inputPortIx_, size_ };
}

size_t spider::sched::SyncTask::dependencyCount() const {
    return 1u;
}
