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
#include <scheduling/allocator/TaskMemory.h>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::RTFifo
spider::NoSyncDefaultFifoAllocator::allocateDefaultVertexInputFifo(ScheduleTask *task, const pisdf::Edge *edge) {
    const auto snkIx = edge->sinkPortIx();
    const auto *inputTask = task->dependencies()[snkIx];
    if (!inputTask) {
        return RTFifo{ };
    } else if (inputTask->type() == TaskType::VERTEX) {
        if (inputTask->state() == TaskState::RUNNING) {
            /* == If input is a Fork, Duplicate or ExternInterface, then we replace the dependency in order to avoid useless sync == */
            const auto *srcVertex = inputTask->vertex();
            if (srcVertex && (srcVertex->subtype() == pisdf::VertexType::FORK ||
                              srcVertex->subtype() == pisdf::VertexType::DUPLICATE ||
                              srcVertex->subtype() == pisdf::VertexType::EXTERN_IN)) {
                auto *newInputTask = inputTask->dependencies()[0];
                task->setDependency(newInputTask, snkIx);
                task->updateExecutionConstraints();
            }
        }
        /* == Set the fifo == */
        auto fifo = inputTask->getOutputFifo(edge->sourcePortIx());
        fifo.attribute_ = (fifo.attribute_ == FifoAttribute::WRITE_EXT) ? FifoAttribute::READ_EXT
                                                                        : FifoAttribute::READ_OWN;
        return fifo;
    } else {
        auto fifo = inputTask->getOutputFifo(0U);
        fifo.attribute_ = (fifo.attribute_ == FifoAttribute::WRITE_EXT) ? FifoAttribute::READ_EXT
                                                                        : FifoAttribute::READ_OWN;
        return fifo;
    }
}

void spider::NoSyncDefaultFifoAllocator::allocateForkTask(ScheduleTask *task) {
    DefaultFifoAllocator::allocateForkTask(task);
    const auto *vertex = task->vertex();
    const auto *inputEdge = vertex->inputEdge(0U);
    const auto *previousTask = task->dependencies()[0];
    auto inputFifo = previousTask->getOutputFifo(inputEdge->sourcePortIx());
    if (inputFifo.attribute_ != FifoAttribute::WRITE_EXT) {
        if (previousTask->state() != TaskState::RUNNING) {
            inputFifo.count_ = task->taskMemory()->inputFifo(0).count_;
            previousTask->taskMemory()->setOutputFifo(inputEdge->sourcePortIx(), inputFifo);
        }
        /* ==
         *    In the case of the task being in RUNNING state, we could perform a MemoryInterface::read here.
         *    However, assuming an heterogeneous architecture, we may not be able to access the corresponding
         *    MemoryInterface from here. Thus, it seems to be a better solution to leave the synchronization point to
         *    take charge of that.
         * == */
    }
    if (previousTask->state() != TaskState::RUNNING) {
        task->setState(TaskState::RUNNING);
    }
}

void spider::NoSyncDefaultFifoAllocator::allocateDuplicateTask(ScheduleTask *task) {
    DefaultFifoAllocator::allocateDuplicateTask(task);
    const auto *vertex = task->vertex();
    const auto *inputEdge = vertex->inputEdge(0U);
    const auto *previousTask = task->dependencies()[0];
    auto inputFifo = previousTask->getOutputFifo(inputEdge->sourcePortIx());
    if (inputFifo.attribute_ != FifoAttribute::WRITE_EXT) {
        if (previousTask->state() != TaskState::RUNNING) {
            inputFifo.count_ = task->taskMemory()->inputFifo(0).count_;
            previousTask->taskMemory()->setOutputFifo(inputEdge->sourcePortIx(), inputFifo);
        }
        /* ==
         *    In the case of the task being in RUNNING state, we could perform a MemoryInterface::read here.
         *    However, assuming an heterogeneous architecture, we may not be able to access the corresponding
         *    MemoryInterface from here. Thus, it seems to be a better solution to leave the synchronization point to
         *    take charge of that.
         * == */
    }
    if (previousTask->state() != TaskState::RUNNING) {
        task->setState(TaskState::RUNNING);
    }
}

void spider::NoSyncDefaultFifoAllocator::allocateExternInTask(ScheduleTask *task) {
    DefaultFifoAllocator::allocateExternInTask(task);
    task->setState(TaskState::RUNNING);
}
