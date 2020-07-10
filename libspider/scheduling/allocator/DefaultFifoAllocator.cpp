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

#include <scheduling/allocator/DefaultFifoAllocator.h>
#include <scheduling/schedule/ScheduleTask.h>
#include <scheduling/allocator/TaskMemory.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Graph.h>
#include <api/archi-api.h>

/* === Function(s) definition === */

spider::RTFifo spider::DefaultFifoAllocator::allocate(size_t size) {
    RTFifo fifo;
    fifo.size_ = static_cast<u32>(size);
    fifo.count_ = 1;
    fifo.offset_ = 0;
    fifo.virtualAddress_ = virtualMemoryAddress_;
    fifo.attribute_ = FifoAttribute::RW_OWN;
    if (log::enabled<log::MEMORY>()) {
        log::print<log::MEMORY>(log::green, "INFO:", "VIRTUAL: allocating %zu bytes at address %zu.\n", size,
                                virtualMemoryAddress_);
    }
    virtualMemoryAddress_ += size;
    return fifo;
}

void spider::DefaultFifoAllocator::allocate(ScheduleTask *task) {
    switch (task->type()) {
        case TaskType::VERTEX:
            allocateVertexTask(task);
            break;
        case TaskType::SYNC_SEND:
            allocateSendTask(task);
            break;
        case TaskType::SYNC_RECEIVE:
            allocateReceiveTask(task);
            break;
        default:
            break;
    }
}

void spider::DefaultFifoAllocator::clear() noexcept {
    virtualMemoryAddress_ = reservedMemory_;
}

void spider::DefaultFifoAllocator::allocatePersistentDelays(pisdf::Graph *graph) {
    const auto *grt = archi::platform()->spiderGRTPE();
    auto *interface = grt->cluster()->memoryInterface();
    for (const auto &edge : graph->edges()) {
        const auto &delay = edge->delay();
        if (delay && delay->isPersistent()) {
            const auto value = static_cast<size_t>(delay->value());
            auto *buffer = interface->allocate(static_cast<uint64_t>(reservedMemory_), value);
            memset(buffer, 0, value * sizeof(char));
            delay->setMemoryAddress(static_cast<uint64_t>(reservedMemory_));
            delay->setMemoryInterface(interface);
            log::info("Reserving #%.8ld bytes of memory.\n", value);
            reservedMemory_ += value;
        }
    }
    virtualMemoryAddress_ = reservedMemory_;
}

/* === Private method(s) implementation === */

