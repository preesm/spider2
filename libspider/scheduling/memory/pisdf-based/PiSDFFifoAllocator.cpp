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
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Graph.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <runtime/message/Notification.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/communicator/RTCommunicator.h>
#include <runtime-api.h>

/* === Function(s) definition === */

void spider::sched::PiSDFFifoAllocator::clear() noexcept {
    FifoAllocator::clear();
    dynamicBuffers_.clear();
}

void spider::sched::PiSDFFifoAllocator::updateDynamicBuffersCount() {
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
                                                  static_cast<size_t>(count - 1) };
            rt::platform()->communicator()->push(addrNotifcation, sndIx);
            rt::platform()->communicator()->push(countNotifcation, sndIx);
            spider::out_of_order_erase(dynamicBuffers_, it);
        } else {
            it++;
        }
    }
}

spider::unique_ptr<spider::JobFifos>
spider::sched::PiSDFFifoAllocator::buildJobFifos(PiSDFTask *task,
                                                 const pisdf::VertexDependencies &execDeps,
                                                 const pisdf::VertexDependencies &consDeps) {
    const auto *vertex = task->vertex();
    auto fifos = spider::make<JobFifos, StackID::RUNTIME>(computeInputFifoCount(execDeps),
                                                          static_cast<u32>(vertex->outputEdgeCount()));
    /* == Allocate input fifos == */
    auto *inputFifos = fifos->inputFifos().data();
    for (const auto *edge : vertex->inputEdges()) {
        const auto &depIt = execDeps[edge->sinkPortIx()];
        const auto depCount = depIt.total();
        if (depCount > 1) {
            buildMergeFifo(inputFifos, static_cast<size_t>(task->handler()->getSnkRate(edge)), depIt);
            inputFifos += depCount + 1;
        } else {
            buildSingleFifo(inputFifos, depIt[0]);
            inputFifos += 1;
        }
    }
    /* == Allocate output fifos == */
    allocate(task, fifos);
    for (const auto *edge : vertex->outputEdges()) {
        const auto edgeIx = edge->sourcePortIx();
        fifos->setOutputFifo(edgeIx, buildOutputFifo(edge, task, consDeps[edgeIx]));
    }
    return spider::make_unique(fifos);
}

/* === Private methods === */

void spider::sched::PiSDFFifoAllocator::allocate(PiSDFTask *task, const JobFifos *fifos) {
    auto *handler = task->handler();
    const auto *vertex = task->vertex();
    const auto firing = task->firing();
    switch (vertex->subtype()) {
        case pisdf::VertexType::FORK: {
            const auto inputFifo = fifos->inputFifo(0);
            auto offset = inputFifo.offset_ * (inputFifo.attribute_ != FifoAttribute::R_MERGE);
            for (auto *edge : vertex->outputEdges()) {
                handler->setEdgeAddress(inputFifo.address_, edge, firing);
                handler->setEdgeOffset(offset, edge, firing);
                offset += static_cast<u32>(handler->getSrcRate(edge));
            }
        }
            break;
        case pisdf::VertexType::DUPLICATE: {
            const auto inputFifo = fifos->inputFifo(0);
            auto offset = inputFifo.offset_ * (inputFifo.attribute_ != FifoAttribute::R_MERGE);
            for (auto *edge : vertex->outputEdges()) {
                handler->setEdgeAddress(inputFifo.address_, edge, firing);
                handler->setEdgeOffset(offset, edge, firing);
            }
        }
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
                    handler->setEdgeAddress(FifoAllocator::allocate(size * handler->getRV(vertex)), edge, firing);
                }
            }
            break;
    }
}

u32 spider::sched::PiSDFFifoAllocator::computeInputFifoCount(const pisdf::VertexDependencies &dependencies) {
    u32 inputCount = 0;
    u32 mergeCount = 0;
    for (const auto &depIt : dependencies) {
        const auto current = inputCount;
        for (const auto &dep : depIt) {
            inputCount += (dep.firingEnd_ - dep.firingStart_ + 1);
        }
        mergeCount += (inputCount > (current + 1));
    }
    return inputCount + mergeCount;
}

void spider::sched::PiSDFFifoAllocator::buildSingleFifo(Fifo *fifos, const pisdf::DependencyInfo &dep) {
    Fifo fifo{ };
    if (dep.vertex_) {
        const auto *srcEdge = dep.vertex_->outputEdge(dep.edgeIx_);
        const auto size = dep.memoryEnd_ - dep.memoryStart_ + 1;
        fifo = buildInputFifo(srcEdge, size, dep.memoryStart_, dep.firingStart_, dep.handler_);
    }
    fifos[0] = fifo;
}

void spider::sched::PiSDFFifoAllocator::buildMergeFifo(Fifo *fifos,
                                                       size_t mergedSize,
                                                       const pisdf::DependencyIterator &dependencies) {
    /* == Allocate the Fifos == */
    size_t fifoIx = 1u;
    size_t inputSize = 0u;
    for (const auto &dep : dependencies) {
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
    if (inputSize != mergedSize) {
        throwSpiderException("invalid rate.");
    }
    /* == Allocate merged fifo == */
    fifos[0].address_ = FifoAllocator::allocate(mergedSize);
    fifos[0].size_ = static_cast<u32>(mergedSize);
    fifos[0].offset_ = static_cast<u32>(fifoIx - 1);
    fifos[0].count_ = 1;
    fifos[0].attribute_ = FifoAttribute::R_MERGE;
}

spider::Fifo spider::sched::PiSDFFifoAllocator::buildInputFifo(const pisdf::Edge *edge,
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

spider::Fifo spider::sched::PiSDFFifoAllocator::buildOutputFifo(const pisdf::Edge *edge,
                                                                const PiSDFTask *task,
                                                                const pisdf::DependencyIterator &depIt) {
    Fifo fifo{ };
    auto *handler = task->handler();
    const auto firing = task->firing();
    fifo.address_ = handler->getEdgeAddress(edge, firing);
    fifo.offset_ = handler->getEdgeOffset(edge, firing);
    fifo.size_ = static_cast<u32>(handler->getSrcRate(edge));
    fifo.attribute_ = FifoAttribute::RW_OWN;
    fifo.count_ = getFifoCount(depIt);
    if (fifo.count_ < 0) {
        fifo.count_ = 1;
        dynamicBuffers_.push_back({ task, static_cast<u32>(edge->sourcePortIx()) });
    } else if (!fifo.count_) {
        fifo.count_ = 1;
        fifo.attribute_ = FifoAttribute::W_SINK;
    }
    /* == Set attribute == */
    const auto sourceSubType = edge->source()->subtype();
    const auto sinkSubType = edge->sink()->subtype();
    if (sourceSubType == pisdf::VertexType::EXTERN_IN || sinkSubType == pisdf::VertexType::EXTERN_OUT) {
        fifo.attribute_ = FifoAttribute::RW_EXT;
    } else if (sourceSubType == pisdf::VertexType::FORK || sourceSubType == pisdf::VertexType::DUPLICATE) {
        fifo.attribute_ = FifoAttribute::RW_ONLY;
    }
    return fifo;
}

i32 spider::sched::PiSDFFifoAllocator::getFifoCount(const spider::pisdf::DependencyIterator &depIt) {
    const auto count = depIt.total();
    if (count == SIZE_MAX) {
        return -1;
    }
    return static_cast<i32>(count);
}
