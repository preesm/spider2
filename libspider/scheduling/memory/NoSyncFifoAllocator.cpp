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
#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <scheduling/memory/NoSyncFifoAllocator.h>
#include <scheduling/task/VertexTask.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <graphs/srdag/SRDAGEdge.h>
#include <api/archi-api.h>
#include <archi/MemoryInterface.h>

/* === Static function === */

static u32 countNonNullFifos(const spider::array_handle<spider::Fifo> &fifos) {
    return static_cast<u32>(std::count_if(std::begin(fifos), std::end(fifos),
                                          [](const spider::Fifo &fifo) {
                                              return fifo.size_ != 0;
                                          }));
}

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::Fifo spider::sched::NoSyncFifoAllocator::allocateDefaultVertexInputFifo(VertexTask *task,
                                                                                const srdag::Edge *edge) const {
    const auto *inputTask = task->previousTask(edge->sinkPortIx());
    if (!inputTask) {
        return Fifo{ };
    } else {
        if (inputTask->state() == TaskState::NOT_SCHEDULABLE) {
            replaceInputTask(task, inputTask, edge->sinkPortIx());
        }
        /* == Set the fifo == */
        auto fifo = inputTask->getOutputFifo(edge->sourcePortIx());
        if (fifo.attribute_ != FifoAttribute::RW_EXT) {
            fifo.attribute_ = FifoAttribute::RW_OWN;
            fifo.count_ = 0u;
        }
        return fifo;
    }
}

void spider::sched::NoSyncFifoAllocator::allocateForkTask(VertexTask *task) const {
    FifoAllocator::allocateForkTask(task);
    auto inputFifo = task->getInputFifo(0U);
    if (inputFifo.attribute_ != FifoAttribute::RW_EXT) {
        updateForkDuplicateInputTask(task);
    }
    if (task->previousTask(0U)->state() != TaskState::RUNNING) {
        task->setState(TaskState::NOT_SCHEDULABLE);
    }
}

void spider::sched::NoSyncFifoAllocator::allocateDuplicateTask(VertexTask *task) const {
    FifoAllocator::allocateDuplicateTask(task);
    auto inputFifo = task->getInputFifo(0U);
    if (inputFifo.attribute_ != FifoAttribute::RW_EXT) {
        updateForkDuplicateInputTask(task);
    }
    if (task->previousTask(0U)->state() != TaskState::RUNNING) {
        task->setState(TaskState::NOT_SCHEDULABLE);
    }
}

void spider::sched::NoSyncFifoAllocator::updateForkDuplicateInputTask(VertexTask *task) {
    auto *inputTask = task->previousTask(0U);
    if (inputTask->state() == TaskState::READY) {
        updateFifoCount(task, inputTask, countNonNullFifos(task->fifos().outputFifos()) - 1u);
    } else if (replaceInputTask(task, inputTask, 0U)) {
        updateFifoCount(inputTask, inputTask->previousTask(0u), countNonNullFifos(task->fifos().outputFifos()) - 1u);
    }
    /* ==
     *    In the case of the task being in RUNNING state, we could perform a MemoryInterface::read here.
     *    However, assuming an heterogeneous architecture, we may not be able to access the corresponding
     *    MemoryInterface from here. Thus, it seems to be a better solution to leave the synchronization point to
     *    take charge of that.
     * == */
}

void spider::sched::NoSyncFifoAllocator::updateFifoCount(const Task *task,
                                                         const Task *inputTask,
                                                         u32 count) {
    const auto rule = task->allocationRuleForInputFifo(0u);
    auto fifo = inputTask->getOutputFifo(rule.fifoIx_);
    fifo.count_ += count;
    inputTask->fifos().setOutputFifo(rule.fifoIx_, fifo);
}

bool spider::sched::NoSyncFifoAllocator::replaceInputTask(sched::Task *task,
                                                          const sched::Task *inputTask,
                                                          size_t ix) {
    if ((inputTask->state() != TaskState::RUNNING) && (inputTask->isSyncOptimizable())) {
        /* == If input is also optimizable, then we replace dependency to avoid cascade sync == */
        auto *newInputTask = inputTask->previousTask(0u);
        task->setExecutionDependency(ix, newInputTask);
        task->updateDependenciesNotificationFlag();
        return true;
    }
    return false;
}

#endif