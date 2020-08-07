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

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::TaskSync::TaskSync(SyncType type) : Task(), type_{ type } {
    fifos_ = spider::make_shared<TaskFifos, StackID::SCHEDULE>(type == SyncType::SEND, 1U);
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

spider::sched::AllocationRule spider::sched::TaskSync::allocationRuleForInputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= 1u) {
        throwSpiderException("index out of bound.");
    }
#endif
    return { 0u, SIZE_MAX, AllocType::SAME, FifoAttribute::RW_ONLY };
}

spider::sched::AllocationRule spider::sched::TaskSync::allocationRuleForOutputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= 1u) {
        throwSpiderException("index out of bound.");
    }
#endif
    return { 0u, SIZE_MAX, AllocType::SAME, FifoAttribute::RW_ONLY };
}

spider::sched::Task *spider::sched::TaskSync::previousTask(size_t ix) const {
#ifndef NDEBUG
    if (ix >= 1u) {
        throwSpiderException("index out of bound.");
    }
#endif
    return nullptr;
}

u32 spider::sched::TaskSync::color() const {
    /* ==  SEND    -> vivid tangerine color == */
    /* ==  RECEIVE -> Studio purple color == */
    return type_ == SyncType::SEND ? 0xff9478 : 0x8e44ad;
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
        execInfo_.dependencies_.get()[0u] = task;
    }
}

spider::JobMessage spider::sched::TaskSync::createJobMessage() const {
    return spider::JobMessage();
}

/* === Private method(s) implementation === */