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

#include <scheduling/allocator/NoSyncDefaultFifoAllocator.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Vertex.h>
#include <scheduling/schedule/ScheduleTask.h>
#include <scheduling/task/TaskFifos.h>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::Fifo
spider::NoSyncDefaultFifoAllocator::allocateDefaultVertexInputFifo(ScheduleTask *task, const pisdf::Edge *edge) {
    const auto snkIx = edge->sinkPortIx();
    const auto *inputTask = task->dependencies()[snkIx];
    if (!inputTask) {
        return Fifo{ };
    } else if (inputTask->type() == TaskType::VERTEX) {
        if (inputTask->state() == TaskState::NOT_SCHEDULABLE) {
            replaceInputTask(task, inputTask, snkIx);
        }
        /* == Set the fifo == */
        auto fifo = inputTask->getOutputFifo(edge->sourcePortIx());
        if (fifo.attribute_ != FifoAttribute::RW_EXT) {
            fifo.attribute_ = FifoAttribute::RW_OWN;
        }
        return fifo;
    } else {
        auto fifo = inputTask->getOutputFifo(0U);
        if (fifo.attribute_ != FifoAttribute::RW_EXT) {
            fifo.attribute_ = FifoAttribute::RW_OWN;
        }
        return fifo;
    }
}

void spider::NoSyncDefaultFifoAllocator::allocateForkTask(ScheduleTask *task) {
    DefaultFifoAllocator::allocateForkTask(task);
    auto inputFifo = task->taskMemory()->inputFifo(0U);
    if (inputFifo.attribute_ != FifoAttribute::RW_EXT) {
        updateForkDuplicateInputTask(task);
    }
    if (task->dependencies()[0U]->state() != TaskState::RUNNING) {
        task->setState(TaskState::NOT_SCHEDULABLE);
    }
}

void spider::NoSyncDefaultFifoAllocator::allocateDuplicateTask(ScheduleTask *task) {
    DefaultFifoAllocator::allocateDuplicateTask(task);
    auto inputFifo = task->taskMemory()->inputFifo(0U);
    if (inputFifo.attribute_ != FifoAttribute::RW_EXT) {
        updateForkDuplicateInputTask(task);
    }
    if (task->dependencies()[0U]->state() != TaskState::RUNNING) {
        task->setState(TaskState::NOT_SCHEDULABLE);
    }
}

void spider::NoSyncDefaultFifoAllocator::updateForkDuplicateInputTask(ScheduleTask *task) const {
    const auto *inputTask = task->dependencies()[0U];
    if (inputTask->state() == TaskState::READY) {
        updateForkDuplicateInputFifoCount(task, task->vertex());
    } else if (replaceInputTask(task, inputTask, 0U)) {
        updateForkDuplicateInputFifoCount(task, inputTask->vertex());
    }
    /* ==
     *    In the case of the task being in RUNNING state, we could perform a MemoryInterface::read here.
     *    However, assuming an heterogeneous architecture, we may not be able to access the corresponding
     *    MemoryInterface from here. Thus, it seems to be a better solution to leave the synchronization point to
     *    take charge of that.
     * == */
}

void spider::NoSyncDefaultFifoAllocator::updateForkDuplicateInputFifoCount(const ScheduleTask *task,
                                                                           const pisdf::Vertex *vertex) const {
    /* == Update fifo count == */
    const auto *edge = vertex->inputEdge(0U);
    const auto *inputTask = task->dependencies()[0U];
    auto fifo = inputTask->getOutputFifo(edge->sourcePortIx());
    const auto *taskMemory = task->taskMemory();
    fifo.count_ += (taskMemory->inputFifo(0U).count_ - 1);

    /* == Replace the fifo == */
    auto *inputTaskMemory = inputTask->taskMemory();
    inputTaskMemory->setOutputFifo(edge->sourcePortIx(), fifo);
}

void spider::NoSyncDefaultFifoAllocator::allocateExternInTask(ScheduleTask *task) {
    DefaultFifoAllocator::allocateExternInTask(task);
    task->setState(TaskState::RUNNING);
}

bool spider::NoSyncDefaultFifoAllocator::replaceInputTask(ScheduleTask *task,
                                                          const ScheduleTask *oldInputTask,
                                                          size_t ix) const {
    if (oldInputTask->state() != TaskState::RUNNING) {
        /* == If input is a Fork, Duplicate or ExternInterface, then we replace the dependency in order to avoid useless sync == */
        const auto *srcVertex = oldInputTask->vertex();
        if (srcVertex && (srcVertex->subtype() == pisdf::VertexType::FORK ||
                          srcVertex->subtype() == pisdf::VertexType::DUPLICATE ||
                          srcVertex->subtype() == pisdf::VertexType::EXTERN_IN)) {
            auto *newInputTask = oldInputTask->dependencies()[0U];
            task->setDependency(newInputTask, ix);
            task->updateExecutionConstraints();
            return true;
        }
    }
    return false;
}


