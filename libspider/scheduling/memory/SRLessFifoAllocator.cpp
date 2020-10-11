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

#include <scheduling/memory/SRLessFifoAllocator.h>
#include <scheduling/task/Task.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Graph.h>
#include <archi/MemoryInterface.h>
#include <runtime/message/Notification.h>
#include <runtime/communicator/RTCommunicator.h>
#include <runtime/platform/RTPlatform.h>
#include <api/archi-api.h>
#include <api/runtime-api.h>

/* === Function(s) definition === */

void spider::sched::SRLessFifoAllocator::allocate(sched::Task *task) {
    if (!task) {
        return;
    }
    /* == Allocating input FIFOs == */
    size_t fifoIx{ 0u };
    size_t offset{ 0u };
    auto inputFifos = task->fifos().inputFifos();
    for (auto it = std::begin(inputFifos); it != std::end(inputFifos); ++it) {
        auto rule = task->allocationRuleForInputFifo(fifoIx);
        if (rule.type_ == AllocType::MERGE) {
            /* == Set the merged fifo == */
            offset += allocateMergedInputFifo(task, it, rule, fifoIx, offset);
            it += rule.offset_;
            if (it == std::end(inputFifos)) {
                break;
            }
        } else if (rule.type_ == AllocType::SAME_IN) {
            const auto *previousTask = task->previousTask(fifoIx + offset);
            allocateInputFifo(previousTask, it, rule);
        } else {
            throwSpiderException("invalid AllocAttribute for input FIFO.");
        }
        fifoIx++;
    }

    /* == Allocating output FIFOs == */
    fifoIx = 0u;
    auto outputFifos = task->fifos().outputFifos();
    for (auto &fifo : outputFifos) {
        const auto rule = task->allocationRuleForOutputFifo(fifoIx);
        switch (rule.type_) {
            case NEW:
                fifo.virtualAddress_ = virtualMemoryAddress_;
                virtualMemoryAddress_ += rule.size_;
                fifo.offset_ = 0u;
                break;
            case SAME_IN: {
                auto &inputFifo = inputFifos[rule.fifoIx_];
                fifo.virtualAddress_ = inputFifo.virtualAddress_;
                if (inputFifo.attribute_ == FifoAttribute::R_MERGE) {
                    fifo.offset_ = rule.offset_;
                } else {
                    fifo.offset_ = inputFifo.offset_ + rule.offset_;
                }
            }
                break;
            case SAME_OUT: {
                auto &outputFifo = outputFifos[rule.fifoIx_];
                fifo.virtualAddress_ = outputFifo.virtualAddress_;
                fifo.offset_ = outputFifo.offset_ + rule.offset_;
            }
                break;
            case EXT:
                fifo.virtualAddress_ = rule.offset_;
                fifo.offset_ = 0u;
                break;
            default:
                break;
        }
        fifo.size_ = rule.size_;
        fifo.attribute_ = rule.attribute_;
        fifo.count_ = rule.count_;
        fifoIx++;
    }
}

void spider::sched::SRLessFifoAllocator::clear() noexcept {
    FifoAllocator::clear();
    mergedFifos_.clear();
}

/* === Private methods === */

