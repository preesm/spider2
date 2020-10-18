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

#include <scheduling/task/Task.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <archi/PE.h>
#include <algorithm>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::sched::Task::Task() {
    const auto lrtCount{ archi::platform()->LRTCount() };
    notifications_ = make_unique<bool>(spider::allocate<bool, StackID::SCHEDULE>(lrtCount));
    std::fill(notifications_.get(), notifications_.get() + lrtCount, false);
}

void spider::sched::Task::enableBroadcast() {
    const auto lrtCount = archi::platform()->LRTCount();
    std::fill(notifications_.get(), notifications_.get() + lrtCount, true);
}

spider::array<size_t> spider::sched::Task::updateDependenciesNotificationFlag() const {
    auto *execDependencies = dependencies_.get();
    auto shouldNotifyArray = array<size_t>(archi::platform()->LRTCount(), SIZE_MAX, StackID::SCHEDULE);
    auto numberOfConstraints{ 0u };
    for (size_t i = 0; i < this->dependencyCount(); ++i) {
        auto *dependency = execDependencies[i];
        if (dependency) {
            const auto *depLRT = dependency->mappedLRT();
            const auto currentDepIxOnLRT = shouldNotifyArray[depLRT->virtualIx()];
            const auto gotConstraintOnLRT = currentDepIxOnLRT != SIZE_MAX;
            if (!gotConstraintOnLRT || (dependency->jobExecIx() > execDependencies[currentDepIxOnLRT]->jobExecIx())) {
                numberOfConstraints += !gotConstraintOnLRT;
                shouldNotifyArray[depLRT->virtualIx()] = i;
            }
        }
    }
    for (const auto depIx : shouldNotifyArray) {
        if (depIx != SIZE_MAX) {
            auto *dependency = execDependencies[depIx];
            /* == Ask the dependency to notify us == */
            dependency->setNotificationFlag(mappedLRT()->virtualIx(), true);
        }
    }
    return shouldNotifyArray;
}

u64 spider::sched::Task::startTime() const {
    return startTime_;
}

u64 spider::sched::Task::endTime() const {
    return endTime_;
}

const spider::PE *spider::sched::Task::mappedPe() const {
    return mappedPE_;
}

const spider::PE *spider::sched::Task::mappedLRT() const {
    return mappedPE_->attachedLRT();
}

spider::sched::Task *spider::sched::Task::previousTask(size_t ix) const {
#ifndef NDEBUG
    if (ix >= dependencyCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    return dependencies_.get()[ix];
}

void spider::sched::Task::setStartTime(u64 time) {
    startTime_ = time;
}

void spider::sched::Task::setEndTime(u64 time) {
    endTime_ = time;
}

void spider::sched::Task::setMappedPE(const spider::PE *const pe) {
    mappedPE_ = pe;
}

void spider::sched::Task::setExecutionDependency(size_t ix, Task *task) {
#ifndef NDEBUG
    if (ix >= dependencyCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    if (task) {
        dependencies_.get()[ix] = task;
    }
}

spider::JobMessage spider::sched::Task::createJobMessage() const {
    JobMessage message{ };
    /* == Set core properties == */
    message.nParamsOut_ = 0u;
    message.kernelIx_ = UINT32_MAX;
    message.taskIx_ = ix_;
    message.ix_ = jobExecIx_;
    /* == Set the synchronization flags == */
    const auto lrtCount{ archi::platform()->LRTCount() };
    const auto *flags = notifications_.get();
    message.synchronizationFlags_ = make_unique<bool>(spider::allocate<bool, StackID::RUNTIME>(lrtCount));
    std::copy(flags, flags + lrtCount, message.synchronizationFlags_.get());
    /* == Set the execution task constraints == */
    message.execConstraints_ = Task::getExecutionConstraints();
    /* == Set Fifos == */
    message.fifos_ = fifos_;
    return message;
}

/* === Protected method(s) === */

spider::array<spider::SyncInfo> spider::sched::Task::getExecutionConstraints() const {
    const auto lrtNotifArray = updateDependenciesNotificationFlag();
    const auto numberOfNonNULLDep = std::count(std::begin(lrtNotifArray), std::end(lrtNotifArray), SIZE_MAX);
    const auto numberOfConstraints{ archi::platform()->LRTCount() - static_cast<size_t>(numberOfNonNULLDep) };
    auto result = array<SyncInfo>(numberOfConstraints, StackID::RUNTIME);
    if (numberOfConstraints) {
        auto resultIt = std::begin(result);
        for (const auto depIx : lrtNotifArray) {
            if (depIx != SIZE_MAX) {
                auto *dependency = dependencies_.get()[depIx];
                /* == Set this dependency as a synchronization constraint == */
                resultIt->lrtToWait_ = dependency->mappedLRT()->virtualIx();
                resultIt->jobToWait_ = dependency->jobExecIx();
                /* == Update iterator == */
                if ((++resultIt) == std::end(result)) {
                    /* == shortcut to avoid useless other checks == */
                    return result;
                }
            }
        }
    }
    return result;
}
