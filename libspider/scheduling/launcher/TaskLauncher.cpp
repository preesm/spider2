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

#include <scheduling/launcher/TaskLauncher.h>
#include <scheduling/task/pisdf-based/PiSDFTask.h>
#include <scheduling/task/SyncTask.h>
#include <scheduling/task/SRDAGTask.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/memory/FifoAllocator.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <archi/Platform.h>
#include <api/archi-api.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/communicator/RTCommunicator.h>
#include <api/runtime-api.h>

/* === Static function === */

/* === Method(s) implementation === */

void spider::sched::TaskLauncher::visit(SRDAGTask *task) {
    if (task->state() != TaskState::READY) {
        return;
    }
    JobMessage message{ };
    /* == Set Fifos == */
    message.fifos_ = allocator_->buildJobFifos(task);
    fillTaskMessage(task, message);
}

void spider::sched::TaskLauncher::visit(SyncTask *task) {
    if (task->state() != TaskState::READY) {
        return;
    }
    JobMessage message{ };
    /* == Set Fifos == */
    message.fifos_ = allocator_->buildJobFifos(task);
    fillTaskMessage(task, message);
}

void spider::sched::TaskLauncher::visit(PiSDFTask *task) {
    if (task->state() != TaskState::READY) {
        return;
    }
    JobMessage message{ };
    /* == Compute exec dependencies == */
    const auto execDeps = task->computeExecDependencies();
    /* == Compute cons dependencies == */
    const auto consDeps = task->computeConsDependencies();
    /* == Set the synchronization flags == */
    message.synchronizationFlags_ = buildJobNotificationFlags(task, consDeps);
    /* == Set the execution task constraints == */
    size_t constraintCount;
    auto constraintsArray = buildConstraintsArray(task, constraintCount, execDeps);
    message.execConstraints_ = buildExecConstraints(std::move(constraintsArray), constraintCount);
    /* == Set Fifos == */
//    message.fifos_ = task->buildFifos(schedule_, execDeps, consDeps);
    /* == Send the job == */
    sendTask(task, message);
}

/* === Private method(s) implementation === */

void spider::sched::TaskLauncher::fillTaskMessage(Task *task, JobMessage &message) {
    /* == Set the synchronization flags == */
    message.synchronizationFlags_ = buildJobNotificationFlags(task);
    /* == Set the execution task constraints == */
    size_t constraintCount;
    auto constraintsArray = buildConstraintsArray(task, constraintCount);
    message.execConstraints_ = buildExecConstraints(std::move(constraintsArray), constraintCount);
    /* == Send the job == */
    sendTask(task, message);
}

/* === Generic functions === */

void spider::sched::TaskLauncher::sendTask(Task *task, JobMessage &message) {
    /* == Set core properties == */
    message.nParamsOut_ = task->getOutputParamsCount();
    message.kernelIx_ = task->getKernelIx();
    message.taskIx_ = task->ix();
    message.ix_ = task->jobExecIx();
    /* == Set input params == */
    message.inputParams_ = task->buildInputParams();
    /* == Send the job == */
    const auto grtIx = archi::platform()->getGRTIx();
    auto *communicator = rt::platform()->communicator();
    const auto mappedLRTIx = task->mappedLRT()->virtualIx();
    const auto messageIx = communicator->push(std::move(message), mappedLRTIx);
    communicator->push(Notification{ NotificationType::JOB_ADD, grtIx, messageIx }, mappedLRTIx);
    /* == Set job in TaskState::RUNNING == */
    task->setState(TaskState::RUNNING);
}

template<class ...Args>
spider::unique_ptr<bool>
spider::sched::TaskLauncher::buildJobNotificationFlags(const Task *task, Args &&...args) const {
    auto flags = spider::make_n<bool, StackID::RUNTIME>(archi::platform()->LRTCount(), false);
    if (updateNotificationFlags(task, flags, std::forward<Args>(args)...)) {
        return make_unique(flags);
    } else {
        deallocate(flags);
        return spider::unique_ptr<bool>();
    }
}

spider::array<spider::SyncInfo>
spider::sched::TaskLauncher::buildExecConstraints(spider::array<constraint_t> constraintsArray,
                                                  size_t constraintsCount) {
    /* == Now build the actual array of synchronization info == */
    auto result = spider::array<SyncInfo>(constraintsCount, StackID::RUNTIME);
    if (constraintsCount) {
        auto resultIt = std::begin(result);
        for (const auto &constraint : constraintsArray) {
            if (constraint.first != SIZE_MAX) {
                const auto *dependency = constraint.second;
                /* == Set this dependency as a synchronization constraint == */
                resultIt->lrtToWait_ = dependency->mappedLRT()->virtualIx();
                resultIt->jobToWait_ = dependency->jobExecIx();
                /* == Update iterator == */
                if ((++resultIt) == std::end(result)) {
                    /* == shortcut to avoid useless other checks == */
                    break;
                }
            }
        }
    }
    return result;
}

/* === Task type specific functions === */