size_t spider::sched::SRLessFifoAllocator::allocateMergedInputFifo(Task *task,
                                                                   Fifo *fifo,
                                                                   AllocationRule &rule,
                                                                   size_t realFifoIx,
                                                                   size_t taskOffset) {
    /* == Search for existing same merged fifo == */
    for (const auto &mergedFifoInfo : mergedFifos_) {
        auto *mergedTask = mergedFifoInfo.task_;
        if (mergedTask->state() != TaskState::RUNNING) {
            const auto &mergedTaskInputFifos = mergedTask->fifos().inputFifos();
            auto existingFifo = mergedTaskInputFifos.at(mergedFifoInfo.ix_);
            if (existingFifo.size_ == rule.size_ && existingFifo.offset_ == rule.offset_) {
                /* == Let's check if it is the same merged fifo == */
                auto isSame = true;
                for (size_t i = 0; i < rule.offset_; ++i) {
                    const auto *prevTask = task->previousTask(realFifoIx + taskOffset + i);
                    const auto prevFifo = prevTask->fifos().outputFifo(rule.others_[i].fifoIx_);
                    const auto prevMergedFifo = mergedTaskInputFifos.at(mergedFifoInfo.ix_ + i + 1);
                    if ((prevFifo.virtualAddress_ != prevMergedFifo.virtualAddress_) ||
                        (prevFifo.size_ != prevMergedFifo.size_) ||
                        (prevFifo.offset_ != prevMergedFifo.offset_)) {
                        isSame = false;
                        break;
                    }
                }
                if (isSame) {
                    *fifo = existingFifo;
                    fifo->offset_ = 0u;
                    fifo->count_ = 0u;
                    fifo->attribute_ = FifoAttribute::RW_OWN;
                    for (size_t i = 0; i < rule.offset_; ++i) {
                        const auto *prevTask = task->previousTask(realFifoIx + taskOffset + i);
                        allocateInputFifo(prevTask, fifo + i + 1, rule.others_[i]);
                        (fifo + i + 1)->attribute_ = FifoAttribute::DUMMY;
                    }
                    destroy(rule.others_);
                    /* == Update info == */
                    existingFifo.count_ += 1;
                    mergedTask->fifos().setInputFifo(mergedFifoInfo.ix_, existingFifo);
                    return rule.offset_ - 1u;
                }
            }
        }
    }
    fifo->virtualAddress_ = virtualMemoryAddress_;
    virtualMemoryAddress_ += rule.size_;
    fifo->size_ = rule.size_;
    fifo->offset_ = rule.offset_;
    fifo->count_ = rule.count_;
    fifo->attribute_ = rule.attribute_;
    /* == do the other fifos == */
#ifndef NDEBUG
    if (!rule.others_) {
        throwNullptrException();
    }
#endif
    for (size_t i = 0; i < rule.offset_; ++i) {
        const auto *prevTask = task->previousTask(realFifoIx + taskOffset + i);
        allocateInputFifo(prevTask, fifo + i + 1, rule.others_[i]);
    }
    destroy(rule.others_);
    mergedFifos_.push_back({ realFifoIx, realFifoIx + taskOffset, task });
    return rule.offset_ - 1u;
}

void spider::sched::SRLessFifoAllocator::allocateInputFifo(const Task *task, Fifo *fifo, AllocationRule &rule) {
    if (task && (rule.attribute_ != FifoAttribute::DUMMY)) {
        *fifo = task->fifos().outputFifo(rule.fifoIx_);
        if (fifo->attribute_ != FifoAttribute::RW_EXT) {
            if (task->state() == TaskState::RUNNING) {
                /* == We are in the case of a vertex already executed, now we try to update its counter value == */
                auto tmp = task->allocationRuleForOutputFifo(rule.fifoIx_);
                if (tmp.count_ > fifo->count_) {
                    const auto diff = tmp.count_ - fifo->count_;
                    fifo->count_ = tmp.count_;
                    const auto sndIx = task->mappedLRT()->virtualIx();
                    auto addrNotifcation = Notification{ NotificationType::MEM_UPDATE_COUNT, sndIx,
                                                         fifo->virtualAddress_ };
                    auto countNotifcation = Notification{ NotificationType::MEM_UPDATE_COUNT, sndIx, diff };
                    rt::platform()->communicator()->push(addrNotifcation, sndIx);
                    rt::platform()->communicator()->push(countNotifcation, sndIx);
                    task->fifos().setOutputFifo(rule.fifoIx_, *fifo);
                }
            }
            fifo->count_ = 0;
            fifo->attribute_ = rule.attribute_;
        }
        fifo->size_ = rule.size_;
        fifo->offset_ += rule.offset_;
    } else {
        *fifo = Fifo{ };
    }
}


