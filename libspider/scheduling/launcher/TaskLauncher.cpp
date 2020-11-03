/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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

#include <scheduling/launcher/TaskLauncher.h>
#include <scheduling/task/PiSDFTask.h>
#include <scheduling/task/SyncTask.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/memory/FifoAllocator.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/MemoryBus.h>
#include <api/archi-api.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/communicator/RTCommunicator.h>
#include <api/runtime-api.h>

#ifndef _NO_BUILD_LEGACY_RT

#include <scheduling/task/SRDAGTask.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <graphs-tools/helper/srdag-helper.h>

#endif
/* === Static function === */

/* === Method(s) implementation === */

#ifndef _NO_BUILD_LEGACY_RT

void spider::sched::TaskLauncher::visit(SRDAGTask *task) {
    if (task->state() != TaskState::READY) {
        return;
    }
    JobMessage message{ };
    /* == Setting core properties == */
    const auto *vertex = task->vertex();
    message.nParamsOut_ = static_cast<u32>(vertex->reference()->outputParamCount());
    message.kernelIx_ = static_cast<u32>(vertex->runtimeInformation()->kernelIx());
    /* == Set the synchronization flags == */
    message.synchronizationFlags_ = buildJobNotificationFlags(task);
    /* == Set Fifos == */
    message.fifos_ = allocator_->buildJobFifos(task);
    /* == Set input params == */
    message.inputParams_ = srdag::buildVertexRuntimeInputParameters(vertex);
    /* == Send the job == */
    sendTask(task, message);
}

#endif

void spider::sched::TaskLauncher::visit(SyncTask *task) {
    if (task->state() != TaskState::READY) {
        return;
    }
    /* == Push task for later purpose == */
    deferedSyncTasks_.push_back({ task, task->nextTask(0, nullptr)->ix() });
}

void spider::sched::TaskLauncher::visit(PiSDFTask *task) {
    if (!task) {
        throwSpiderException("can not launch nullptr task.");
    }
    if (task->state() != TaskState::READY) {
        return;
    }
    JobMessage message{ };
    /* == Compute exec dependencies == */
    const auto execDeps = task->computeExecDependencies();
    /* == Compute cons dependencies == */
    const auto consDeps = task->computeConsDependencies();
    /* == Setting core properties == */
    const auto *vertex = task->vertex();
    message.nParamsOut_ = static_cast<u32>(vertex->outputParamCount());
    message.kernelIx_ = static_cast<u32>(vertex->runtimeInformation()->kernelIx());
    /* == Set the synchronization flags == */
    message.synchronizationFlags_ = buildJobNotificationFlags(task, consDeps);
    /* == Set Fifos == */
    message.fifos_ = allocator_->buildJobFifos(task, execDeps, consDeps);
    /* == Set input params == */
    message.inputParams_ = pisdf::buildVertexRuntimeInputParameters(vertex, task->handler()->getParams());
    /* == Send the job == */
    sendTask(task, message);
}

/* === Private method(s) implementation === */

void spider::sched::TaskLauncher::sendTask(Task *task, JobMessage &message) {
    /* == Set core properties == */
    message.taskIx_ = task->ix();
    message.execIx_ = task->jobExecIx();
    /* == Set the execution task constraints == */
    message.execConstraints_ = buildExecConstraints(task);
    /* == Check for sync tasks to be sent == */
    if (!deferedSyncTasks_.empty()) {
        for (size_t i = 0; i < deferedSyncTasks_.size(); ++i) {
            const auto &deferedTask = deferedSyncTasks_[i];
            if (deferedTask.second == message.taskIx_) {
                /* == Ok, this concern us == */
                /* == Send task == */
                sendSyncTask(deferedSyncTasks_[i - 1].first, message);
                /* == Receive task == */
                sendSyncTask(deferedTask.first, message);
            }
        }
    }
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
    updateNotificationFlags(task, flags, std::forward<Args>(args)...);
    if (std::any_of(flags, flags + archi::platform()->LRTCount(), [](bool value) { return value; })) {
        return make_unique(flags);
    } else {
        deallocate(flags);
        return spider::unique_ptr<bool>();
    }
}

