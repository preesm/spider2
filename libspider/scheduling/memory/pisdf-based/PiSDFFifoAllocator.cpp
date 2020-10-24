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

#include <scheduling/memory/pisdf-based/PiSDFFifoAllocator.h>
#include <scheduling/task/PiSDFTask.h>
#include <graphs-tools/numerical/dependencies.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Graph.h>
#include <archi/MemoryInterface.h>
#include <api/archi-api.h>
#include <runtime/message/Notification.h>
#include <runtime/special-kernels/specialKernels.h>

/* === Function(s) definition === */

void spider::sched::PiSDFFifoAllocator::allocate(PiSDFTask *task) {
    if (!task) {
        return;
    }
    /* == Allocating FIFOs == */
    const auto *vertex = task->vertex();
    switch (vertex->subtype()) {
        case pisdf::VertexType::FORK:
        case pisdf::VertexType::DUPLICATE:
            if (spider::pisdf::computeExecDependencyCount(vertex,
                                                          task->vertexFiring(),
                                                          0U,
                                                          task->handler()) > 1) {
                /* == Allocating a merged edge == */
                const auto *edge = vertex->inputEdge(0U);
                task->addMergeFifoInfo(edge, virtualMemoryAddress_);
                virtualMemoryAddress_ += static_cast<size_t>(task->handler()->getSinkRate(edge));
            }
            break;
        case pisdf::VertexType::EXTERN_IN:
            break;
        case pisdf::VertexType::REPEAT:
            allocateRepeatTask(task);
            break;
        default:
            allocateDefaultVertexTask(task);
            break;
    }
}

/* === Private methods === */

void spider::sched::PiSDFFifoAllocator::allocateDefaultVertexTask(PiSDFTask *task) {
    const auto *vertex = task->vertex();
    const auto firing = task->vertexFiring();
    auto *handler = task->handler();
    for (const auto *edge : vertex->inputEdges()) {
        const auto count = spider::pisdf::computeExecDependencyCount(vertex, firing, edge->sinkPortIx(), handler);
        if (count > 1) {
            /* == Allocating a merged edge == */
            task->addMergeFifoInfo(edge, virtualMemoryAddress_);
            virtualMemoryAddress_ += static_cast<size_t>(handler->getSinkRate(edge));
        }
    }
    for (const auto *edge : vertex->outputEdges()) {
        handler->registerEdgeAlloc(allocateDefaultVertexOutputFifo(task, edge), edge, firing);
    }
}

void spider::sched::PiSDFFifoAllocator::allocateRepeatTask(PiSDFTask *task) {
    auto *handler = task->handler();
    const auto *vertex = task->vertex();
    const auto *inputEdge = vertex->inputEdge(0U);
    const auto *outputEdge = vertex->outputEdge(0U);
    if (handler->getSinkRate(inputEdge) != handler->getSourceRate(outputEdge)) {
        allocateDefaultVertexTask(task);
    }
}

spider::Fifo
spider::sched::PiSDFFifoAllocator::allocateDefaultVertexOutputFifo(PiSDFTask *task, const pisdf::Edge *edge) {
    const auto *sink = edge->sink();
    const auto *vertex = task->vertex();
    const auto firing = task->vertexFiring();
    auto *handler = task->handler();
    const auto rate = static_cast<u32>(handler->getSourceRate(edge));
    Fifo fifo{ };
    if (sink && sink->subtype() == pisdf::VertexType::EXTERN_OUT) {
        const auto *reference = sink->convertTo<pisdf::ExternInterface>();
        fifo.size_ = rate;
        fifo.offset_ = 0;
        fifo.count_ = fifo.size_ ? 1 : 0;
        fifo.virtualAddress_ = reference->bufferIndex();
        fifo.attribute_ = FifoAttribute::RW_EXT;
    } else {
        fifo = FifoAllocator::allocate(rate);
        auto consCount = spider::pisdf::computeConsDependencyCount(vertex, firing, edge->sourcePortIx(), handler);
        if (!consCount) {
            /* == Dynamic case, the FIFO will be automatically managed == */
            fifo.count_ = -1;
            fifo.attribute_ = FifoAttribute::RW_AUTO;
        } else if (consCount < 0) {
            fifo.attribute_ = FifoAttribute::W_SINK;
        } else {
            fifo.count_ = consCount;
        }
    }
    return fifo;
}
