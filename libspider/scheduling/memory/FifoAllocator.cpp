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

#include <scheduling/memory/FifoAllocator.h>
#include <scheduling/task/SyncTask.h>
#include <archi/MemoryInterface.h>
#include <api/archi-api.h>
#include <graphs/pisdf/ExternInterface.h>

#ifndef _NO_BUILD_LEGACY_RT

#include <scheduling/task/VertexTask.h>
#include <graphs/srdag/SRDAGGraph.h>
#include <graphs/srdag/SRDAGEdge.h>

#endif

/* === Function(s) definition === */

void spider::sched::FifoAllocator::clear() noexcept {
    virtualMemoryAddress_ = reservedMemory_;
}

void spider::sched::FifoAllocator::allocatePersistentDelays(pisdf::Graph *graph) {
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

void spider::sched::FifoAllocator::allocate(VertexTask *task) {
    if (!task) {
        return;
    }
#ifndef _NO_BUILD_LEGACY_RT
    /* == Allocating output FIFOs == */
    const auto *vertex = task->vertex();
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
#endif
}

void spider::sched::FifoAllocator::allocate(SyncTask *task) {
    if (!task) {
        return;
    }
    if (task->syncType() == RECEIVE) {
        task->fifos().setOutputFifo(0, allocateNewFifo(task->size()));
    } else {
        auto fifo = task->previousTask(0)->getOutputFifo(task->inputPortIx());
        if (fifo.attribute_ != FifoAttribute::RW_EXT) {
            fifo.attribute_ = FifoAttribute::RW_ONLY;
        }
        task->fifos().setInputFifo(0, fifo);
        task->fifos().setOutputFifo(0, fifo);
    }
}

/* === Protected method(s) === */

spider::Fifo spider::sched::FifoAllocator::allocateNewFifo(size_t size) {
    Fifo fifo{ };
    fifo.size_ = static_cast<u32>(size);
    fifo.offset_ = 0;
    fifo.count_ = size ? 1 : 0;
    fifo.virtualAddress_ = virtualMemoryAddress_;
    fifo.attribute_ = FifoAttribute::RW_OWN;
    if (log::enabled<log::MEMORY>()) {
        log::print<log::MEMORY>(log::green, "INFO:", "VIRTUAL: allocating %zu bytes at address %zu.\n", size,
                                virtualMemoryAddress_);
    }
    virtualMemoryAddress_ += size;
    return fifo;
}

void spider::sched::FifoAllocator::allocateDefaultVertexTask(VertexTask *task) {
    const auto *vertex = task->vertex();
    for (const auto *edge : vertex->inputEdges()) {
        task->fifos().setInputFifo(edge->sinkPortIx(), allocateDefaultVertexInputFifo(task, edge));
    }
    for (const auto &edge : vertex->outputEdges()) {
        task->fifos().setOutputFifo(edge->sourcePortIx(), allocateDefaultVertexOutputFifo(edge));
    }
}

spider::Fifo
spider::sched::FifoAllocator::allocateDefaultVertexInputFifo(VertexTask *task, const srdag::Edge *edge) const {
    const auto *inputTask = task->previousTask(edge->sinkPortIx());
    if (!inputTask) {
        return Fifo{ };
    } else {
        auto fifo = inputTask->getOutputFifo(edge->sourcePortIx());
        if (fifo.attribute_ != FifoAttribute::RW_EXT) {
            fifo.attribute_ = FifoAttribute::RW_OWN;
            fifo.count_ = 0u;
        }
        return fifo;
    }
}

spider::Fifo spider::sched::FifoAllocator::allocateDefaultVertexOutputFifo(const srdag::Edge *edge) {
    const auto size = edge->sourceRateValue();
    const auto *sink = edge->sink();
    if (sink && sink->subtype() == pisdf::VertexType::EXTERN_OUT) {
        const auto *reference = sink->reference()->convertTo<pisdf::ExternInterface>();
        Fifo fifo{ };
        fifo.size_ = static_cast<u32>(size);
        fifo.offset_ = 0;
        fifo.count_ = size ? 1 : 0;
        fifo.virtualAddress_ = reference->bufferIndex();
        fifo.attribute_ = FifoAttribute::RW_EXT;
        return fifo;
    } else {
        return allocateNewFifo(static_cast<size_t>(size));
    }
}

void spider::sched::FifoAllocator::allocateExternInTask(VertexTask *task) {
    const auto *vertex = task->vertex();
    const auto *reference = vertex->reference()->convertTo<pisdf::ExternInterface>();
    Fifo fifo{ };
    fifo.size_ = static_cast<u32>(vertex->outputEdge(0U)->sourceRateValue());
    fifo.offset_ = 0;
    fifo.count_ = fifo.size_ ? 1 : 0;
    fifo.virtualAddress_ = reference->bufferIndex();
    fifo.attribute_ = FifoAttribute::RW_EXT;
    task->fifos().setOutputFifo(0U, fifo);
}

void spider::sched::FifoAllocator::allocateForkTask(VertexTask *task) const {
    const auto *vertex = task->vertex();
    const auto *inputEdge = vertex->inputEdge(0U);
    const auto *previousTask = task->previousTask(0U);
    auto inputFifo = previousTask->getOutputFifo(inputEdge->sourcePortIx());
    u32 offset = 0;
    for (const auto *edge : vertex->outputEdges()) {
        Fifo fifo{ };
        fifo.size_ = static_cast<u32>(edge->sourceRateValue());
        fifo.offset_ = inputFifo.offset_ + offset;
        fifo.count_ = fifo.size_ ? 1 : 0;
        fifo.virtualAddress_ = inputFifo.virtualAddress_;
        fifo.attribute_ = FifoAttribute::RW_ONLY;
        offset += fifo.size_;
        task->fifos().setOutputFifo(edge->sourcePortIx(), fifo);
    }
    task->fifos().setInputFifo(0U, allocateDefaultVertexInputFifo(task, inputEdge));
}

void spider::sched::FifoAllocator::allocateDuplicateTask(VertexTask *task)  const{
    const auto *vertex = task->vertex();
    const auto *inputEdge = vertex->inputEdge(0U);
    const auto *previousTask = task->previousTask(0U);
    auto inputFifo = previousTask->getOutputFifo(inputEdge->sourcePortIx());
    for (const auto *edge : vertex->outputEdges()) {
        auto fifo = inputFifo;
        fifo.count_ = fifo.size_ ? 1 : 0;
        fifo.attribute_ = FifoAttribute::RW_ONLY;
        task->fifos().setOutputFifo(edge->sourcePortIx(), fifo);
    }
    task->fifos().setInputFifo(0U, allocateDefaultVertexInputFifo(task, inputEdge));
}

void spider::sched::FifoAllocator::allocateRepeatTask(VertexTask *task) {
    const auto *vertex = task->vertex();
    const auto *inputEdge = vertex->inputEdge(0U);
    const auto *outputEdge = vertex->outputEdge(0U);
    if (inputEdge->sinkRateValue() == outputEdge->sourceRateValue()) {
        const auto *previousTask = task->previousTask(0U);
        auto outputFifo = previousTask->fifos().inputFifo(inputEdge->sourcePortIx());
        outputFifo.count_ = outputFifo.size_ ? 1 : 0;
        if (outputFifo.attribute_ != FifoAttribute::RW_EXT) {
            outputFifo.attribute_ = FifoAttribute::RW_ONLY;
        }
        task->fifos().setInputFifo(0U, allocateDefaultVertexInputFifo(task, inputEdge));
        task->fifos().setOutputFifo(0U, outputFifo);
    } else {
        allocateDefaultVertexTask(task);
    }
}
