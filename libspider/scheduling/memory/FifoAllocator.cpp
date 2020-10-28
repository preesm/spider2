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
#include <scheduling/schedule/Schedule.h>
#include <scheduling/task/SyncTask.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <archi/MemoryInterface.h>
#include <api/archi-api.h>

#ifndef _NO_BUILD_LEGACY_RT

#include <graphs/srdag/SRDAGVertex.h>
#include <graphs/srdag/SRDAGEdge.h>
#include <scheduling/task/SRDAGTask.h>

#endif

/* === Function(s) definition === */

void spider::sched::FifoAllocator::clear() noexcept {
    virtualMemoryAddress_ = reservedMemory_;
}

#ifndef _NO_BUILD_LEGACY_RT

void spider::sched::FifoAllocator::allocate(srdag::Vertex *vertex) {
    u32 offset = 0;
    switch (vertex->subtype()) {
        case pisdf::VertexType::EXTERN_IN: {
            const auto *ext = vertex->reference()->convertTo<pisdf::ExternInterface>();
            vertex->outputEdge(0)->setAddress(ext->address());
        }
            break;
        case pisdf::VertexType::FORK:
            for (auto *edge : vertex->outputEdges()) {
                const auto *inputEdge = vertex->inputEdge(0);
                edge->setAddress(inputEdge->address());
                edge->setOffset(inputEdge->offset() + offset);
                offset += static_cast<u32>(edge->rate());
            }
            break;
        case pisdf::VertexType::DUPLICATE:
            for (auto *edge : vertex->outputEdges()) {
                const auto *inputEdge = vertex->inputEdge(0);
                edge->setAddress(inputEdge->address());
                edge->setOffset(inputEdge->offset());
            }
            break;
        default:
            for (auto *edge : vertex->outputEdges()) {
                if (edge->sink()->subtype() == pisdf::VertexType::EXTERN_OUT) {
                    const auto *ext = vertex->reference()->convertTo<pisdf::ExternInterface>();
                    edge->setAddress(ext->address());
                } else {
                    edge->setAddress(allocate(static_cast<size_t>(edge->rate())));
                    edge->setOffset(0);
                }
            }
            break;
    }
}

#endif

void spider::sched::FifoAllocator::allocate(pisdf::GraphFiring *handler, const pisdf::Vertex *vertex) {
    for (auto *edge : vertex->outputEdges()) {
        if (handler->getEdgeAlloc(edge) == SIZE_MAX) {
            if (vertex->subtype() == pisdf::VertexType::FORK ||
                vertex->subtype() == pisdf::VertexType::DUPLICATE) {
                handler->registerEdgeAlloc(handler->getEdgeAlloc(vertex->inputEdge(0)), edge);
            } else if (vertex->subtype() != pisdf::VertexType::EXTERN_IN) {
                const auto size = static_cast<size_t>(handler->getSourceRate(edge));
                handler->registerEdgeAlloc(allocate(size * handler->getRV(vertex)), edge);
            }
        }
    }
}

size_t spider::sched::FifoAllocator::allocate(size_t size) {
    const auto address = virtualMemoryAddress_;
    if (log::enabled<log::MEMORY>()) {
        log::print<log::MEMORY>(log::green, "INFO:", "VIRTUAL: allocating %zu bytes at address %zu.\n", size,
                                address);
    }
    virtualMemoryAddress_ += size;
    return address;
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


#ifndef _NO_BUILD_LEGACY_RT

spider::unique_ptr<spider::JobFifos> spider::sched::FifoAllocator::buildJobFifos(SRDAGTask *task) const {
    const auto *vertex = task->vertex();
    auto fifos = spider::make_unique<JobFifos, StackID::RUNTIME>(vertex->inputEdgeCount(), vertex->outputEdgeCount());
    /* == Allocate input fifos == */
    for (const auto *edge : vertex->inputEdges()) {
        fifos->setInputFifo(edge->sinkPortIx(), buildInputFifo(edge));
    }
    /* == Allocate output fifos == */
    for (const auto *edge : vertex->outputEdges()) {
        fifos->setOutputFifo(edge->sourcePortIx(), buildOutputFifo(edge));
    }
    return fifos;
}

#endif

spider::unique_ptr<spider::JobFifos> spider::sched::FifoAllocator::buildJobFifos(SyncTask *task) const {
    auto fifos = spider::make_unique<JobFifos, StackID::RUNTIME>(1, 1);
    Fifo fifo{ };
    fifo.address_ = task->getAllocAddress();
    fifo.offset_ = task->getAllocOffset();
    fifo.size_ = static_cast<u32>(task->size());
    fifo.count_ = 0;
    fifo.attribute_ = FifoAttribute::RW_ONLY;
    fifos->setInputFifo(0, fifo);
    if (task->syncType() == SyncType::RECEIVE) {
        /* == The receive task should allocate memory in the other memory interface == */
        fifo.count_ = 1;
        fifo.attribute_ = FifoAttribute::RW_OWN;
    }
    fifos->setOutputFifo(0, fifo);
    return fifos;
}

/* === Private method(s) === */

#ifndef _NO_BUILD_LEGACY_RT

spider::Fifo spider::sched::FifoAllocator::buildInputFifo(const srdag::Edge *edge) {
    Fifo fifo{ };
    fifo.address_ = edge->address();
    fifo.offset_ = edge->offset();
    fifo.size_ = static_cast<u32>(edge->rate());
    fifo.count_ = 0;
    fifo.attribute_ = FifoAttribute::RW_OWN;
    if (edge->source()->subtype() == pisdf::VertexType::EXTERN_IN ||
        edge->sink()->subtype() == pisdf::VertexType::EXTERN_OUT) {
        fifo.attribute_ = FifoAttribute::RW_EXT;
    }
    return fifo;
}

spider::Fifo spider::sched::FifoAllocator::buildOutputFifo(const srdag::Edge *edge) {
    Fifo fifo{ };
    fifo.address_ = edge->address();
    fifo.offset_ = edge->offset();
    fifo.size_ = static_cast<u32>(edge->rate());
    fifo.count_ = 1;
    fifo.attribute_ = FifoAttribute::RW_OWN;
    if (edge->source()->subtype() == pisdf::VertexType::EXTERN_IN ||
        edge->sink()->subtype() == pisdf::VertexType::EXTERN_OUT) {
        fifo.attribute_ = FifoAttribute::RW_EXT;
    } else if (edge->source()->subtype() == pisdf::VertexType::FORK ||
               edge->source()->subtype() == pisdf::VertexType::DUPLICATE) {
        fifo.attribute_ = FifoAttribute::RW_ONLY;
    }
    return fifo;
}

#endif
