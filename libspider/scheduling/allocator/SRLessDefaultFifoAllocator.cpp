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

#include <scheduling/allocator/SRLessDefaultFifoAllocator.h>
#include <scheduling/schedule/ScheduleTask.h>
#include <scheduling/allocator/TaskMemory.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Graph.h>
#include <api/archi-api.h>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

void spider::SRLessDefaultFifoAllocator::allocateVertexTask(spider::ScheduleTask *task) {
    auto *vertex = task->vertex();
    if (!vertex) {
        throwNullptrException();
    }
    switch (vertex->subtype()) {
        case pisdf::VertexType::REPEAT:
            allocateRepeatTask(task);
            break;
        case pisdf::VertexType::FORK:
            allocateForkTask(task);
            break;
        case pisdf::VertexType::DUPLICATE:
            allocateDuplicateTask(task);
            break;
        case pisdf::VertexType::EXTERN_IN:
            allocateExternInTask(task);
            break;
        default:
            allocateDefaultVertexTask(task);
            break;
    }
}

void spider::SRLessDefaultFifoAllocator::allocateDefaultVertexTask(spider::ScheduleTask *task) {
    const auto *vertex = task->vertex();
    auto taskMemory = make_unique<TaskMemory>(
            make<TaskMemory, StackID::SCHEDULE>(vertex->inputEdgeCount(), vertex->outputEdgeCount()));
    for (const auto &edge : vertex->inputEdgeVector()) {
        const auto snkIx = edge->sinkPortIx();
        const auto &inputTask = task->dependencies()[snkIx];
        if (!inputTask) {
            taskMemory->setInputFifo(snkIx, RTFifo{ });
        } else {
            const auto srcIx = (inputTask->type() == TaskType::VERTEX) ? edge->sourcePortIx() : 0u;
            auto fifo = inputTask->getOutputFifo(srcIx);
            if (fifo.attribute_ != FifoAttribute::RW_EXT) {
                fifo.attribute_ = FifoAttribute::RW_OWN;
            }
            taskMemory->setInputFifo(snkIx, fifo);
        }
    }

    for (const auto &edge : vertex->outputEdgeVector()) {
        const auto size = edge->sourceRateValue();
        taskMemory->setOutputFifo(edge->sourcePortIx(), allocate(static_cast<size_t>(size)));
    }
    task->setTaskMemory(std::move(taskMemory));
}

void spider::SRLessDefaultFifoAllocator::allocateRepeatTask(spider::ScheduleTask *) {

}

void spider::SRLessDefaultFifoAllocator::allocateForkTask(spider::ScheduleTask *) {

}

void spider::SRLessDefaultFifoAllocator::allocateDuplicateTask(spider::ScheduleTask *) {

}

void spider::SRLessDefaultFifoAllocator::allocateExternInTask(spider::ScheduleTask *) {

}

void spider::SRLessDefaultFifoAllocator::allocateReceiveTask(spider::ScheduleTask *) {

}

void spider::SRLessDefaultFifoAllocator::allocateSendTask(spider::ScheduleTask *) {

}
