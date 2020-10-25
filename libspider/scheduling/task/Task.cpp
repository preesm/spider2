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
#include <archi/Platform.h>
#include <api/archi-api.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/communicator/RTCommunicator.h>
#include <api/runtime-api.h>
#include <algorithm>

/* === Method(s) implementation === */

std::pair<ufast64, ufast64> spider::sched::Task::computeCommunicationCost(const PE *mappedPE,
                                                                          const Schedule *schedule) const {
    const auto *platform = archi::platform();
    ufast64 externDataToReceive = 0u;
    /* == Compute communication cost == */
    ufast64 communicationCost = 0;
    for (size_t ix = 0; ix < this->dependencyCount(); ++ix) {
        const auto *source = this->previousTask(ix, schedule);
        const auto rate = static_cast<ufast64>(this->inputRate(ix));
        if (rate && source && source->state() != TaskState::NOT_RUNNABLE) {
            const auto *mappedPESource = source->mappedPe();
            communicationCost += platform->dataCommunicationCostPEToPE(mappedPESource, mappedPE, rate);
            if (mappedPE->cluster() != mappedPESource->cluster()) {
                externDataToReceive += rate;
            }
        }
    }
    return { communicationCost, externDataToReceive };
}

void spider::sched::Task::send(const Schedule *schedule) {
    if (state_ != TaskState::READY) {
        return;
    }
    JobMessage message{ };
    /* == Set core properties == */
    message.nParamsOut_ = this->getOutputParamsCount();
    message.kernelIx_ = this->getKernelIx();
    message.taskIx_ = ix_;
    message.ix_ = jobExecIx_;
    /* == Set the synchronization flags == */
    message.synchronizationFlags_ = this->buildJobNotificationFlags(schedule);
    /* == Set the execution task constraints == */
    message.execConstraints_ = this->buildExecConstraints(schedule);
    /* == Set input params == */
    message.inputParams_ = this->buildInputParams();
    /* == Set Fifos == */
    message.fifos_ = this->buildJobFifos(schedule);
    /* == Send the job == */
    const auto grtIx = archi::platform()->getGRTIx();
    auto *communicator = rt::platform()->communicator();
    const auto mappedLRTIx = mappedLRT()->virtualIx();
    const auto messageIx = communicator->push(std::move(message), mappedLRTIx);
    communicator->push(Notification{ NotificationType::JOB_ADD, grtIx, messageIx }, mappedLRTIx);
    /* == Set job in TaskState::RUNNING == */
    state_ = TaskState::RUNNING;
}

/* === Private method(s) === */

spider::unique_ptr<bool> spider::sched::Task::buildJobNotificationFlags(const Schedule *schedule) const {
    /* == Check if task are not ready == */
    const auto shouldBroadcast = this->shouldBroadCast(schedule);
    if (!shouldBroadcast) {
        /* == Update values == */
        const auto lrtCount{ archi::platform()->LRTCount() };
        auto flags = spider::make_n<bool, StackID::RUNTIME>(lrtCount, false);
        if (this->updateNotificationFlags(flags, schedule)) {
            return make_unique(flags);
        } else {
            deallocate(flags);
            return spider::unique_ptr<bool>();
        }
    } else {
        /* == broadcast to every LRT == */
        return make_unique(spider::make_n<bool, StackID::RUNTIME>(archi::platform()->LRTCount(), true));
    }
}

spider::array<spider::SyncInfo> spider::sched::Task::buildExecConstraints(const Schedule *schedule) const {
    /* == Get the number of actual execution constraints == */
    auto shouldNotifyArray = array<size_t>(archi::platform()->LRTCount(), SIZE_MAX, StackID::RUNTIME);
    size_t numberOfConstraints{ 0u };
    for (size_t ix = 0; ix < this->dependencyCount(); ++ix) {
        const auto *source = this->previousTask(ix, schedule);
        if (source && (source->mappedLRT() != mappedLRT())) {
            // TODO: handle SKIPPED source
            const auto sourceLRTIx = source->mappedLRT()->virtualIx();
            const auto currentDepIxOnLRT = shouldNotifyArray[sourceLRTIx];
            const auto gotConstraintOnLRT = currentDepIxOnLRT != SIZE_MAX;
            if (!gotConstraintOnLRT ||
                (source->jobExecIx_ > this->previousTask(currentDepIxOnLRT, schedule)->jobExecIx_)) {
                numberOfConstraints += !gotConstraintOnLRT;
                shouldNotifyArray[sourceLRTIx] = ix;
            }
        }

    }
    /* == Now build the actual array of synchronization info == */
    auto result = array<SyncInfo>(numberOfConstraints, StackID::RUNTIME);
    if (!numberOfConstraints) {
        return result;
    }
    auto resultIt = std::begin(result);
    for (const auto depIx : shouldNotifyArray) {
        if (depIx != SIZE_MAX) {
            auto *dependency = this->previousTask(depIx, schedule);
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
    return result;
}

bool spider::sched::Task::updateNotificationFlags(bool *flags, const Schedule *schedule) const {
    auto oneTrue = false;
    for (size_t iOut = 0; iOut < this->successorCount(); ++iOut) {
        const auto *sinkTask = this->nextTask(iOut, schedule);
        if (sinkTask->state() == TaskState::SKIPPED) {
            sinkTask->updateNotificationFlags(flags, schedule);
        }
        auto &currentFlag = flags[sinkTask->mappedLRT()->virtualIx()];
        if (!currentFlag) {
            currentFlag = true;
            for (size_t ix = 0; ix < sinkTask->dependencyCount(); ++ix) {
                auto *sourceTask = sinkTask->previousTask(ix, schedule);
                if (sourceTask && (sourceTask->mappedLRT() == mappedLRT()) && (sourceTask->jobExecIx() > jobExecIx())) {
                    currentFlag = false;
                    break;
                }
            }
        }
        oneTrue |= currentFlag;
    }
    return oneTrue;
}

bool spider::sched::Task::shouldBroadCast(const Schedule *schedule) const {
    for (size_t iOut = 0; iOut < this->successorCount(); ++iOut) {
        const auto *sinkTask = this->nextTask(iOut, schedule);
        if (!sinkTask || (sinkTask->state() != TaskState::READY && sinkTask->state() != TaskState::SKIPPED)) {
            return true;
        }
    }
    return false;
}