void spider::DefaultFifoAllocator::allocateVertexTask(ScheduleTask *task) {
    const auto *vertex = task->vertex();
    if (!vertex) {
        // LCOV_IGNORE
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

void spider::DefaultFifoAllocator::allocateDefaultVertexTask(ScheduleTask *task) {
    const auto *vertex = task->vertex();
    auto taskMemory = make_unique<TaskMemory>(
            make<TaskMemory, StackID::SCHEDULE>(vertex->inputEdgeCount(), vertex->outputEdgeCount()));
    for (const auto &edge : vertex->inputEdgeVector()) {
        taskMemory->setInputFifo(edge->sinkPortIx(), allocateDefaultVertexInputFifo(task, edge));
    }
    for (const auto &edge : vertex->outputEdgeVector()) {
        taskMemory->setOutputFifo(edge->sourcePortIx(), allocateDefaultVertexOutputFifo(edge));
    }
    task->setTaskMemory(std::move(taskMemory));
}


spider::RTFifo
spider::DefaultFifoAllocator::allocateDefaultVertexInputFifo(ScheduleTask *task, const pisdf::Edge *edge) {
    const auto snkIx = edge->sinkPortIx();
    const auto &inputTask = task->dependencies()[snkIx];
    if (!inputTask) {
        return RTFifo{ };
    } else {
        const auto srcIx = (inputTask->type() == TaskType::VERTEX) ? edge->sourcePortIx() : 0U;
        auto fifo = inputTask->getOutputFifo(srcIx);
        if (fifo.attribute_ != FifoAttribute::RW_EXT) {
            fifo.attribute_ = FifoAttribute::RW_OWN;
        }
        return fifo;
    }
}

spider::RTFifo
spider::DefaultFifoAllocator::allocateDefaultVertexOutputFifo(const pisdf::Edge *edge) {
    const auto size = edge->sourceRateValue();
    const auto *sink = edge->sink();
    if (sink && sink->subtype() == pisdf::VertexType::EXTERN_OUT) {
        const auto *reference = sink->reference()->convertTo<pisdf::ExternInterface>();
        const auto index = reference->bufferIndex();
        RTFifo fifo{ };
        fifo.size_ = static_cast<u32>(size);
        fifo.count_ = 1;
        fifo.virtualAddress_ = index;
        fifo.offset_ = 0;
        fifo.attribute_ = FifoAttribute::RW_EXT;
        return fifo;
    } else {
        return allocate(static_cast<size_t>(size));
    }
}

void spider::DefaultFifoAllocator::allocateExternInTask(ScheduleTask *task) {
    const auto *vertex = task->vertex();
    const auto *reference = vertex->reference()->convertTo<pisdf::ExternInterface>();
    const auto index = reference->bufferIndex();
    auto taskMemory = make_unique<TaskMemory>(make<TaskMemory, StackID::SCHEDULE>(0U, 1U));
    RTFifo fifo{ };
    fifo.size_ = static_cast<u32>(vertex->outputEdge(0U)->sourceRateValue());
    fifo.count_ = 1;
    fifo.virtualAddress_ = index;
    fifo.offset_ = 0;
    fifo.attribute_ = FifoAttribute::RW_EXT;
    taskMemory->setOutputFifo(0U, fifo);
    task->setTaskMemory(std::move(taskMemory));
}

void spider::DefaultFifoAllocator::allocateRepeatTask(ScheduleTask *task) {
    const auto *vertex = task->vertex();
    const auto *inputEdge = vertex->inputEdge(0U);
    const auto *outputEdge = vertex->outputEdge(0U);
    if (inputEdge->sinkRateValue() == outputEdge->sourceRateValue()) {
        const auto *previousTask = task->dependencies()[0];
        auto inputFifo = previousTask->getOutputFifo(inputEdge->sourcePortIx());
        auto outputFifo = inputFifo;
        auto taskMemory = make_unique<TaskMemory>(make<TaskMemory, StackID::SCHEDULE>(1U, 1U));
        if (inputFifo.attribute_ != FifoAttribute::RW_EXT) {
            inputFifo.count_ = 2;
            inputFifo.attribute_ = FifoAttribute::RW_ONLY;
            outputFifo.attribute_ = FifoAttribute::RW_ONLY;
        }
        taskMemory->setInputFifo(0U, inputFifo);
        taskMemory->setOutputFifo(0U, outputFifo);
        task->setTaskMemory(std::move(taskMemory));
    } else {
        allocateDefaultVertexTask(task);
    }
}

void spider::DefaultFifoAllocator::allocateForkTask(ScheduleTask *task) {
    const auto *vertex = task->vertex();
    const auto *inputEdge = vertex->inputEdge(0U);
    const auto *previousTask = task->dependencies()[0];
    auto inputFifo = previousTask->getOutputFifo(inputEdge->sourcePortIx());
    auto taskMemory = make_unique<TaskMemory>(make<TaskMemory, StackID::SCHEDULE>(1U, vertex->outputEdgeCount()));
    u32 count = 0;
    u32 offset = 0;
    for (const auto &edge : vertex->outputEdgeVector()) {
        RTFifo fifo{ };
        fifo.size_ = static_cast<u32>(edge->sourceRateValue());
        fifo.count_ = 1;
        fifo.virtualAddress_ = inputFifo.virtualAddress_;
        fifo.offset_ = inputFifo.offset_ + offset;
        fifo.attribute_ = FifoAttribute::RW_ONLY;
        offset += fifo.size_;
        count += (fifo.size_ != 0);
        taskMemory->setOutputFifo(edge->sourcePortIx(), fifo);
    }
    if (inputFifo.attribute_ != FifoAttribute::RW_EXT) {
        inputFifo.attribute_ = FifoAttribute::RW_ONLY;
        inputFifo.count_ = count;
    }
    taskMemory->setInputFifo(0U, inputFifo);
    task->setTaskMemory(std::move(taskMemory));
}

void spider::DefaultFifoAllocator::allocateDuplicateTask(ScheduleTask *task) {
    const auto *vertex = task->vertex();
    const auto *inputEdge = vertex->inputEdge(0U);
    const auto *previousTask = task->dependencies()[0];
    auto inputFifo = previousTask->getOutputFifo(inputEdge->sourcePortIx());
    auto taskMemory = make_unique<TaskMemory>(make<TaskMemory, StackID::SCHEDULE>(1U, vertex->outputEdgeCount()));
    /* == Copy input fifo for each output == */
    for (size_t i = 0; i < vertex->outputEdgeCount(); ++i) {
        auto fifo = inputFifo;
        fifo.count_ = 1;
        fifo.attribute_ = FifoAttribute::RW_ONLY;
        taskMemory->setOutputFifo(i, fifo);
    }
    if (inputFifo.attribute_ != FifoAttribute::RW_EXT) {
        inputFifo.attribute_ = FifoAttribute::RW_ONLY;
        inputFifo.count_ = static_cast<u32>(inputFifo.size_ ? vertex->outputEdgeCount() : 0U);
    }
    taskMemory->setInputFifo(0U, inputFifo);
    task->setTaskMemory(std::move(taskMemory));
}

void spider::DefaultFifoAllocator::allocateReceiveTask(ScheduleTask *task) {
    const auto *information = task->comTaskInfo();
    if (!information) {
        throwNullptrException();
    }
    auto taskMemory = make_unique<TaskMemory>(make<TaskMemory, StackID::SCHEDULE>(0U, 1U));
    taskMemory->setOutputFifo(0U, allocate(static_cast<size_t>(information->size_)));
    task->setTaskMemory(std::move(taskMemory));
}

void spider::DefaultFifoAllocator::allocateSendTask(ScheduleTask *task) {
    const auto *information = task->comTaskInfo();
    if (!information) {
        throwNullptrException();
    }
    auto taskMemory = make_unique<TaskMemory>(make<TaskMemory, StackID::SCHEDULE>(1U, 1U));
    auto fifo = task->dependencies()[0]->getOutputFifo(static_cast<size_t>(information->inputPortIx_));
    if (fifo.attribute_ != FifoAttribute::RW_EXT) {
        fifo.attribute_ = FifoAttribute::RW_ONLY;
    }
    taskMemory->setInputFifo(0U, fifo);
    taskMemory->setOutputFifo(0U, fifo);
    task->setTaskMemory(std::move(taskMemory));
}