spider::array<spider::SyncInfo> spider::sched::TaskLauncher::buildExecConstraints(const Task *task) {
    const auto lrtCount = archi::platform()->LRTCount();
    size_t constraintsCount = 0;
    /* == Count the number of dependencies == */
    for (size_t i = 0; i < lrtCount; ++i) {
        constraintsCount += task->syncExecIxOnLRT(i) != UINT32_MAX;
    }
    /* == Now build the actual array of synchronization info == */
    auto result = spider::array<SyncInfo>(constraintsCount, StackID::RUNTIME);
    if (constraintsCount) {
        auto resultIt = std::begin(result);
        for (size_t i = 0; i < lrtCount; ++i) {
            if (task->syncExecIxOnLRT(i) != UINT32_MAX) {
                /* == Set this dependency as a synchronization constraint == */
                resultIt->lrtToWait_ = i;
                resultIt->jobToWait_ = task->syncExecIxOnLRT(i);
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

void spider::sched::TaskLauncher::updateNotificationFlags(const Task *task, bool *flags) const {
    for (size_t iOut = 0; iOut < task->successorCount(); ++iOut) {
        if (setFlagsFromSink(task, task->nextTask(iOut, schedule_), flags)) {
            return;
        }
    }
}

void spider::sched::TaskLauncher::updateNotificationFlags(const Task *task,
                                                          bool *flags,
                                                          const spider::vector<pisdf::DependencyIterator> &consDeps) const {
    for (const auto &depIt : consDeps) {
        for (const auto &dep : depIt) {
            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                auto *snkTask = dep.vertex_ ? schedule_->task(dep.handler_->getTaskIx(dep.vertex_, k)) : nullptr;
                if (setFlagsFromSink(task, snkTask, flags)) {
                    return;
                }
            }
        }
    }
}

bool spider::sched::TaskLauncher::setFlagsFromSink(const Task *task, const Task *sinkTask, bool *flags) {
    const auto mappedLRTIx = task->mappedLRT()->virtualIx();
    /* == Check if task are not ready == */
    if (!sinkTask || (sinkTask->state() != TaskState::READY &&
                      sinkTask->state() != TaskState::SKIPPED)) {
        /* == broadcast to every LRT == */
        std::fill(flags, flags + archi::platform()->LRTCount(), true);
        return true;
    }
    /* == Check if we are the one the sink task is synchronized on == */
    const auto snkMappedLRTIx = sinkTask->mappedLRT()->virtualIx();
    auto &currentFlag = flags[snkMappedLRTIx];
    if (!currentFlag && snkMappedLRTIx != mappedLRTIx) {
        currentFlag = sinkTask->syncExecIxOnLRT(mappedLRTIx) == UINT32_MAX ||
                      task->jobExecIx() >= sinkTask->syncExecIxOnLRT(mappedLRTIx);
    }
    return false;
}

void spider::sched::TaskLauncher::sendSyncTask(SyncTask *task, const JobMessage &message) {
    JobMessage syncMessage{ };
    /* == Set the synchronization flags == */
    syncMessage.synchronizationFlags_ = buildJobNotificationFlags(task);
    /* == Set the execution task constraints == */
    syncMessage.execConstraints_ = buildExecConstraints(task);
    /* == Set Fifos == */
    syncMessage.fifos_ = spider::make_unique<JobFifos, StackID::RUNTIME>(1, 1);
    auto fifo = message.fifos_->inputFifo(task->getDepIx());
    fifo.count_ = 0;
    fifo.attribute_ = FifoAttribute::RW_ONLY;
    syncMessage.fifos_->setInputFifo(0, fifo);
    if (task->syncType() == SyncType::RECEIVE) {
        /* == The receive task should allocate memory in the other memory interface == */
        fifo.count_ = 1;
        fifo.attribute_ = FifoAttribute::RW_OWN;
    }
    syncMessage.fifos_->setOutputFifo(0, fifo);
    /* == Set core properties == */
    syncMessage.nParamsOut_ = 0u;
    if (task->syncType() == SyncType::SEND) {
        syncMessage.kernelIx_ = static_cast<u32>(task->getMemoryBus()->sendKernel()->ix());
    } else {
        syncMessage.kernelIx_ = static_cast<u32>(task->getMemoryBus()->receiveKernel()->ix());
    }
    syncMessage.taskIx_ = task->ix();
    syncMessage.execIx_ = task->jobExecIx();
    /* == Set input params == */
    auto params = spider::allocate<i64, StackID::RUNTIME>(4u);
#ifndef NDEBUG
    if (!params) {
        throwNullptrException();
    }
#endif
    if (task->syncType() == SyncType::SEND) {
        const auto *fstLRT = task->mappedLRT();
        const auto *sndLRT = task->nextTask(0, nullptr)->mappedLRT();
        params[0u] = static_cast<i64>(fstLRT->cluster()->ix());
        params[1u] = static_cast<i64>(sndLRT->cluster()->ix());
        params[2u] = static_cast<i64>(fifo.size_);
        params[4u] = 0;
    } else {
        const auto *fstLRT = task->previousTask(0, nullptr)->mappedLRT();
        const auto *sndLRT = task->mappedLRT();
        params[0u] = static_cast<i64>(fstLRT->cluster()->ix());
        params[1u] = static_cast<i64>(sndLRT->cluster()->ix());
        params[2u] = static_cast<i64>(fifo.size_);
        params[3u] = static_cast<i64>(fifo.address_);
    }
    syncMessage.inputParams_ = spider::make_unique(params);
    /* == Send the job == */
    const auto grtIx = archi::platform()->getGRTIx();
    auto *communicator = rt::platform()->communicator();
    const auto mappedLRTIx = task->mappedLRT()->virtualIx();
    const auto messageIx = communicator->push(std::move(syncMessage), mappedLRTIx);
    communicator->push(Notification{ NotificationType::JOB_ADD, grtIx, messageIx }, mappedLRTIx);
    /* == Set job in TaskState::RUNNING == */
    task->setState(TaskState::RUNNING);
}
