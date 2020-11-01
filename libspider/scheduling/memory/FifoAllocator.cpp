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
#include <scheduling/task/PiSDFTask.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <archi/MemoryInterface.h>
#include <api/archi-api.h>
#include <runtime/message/Notification.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/communicator/RTCommunicator.h>
#include <runtime-api.h>

#ifndef _NO_BUILD_LEGACY_RT

#include <graphs/srdag/SRDAGVertex.h>
#include <graphs/srdag/SRDAGEdge.h>
#include <scheduling/task/SRDAGTask.h>
#include <runtime/special-kernels/specialKernels.h>

#endif

/* === Function(s) definition === */

void spider::sched::FifoAllocator::clear() noexcept {
    virtualMemoryAddress_ = reservedMemory_;
}

#ifndef _NO_BUILD_LEGACY_RT

void spider::sched::FifoAllocator::allocate(SRDAGTask *task) {
    u32 offset = 0;
    const auto *vertex = task->vertex();
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
                    const auto *ext = edge->sink()->reference()->convertTo<pisdf::ExternInterface>();
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

void spider::sched::FifoAllocator::allocate(PiSDFTask *task) {
    auto *handler = task->handler();
    const auto *vertex = task->vertex();
    const auto firing = task->firing();
    switch (vertex->subtype()) {
        case pisdf::VertexType::FORK:
        case pisdf::VertexType::DUPLICATE:
            break;
        case pisdf::VertexType::EXTERN_IN:
            if (handler->getEdgeAddress(vertex->outputEdge(0), firing) == SIZE_MAX) {
                if (vertex->subtype() == pisdf::VertexType::EXTERN_IN) {
                    const auto *ext = vertex->convertTo<pisdf::ExternInterface>();
                    handler->setEdgeAddress(ext->address(), vertex->outputEdge(0), firing);
                }
            }
            break;
        default:
            for (auto *edge : vertex->outputEdges()) {
                if (edge->sink()->subtype() == pisdf::VertexType::EXTERN_OUT) {
                    const auto *ext = edge->sink()->convertTo<pisdf::ExternInterface>();
                    handler->setEdgeAddress(ext->address(), edge, firing);
                } else {
                    const auto size = static_cast<size_t>(handler->getSrcRate(edge));
                    handler->setEdgeAddress(allocate(size * handler->getRV(vertex)), edge, firing);
                }
            }
            break;
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

void spider::sched::FifoAllocator::updateDynamicBuffersCount() {
    /* == We are in the case of a vertex already executed, now we try to update its counter value == */
    auto it = std::begin(dynamicBuffers_);
    while (it != std::end(dynamicBuffers_)) {
        const auto *task = it->task_;
        const auto *vertex = task->vertex();
        const auto *edge = vertex->outputEdge(it->edgeIx_);
        const auto count = pisdf::computeConsDependencyCount(vertex, task->firing(), it->edgeIx_, task->handler());
        if (count > 0) {
            const auto sndIx = task->mappedLRT()->virtualIx();
            const auto grtIx = archi::platform()->spiderGRTPE()->virtualIx();
            const auto address = task->handler()->getEdgeAddress(edge, task->firing());
            auto addrNotifcation = Notification{ NotificationType::MEM_UPDATE_COUNT, grtIx, address };
            auto countNotifcation = Notification{ NotificationType::MEM_UPDATE_COUNT, grtIx,
                                                  static_cast<size_t>(count) };
            rt::platform()->communicator()->push(addrNotifcation, sndIx);
            rt::platform()->communicator()->push(countNotifcation, sndIx);
            spider::out_of_order_erase(dynamicBuffers_, it);
        } else {
            it++;
        }
    }
}

#ifndef _NO_BUILD_LEGACY_RT

spider::unique_ptr<spider::JobFifos> spider::sched::FifoAllocator::buildJobFifos(SRDAGTask *task) {
    const auto *vertex = task->vertex();
    auto fifos = spider::make_unique<JobFifos, StackID::RUNTIME>(static_cast<u32>(vertex->inputEdgeCount()),
                                                                 static_cast<u32>(vertex->outputEdgeCount()));
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

spider::unique_ptr<spider::JobFifos>
spider::sched::FifoAllocator::buildJobFifos(PiSDFTask *task,
                                            const spider::vector<pisdf::DependencyIterator> &execDeps,
                                            const spider::vector<pisdf::DependencyIterator> &consDeps) {
    const auto *vertex = task->vertex();
    u32 inputCount = 0;
    u32 mergeCount = 0;
    for (const auto &depIt : execDeps) {
        const auto current = inputCount;
        for (const auto &dep : depIt) {
            inputCount += (dep.firingEnd_ - dep.firingStart_ + 1);
        }
        mergeCount += (inputCount > (current + 1));
    }
    auto fifos = spider::make<JobFifos, StackID::RUNTIME>(inputCount + mergeCount,
                                                          static_cast<u32>(vertex->outputEdgeCount()));
    /* == Allocate input fifos == */
    auto *inputFifos = fifos->inputFifos().data();
    size_t fifoIx = 0;
    for (const auto *edge : vertex->inputEdges()) {
        const auto &depIt = execDeps[edge->sinkPortIx()];
        if (depIt.total() > 1) {
            auto mergedFifo = buildMergeFifo(inputFifos + fifoIx, task, edge, depIt);
            fifos->setInputFifo(fifoIx, mergedFifo);
            fifoIx += (mergedFifo.offset_ + 1);
        } else {
            const auto &dep = *depIt.begin();
            if (dep.vertex_) {
                const auto *srcEdge = dep.vertex_->outputEdge(dep.edgeIx_);
                const auto size = dep.memoryEnd_ - dep.memoryStart_ + 1;
                fifos->setInputFifo(fifoIx,
                                    buildInputFifo(srcEdge, size, dep.memoryStart_, dep.firingStart_, dep.handler_));
            } else {
                fifos->setInputFifo(fifoIx, Fifo{ });
            }
            fifoIx++;
        }
    }
    /* == Allocate output fifos == */
    for (const auto *edge : vertex->outputEdges()) {
        const auto edgeIx = edge->sourcePortIx();
        fifos->setOutputFifo(edgeIx, buildOutputFifo(fifos, edge, task, consDeps[edgeIx]));
    }
    return spider::make_unique(fifos);
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

spider::Fifo spider::sched::FifoAllocator::buildInputFifo(const pisdf::Edge *edge,
                                                          u32 size,
                                                          u32 offset,
                                                          u32 firing,
                                                          const pisdf::GraphFiring *handler) {
    Fifo fifo{ };
    fifo.address_ = handler->getEdgeAddress(edge, firing);
    fifo.offset_ = handler->getEdgeOffset(edge, firing) + offset;
    fifo.size_ = size;
    fifo.count_ = 0;
    fifo.attribute_ = FifoAttribute::RW_OWN;
    if (edge->source()->subtype() == pisdf::VertexType::EXTERN_IN ||
        edge->sink()->subtype() == pisdf::VertexType::EXTERN_OUT) {
        fifo.attribute_ = FifoAttribute::RW_EXT;
    }
    return fifo;
}

spider::Fifo spider::sched::FifoAllocator::buildOutputFifo(const JobFifos *fifos,
                                                           const pisdf::Edge *edge,
                                                           const PiSDFTask *task,
                                                           const pisdf::DependencyIterator &depIt) {
    Fifo fifo{ };
    auto *handler = task->handler();
    const auto firing = task->firing();
    fifo.address_ = handler->getEdgeAddress(edge, firing);
    fifo.offset_ = 0;
    fifo.size_ = static_cast<u32>(handler->getSrcRate(edge));
    fifo.attribute_ = FifoAttribute::RW_OWN;
    fifo.count_ = getFifoCount(depIt);
    if (fifo.count_ < 0) {
        fifo.count_ = 1;
        dynamicBuffers_.push_back({ task, static_cast<u32>(edge->sourcePortIx()) });
    } else if (fifo.count_ == INT32_MAX) {
        fifo.count_ = 1;
        fifo.attribute_ = FifoAttribute::W_SINK;
    }
    /* == Set attribute == */
    const auto sourceSubType = edge->source()->subtype();
    const auto sinkSubType = edge->sink()->subtype();
    if (sourceSubType == pisdf::VertexType::EXTERN_IN || sinkSubType == pisdf::VertexType::EXTERN_OUT) {
        fifo.attribute_ = FifoAttribute::RW_EXT;
    } else if (sourceSubType == pisdf::VertexType::FORK || sourceSubType == pisdf::VertexType::DUPLICATE) {
        auto &inputFifo = fifos->inputFifos().data()[0];
        fifo.address_ = inputFifo.address_;
        fifo.offset_ = inputFifo.offset_ * (inputFifo.attribute_ != FifoAttribute::R_MERGE);
        fifo.attribute_ = FifoAttribute::RW_ONLY;
//        if (inputFifo.attribute_ != FifoAttribute::R_MERGE) {
//            inputFifo.attribute_ = FifoAttribute::RW_ONLY;
//        }
        if (sourceSubType == pisdf::VertexType::FORK) {
            const auto *vertex = edge->source();
            for (size_t i = 0; i < edge->sourcePortIx(); ++i) {
                fifo.offset_ += static_cast<u32>(handler->getSrcRate(vertex->outputEdge(i)));
            }
        }
        handler->setEdgeAddress(fifo.address_, edge, firing);
        handler->setEdgeOffset(fifo.offset_, edge, firing);
    }
    return fifo;
}

i32 spider::sched::FifoAllocator::getFifoCount(const spider::pisdf::DependencyIterator &depIt) {
    if (!depIt.count()) {
        return INT32_MAX;
    } else if (depIt.begin()->rate_ < 0) {
        return -1;
    }
    i32 count = 0;
    for (const auto &dep : depIt) {
        count += (dep.firingEnd_ - dep.firingStart_ + 1);
    }
    if (!count) {
        return INT32_MAX;
    }
    return count;
}

spider::Fifo spider::sched::FifoAllocator::buildMergeFifo(Fifo *fifos,
                                                          const PiSDFTask *task,
                                                          const pisdf::Edge *edge,
                                                          const pisdf::DependencyIterator &depIt) {
    /* == Allocate the Fifos == */
    size_t fifoIx = 1u;
    size_t inputSize = 0u;
    for (const auto &dep : depIt) {
        for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
            const auto *srcVertex = dep.vertex_;
            const auto *srcEdge = srcVertex->outputEdge(dep.edgeIx_);
            const auto memStart = k == dep.firingStart_ ? dep.memoryStart_ : 0;
            const auto memEnd = k == dep.firingEnd_ ? dep.memoryEnd_ : static_cast<u32>(dep.rate_) - 1;
            fifos[fifoIx] = buildInputFifo(srcEdge, memEnd - memStart + 1, memStart, k, dep.handler_);
            inputSize += fifos[fifoIx].size_;
            fifoIx++;
        }
    }
    const auto mergedSize = static_cast<size_t>(task->handler()->getSnkRate(edge));
    if (inputSize != mergedSize) {
        throwSpiderException("invalid rate.");
    }
    /* == Allocate merged fifo == */
    Fifo fifo{ };
    fifo.address_ = virtualMemoryAddress_;
    fifo.size_ = static_cast<u32>(task->handler()->getSnkRate(edge));
    fifo.offset_ = static_cast<u32>(fifoIx - 1);
    fifo.count_ = 1;
    fifo.attribute_ = FifoAttribute::R_MERGE;
    virtualMemoryAddress_ += fifo.size_;
    return fifo;
}