spider::array<spider::sched::TaskLauncher::constraint_t>
spider::sched::TaskLauncher::buildConstraintsArray(const Task *task, size_t &constraintsCount) const {
    /* == Get the number of actual execution constraints == */
    const auto lrtCount = archi::platform()->LRTCount();
    auto constraintsArray = spider::array<constraint_t>(lrtCount, { SIZE_MAX, nullptr }, StackID::RUNTIME);
    constraintsCount = 0u;
    for (size_t ix = 0; ix < task->dependencyCount(); ++ix) {
        const auto *srcTask = task->previousTask(ix, schedule_);
        if (srcTask && (srcTask->mappedLRT() != task->mappedLRT())) {
            // TODO: handle SKIPPED source
            const auto srcLRTIx = srcTask->mappedLRT()->virtualIx();
            const auto &currentConstraint = constraintsArray[srcLRTIx];
            const auto currentDepIxOnLRT = currentConstraint.first;
            const auto gotConstraintOnLRT = currentDepIxOnLRT != SIZE_MAX;
            if (!gotConstraintOnLRT || (srcTask->jobExecIx() > currentConstraint.second->jobExecIx())) {
                constraintsCount += !gotConstraintOnLRT;
                constraintsArray[srcLRTIx].first = ix;
                constraintsArray[srcLRTIx].second = srcTask;
            }
        }
    }
    return constraintsArray;
}

spider::array<spider::sched::TaskLauncher::constraint_t>
spider::sched::TaskLauncher::buildConstraintsArray(const Task *task,
                                                   size_t &constraintsCount,
                                                   const spider::vector<pisdf::DependencyIterator> &execDeps) const {
    constraintsCount = 0u;
    /* == Get the number of actual execution constraints == */
    const auto lrtCount = archi::platform()->LRTCount();
    auto constraintsArray = spider::array<constraint_t>(lrtCount, { SIZE_MAX, nullptr }, StackID::RUNTIME);
    size_t depIx = 0;
    for (const auto &depIt : execDeps) {
        for (const auto &dep : depIt) {
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto *srcTask = schedule_->task(dep.handler_->getTaskIx(dep.vertex_, k));
                if (srcTask && (srcTask->mappedLRT() != task->mappedLRT())) {
                    // TODO: handle SKIPPED source
                    const auto srcLRTIx = srcTask->mappedLRT()->virtualIx();
                    const auto &currentConstraint = constraintsArray[srcLRTIx];
                    const auto currentDepIxOnLRT = currentConstraint.first;
                    const auto gotConstraintOnLRT = currentDepIxOnLRT != SIZE_MAX;
                    if (!gotConstraintOnLRT || (srcTask->jobExecIx() > currentConstraint.second->jobExecIx())) {
                        constraintsCount += !gotConstraintOnLRT;
                        constraintsArray[srcLRTIx].first = depIx;
                        constraintsArray[srcLRTIx].second = srcTask;
                    }
                }
                depIx++;
            }
        }
    }
    return constraintsArray;
}

bool spider::sched::TaskLauncher::updateNotificationFlags(const Task *task, bool *flags) const {
    auto oneTrue = false;
    for (size_t iOut = 0; iOut < task->successorCount(); ++iOut) {
        const auto *sinkTask = task->nextTask(iOut, schedule_);
        /* == Check if task are not ready == */
        if (!sinkTask || (sinkTask->state() != TaskState::READY &&
                          sinkTask->state() != TaskState::SKIPPED)) {
            /* == broadcast to every LRT == */
            std::fill(flags, flags + archi::platform()->LRTCount(), true);
            return true;
        } else if (sinkTask->state() == TaskState::SKIPPED) {
            updateNotificationFlags(sinkTask, flags);
        }
        auto &currentFlag = flags[sinkTask->mappedLRT()->virtualIx()];
        if (!currentFlag) {
            currentFlag = true;
            for (size_t ix = 0; ix < sinkTask->dependencyCount(); ++ix) {
                auto *sourceTask = sinkTask->previousTask(ix, schedule_);
                if (sourceTask &&
                    (sourceTask->mappedLRT() == task->mappedLRT()) &&
                    (sourceTask->jobExecIx() > task->jobExecIx())) {
                    currentFlag = false;
                    break;
                }
            }
        }
        oneTrue |= currentFlag;
    }
    return oneTrue;
}

bool spider::sched::TaskLauncher::updateNotificationFlags(const Task *task,
                                                          bool *flags,
                                                          const spider::vector<pisdf::DependencyIterator> &consDeps) const {
    auto oneTrue = false;
    for (const auto &depIt : consDeps) {
        for (const auto &dep : depIt) {
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                const auto *sinkTask = schedule_->task(dep.handler_->getTaskIx(dep.vertex_, k));
                /* == Check if task are not ready == */
                if (!sinkTask || (sinkTask->state() != TaskState::READY &&
                                  sinkTask->state() != TaskState::SKIPPED)) {
                    /* == broadcast to every LRT == */
                    std::fill(flags, flags + archi::platform()->LRTCount(), true);
                    return true;
                } else if (sinkTask->state() == TaskState::SKIPPED) {
                    updateNotificationFlags(sinkTask, flags);
                }
                auto &currentFlag = flags[sinkTask->mappedLRT()->virtualIx()];
                if (!currentFlag) {
                    currentFlag = getFlagFromSink(task, static_cast<const PiSDFTask *>(sinkTask));
                }
                oneTrue |= currentFlag;
            }
        }
    }
    return oneTrue;
}

bool spider::sched::TaskLauncher::getFlagFromSink(const Task *task, const PiSDFTask *sinkTask) const {
    /* == Compute exec dependencies == */
    const auto execDeps = sinkTask->computeExecDependencies();
    for (const auto &depIt : execDeps) {
        for (const auto &dep : depIt) {
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                auto *sourceTask = schedule_->task(dep.handler_->getTaskIx(dep.vertex_, k));
                if (sourceTask &&
                    (sourceTask->mappedLRT() == task->mappedLRT()) &&
                    (sourceTask->jobExecIx() > task->jobExecIx())) {
                    return false;
                }
            }
        }
    }
    return true;
}
