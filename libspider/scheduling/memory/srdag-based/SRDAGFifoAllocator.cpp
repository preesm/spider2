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

#include <scheduling/memory/srdag-based/SRDAGFifoAllocator.h>
#include <scheduling/task/SRDAGTask.h>
#include <archi/MemoryInterface.h>
#include <api/archi-api.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/srdag/SRDAGGraph.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <graphs/srdag/SRDAGEdge.h>

/* === Function(s) definition === */

void spider::sched::SRDAGFifoAllocator::allocate(SRDAGTask *task) {
    if (!task) {
        return;
    }
    /* == Allocating FIFOs == */
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
}


/* === Protected method(s) === */

void spider::sched::SRDAGFifoAllocator::allocateDefaultVertexTask(SRDAGTask *task) {
    const auto *vertex = task->vertex();
    for (const auto *edge : vertex->inputEdges()) {
        task->fifos().setInputFifo(edge->sinkPortIx(), allocateDefaultVertexInputFifo(task, edge));
    }
    for (const auto &edge : vertex->outputEdges()) {
        task->fifos().setOutputFifo(edge->sourcePortIx(), allocateDefaultVertexOutputFifo(edge));
    }
}

spider::Fifo
spider::sched::SRDAGFifoAllocator::allocateDefaultVertexInputFifo(SRDAGTask *task, const srdag::Edge *edge) const {
    const auto *inputTask = task->previousTask(edge->sinkPortIx());
    if (!inputTask) {
        return Fifo{ };
    } else {
        auto fifo = inputTask->getOutputFifo(edge->sourcePortIx());
        if (fifo.attribute_ != FifoAttribute::RW_EXT) {
            fifo.attribute_ = FifoAttribute::RW_OWN;
            fifo.count_ = 0;
        }
        return fifo;
    }
}

spider::Fifo spider::sched::SRDAGFifoAllocator::allocateDefaultVertexOutputFifo(const srdag::Edge *edge) {
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
        return FifoAllocator::allocate(static_cast<size_t>(size));
    }
}

void spider::sched::SRDAGFifoAllocator::allocateExternInTask(SRDAGTask *task) {
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

void spider::sched::SRDAGFifoAllocator::allocateForkTask(SRDAGTask *task) const {
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

void spider::sched::SRDAGFifoAllocator::allocateDuplicateTask(SRDAGTask *task)  const{
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

void spider::sched::SRDAGFifoAllocator::allocateRepeatTask(SRDAGTask *task) {
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

#endif

