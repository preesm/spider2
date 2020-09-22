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

#include <scheduling/task/TaskSync.h>
#include <archi/PE.h>
#include <archi/Platform.h>
#include <archi/MemoryBus.h>
#include <archi/Cluster.h>
#include <runtime/common/RTKernel.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::TaskSync::TaskSync(SyncType type) : Task(), type_{ type } {
    fifos_ = spider::make_shared<AllocatedFifos, StackID::SCHEDULE>(type == SyncType::SEND, 1U);
    execInfo_.dependencies_ = spider::make_unique(allocate<Task *, StackID::SCHEDULE>(1u));
    execInfo_.dependencies_.get()[0u] = nullptr;
}

void spider::sched::TaskSync::updateExecutionConstraints() {
    auto *execDependencies = execInfo_.dependencies_.get();
    auto *execConstraints = execInfo_.constraints_.get();
    auto *dependency = execDependencies[0u];
    const auto lrtCount = archi::platform()->LRTCount();
    std::fill(execConstraints, execConstraints + lrtCount, SIZE_MAX);
    if (dependency) {
        const auto *depLRT = dependency->mappedLRT();
        if (depLRT != mappedLRT()) {
            dependency->setNotificationFlag(mappedLRT()->virtualIx(), true);
            execConstraints[depLRT->virtualIx()] = dependency->jobExecIx();
        }
    }
}

#ifndef NDEBUG

spider::sched::AllocationRule spider::sched::TaskSync::allocationRuleForInputFifo(size_t ix) const {
    if (ix >= 1u) {
        throwSpiderException("index out of bound.");
    }
#else
    spider::sched::AllocationRule spider::sched::TaskSync::allocationRuleForInputFifo(size_t) const {
#endif
    if (type_ == SyncType::SEND) {
        return { SIZE_MAX, 0u, inputPortIx_, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
    }
    return { };
}

#ifndef NDEBUG

spider::sched::AllocationRule spider::sched::TaskSync::allocationRuleForOutputFifo(size_t ix) const {
    if (ix >= 1u) {
        throwSpiderException("index out of bound.");
    }
#else
    spider::sched::AllocationRule spider::sched::TaskSync::allocationRuleForOutputFifo(size_t) const {
#endif
    if (type_ == SyncType::SEND) {
        return { SIZE_MAX, 0u, 0u, AllocType::SAME_IN, FifoAttribute::RW_ONLY };
    } else {
        return { size_, 0u, SIZE_MAX, AllocType::NEW, FifoAttribute::RW_OWN };
    }
}

spider::sched::Task *spider::sched::TaskSync::previousTask(size_t ix) const {
#ifndef NDEBUG
    if (ix >= 1u) {
        throwSpiderException("index out of bound.");
    }
#endif
    return execInfo_.dependencies_.get()[ix];
}

u32 spider::sched::TaskSync::color() const {
    /* ==  SEND    -> vivid tangerine color == */
    /* ==  RECEIVE -> Studio purple color == */
    return type_ == SyncType::SEND ? 0xff9478 : 0x8e44ad;
}

spider::array_handle<spider::sched::Task *> spider::sched::TaskSync::getDependencies() const {
    return { execInfo_.dependencies_.get(), 1u };
}

std::string spider::sched::TaskSync::name() const {
    return type_ == SyncType::SEND ? "send" : "receive";
}

void spider::sched::TaskSync::setSuccessor(spider::sched::Task *successor) {
    if (successor && type_ == SyncType::SEND) {
        successor_ = successor;
    }
}

void spider::sched::TaskSync::setSize(size_t size) {
    size_ = size;
}

void spider::sched::TaskSync::setInputPortIx(u32 ix) {
    inputPortIx_ = ix;
}

void spider::sched::TaskSync::setExecutionDependency(size_t ix, Task *task) {
#ifndef NDEBUG
    if (ix >= 1u) {
        throwSpiderException("index out of bound.");
    }
#endif
    if (task) {
        execInfo_.dependencies_.get()[ix] = task;
    }
}

spider::JobMessage spider::sched::TaskSync::createJobMessage() const {
    JobMessage message{ };
    /* == Set core properties == */
    message.nParamsOut_ = 0u;
    const auto &kernel = type_ == SyncType::SEND ? bus_->sendKernel() : bus_->receiveKernel();
    message.kernelIx_ = static_cast<u32>(kernel->ix());
    message.taskIx_ = ix_;
    message.ix_ = jobExecIx_;

    /* == Set the synchronization flags == */
    const auto lrtCount{ archi::platform()->LRTCount() };
    const auto *flags = execInfo_.notifications_.get();
    message.synchronizationFlags_ = make_unique<bool>(allocate<bool, StackID::RUNTIME>(lrtCount));
    std::copy(flags, std::next(flags, static_cast<long long>(lrtCount)), message.synchronizationFlags_.get());

    /* == Set the execution task constraints == */
    auto *execConstraints = execInfo_.constraints_.get();
    message.execConstraints_ = array<SyncInfo>(1u, StackID::RUNTIME);
    for (size_t i = 0; i < lrtCount; ++i) {
        const auto value = execConstraints[i];
        if (value != SIZE_MAX) {
            message.execConstraints_[0u].lrtToWait_ = i;
            message.execConstraints_[0u].jobToWait_ = static_cast<size_t>(value);
            break;
        }
    }

    /* == Set the params == */
    const auto *fstLRT = type_ == SyncType::SEND ? mappedLRT() : execInfo_.dependencies_.get()[0u]->mappedLRT();
    const auto *sndLRT = type_ == SyncType::SEND ? successor_->mappedLRT() : mappedLRT();
    message.inputParams_ = make_unique(allocate<i64, StackID::RUNTIME>(4u));
    message.inputParams_.get()[0u] = static_cast<i64>(fstLRT->cluster()->ix());
    message.inputParams_.get()[1u] = static_cast<i64>(sndLRT->cluster()->ix());
    message.inputParams_.get()[2u] = static_cast<i64>(size_);
    if (type_ == SyncType::RECEIVE) {
        const auto *dependency = execInfo_.dependencies_.get()[0u];
        const auto &outputFifo = dependency->fifos().outputFifo(0u);
        message.inputParams_.get()[3u] = static_cast<i64>(outputFifo.virtualAddress_);
    } else {
        message.inputParams_.get()[3u] = 0;
    }

    /* == Set Fifos == */
    message.fifos_ = fifos_;
    return message;
}

std::pair<ufast64, ufast64> spider::sched::TaskSync::computeCommunicationCost(const spider::PE *mappedPE) const {
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

u64 spider::sched::TaskSync::timingOnPE(const spider::PE *) const {
    if (!bus_) {
        return UINT64_MAX;
    }
    const auto busSpeed = type_ == SyncType::SEND ? bus_->writeSpeed() : bus_->readSpeed();
    return busSpeed / size_;
}

spider::sched::DependencyInfo spider::sched::TaskSync::getDependencyInfo(size_t) const {
    return { inputPortIx_, size_ };
}
