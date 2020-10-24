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

#include <scheduling/task/PiSDFTask.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/memory/FifoAllocator.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs-tools/numerical/dependencies.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Graph.h>
#include <runtime/special-kernels/specialKernels.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/communicator/RTCommunicator.h>
#include <runtime/message/Notification.h>
#include <runtime-api.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::PiSDFTask::PiSDFTask(pisdf::GraphFiring *handler,
                                    const pisdf::Vertex *vertex,
                                    u32 firing,
                                    u32 depCount,
                                    u32 mergedFifoCount) :
        Task(),
        mergeFifoInfo_{ factory::vector<std::pair<size_t, size_t>>(StackID::RUNTIME) },
        handler_{ handler },
        vertex_{ vertex },
        firing_{ firing },
        dependenciesCount_{ depCount } {
    spider::reserve(mergeFifoInfo_, mergedFifoCount);
    dependencies_ = spider::make_unique(spider::allocate<Task *, StackID::SCHEDULE>(depCount));
    std::fill(dependencies_.get(), dependencies_.get() + depCount, nullptr);
}

void spider::sched::PiSDFTask::allocate(FifoAllocator *allocator) {
    allocator->allocate(this);
}

void spider::sched::PiSDFTask::updateTaskExecutionDependencies(const Schedule *schedule) {
    size_t i = 0u;
    for (const auto *edge : vertex_->inputEdges()) {
        const auto deps = spider::pisdf::computeExecDependency(vertex_, firing_, edge->sinkPortIx(), handler_);
        for (const auto &dep : deps) {
            if (dep.vertex_) {
                for (u32 k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
                    const auto taskIx = dep.handler_->getTaskIx(dep.vertex_, k);
//                    setExecutionDependency(i + k - dep.firingStart_, schedule->tasks()[taskIx].get());
                }
            }
            i += (dep.firingEnd_ - dep.firingStart_) + 1u;
        }
    }
}

spider::JobMessage spider::sched::PiSDFTask::createJobMessage() const {
    auto message = Task::createJobMessage();
    message.nParamsOut_ = static_cast<u32>(vertex_->outputParamCount());
    message.kernelIx_ = static_cast<u32>(vertex_->runtimeInformation()->kernelIx());
    /* == Set the input parameters (if any) == */
    message.inputParams_ = pisdf::buildVertexRuntimeInputParameters(vertex_, handler_->getParams());
    message.fifos_ = spider::make_shared<JobFifos, StackID::SCHEDULE>(vertex_->inputEdgeCount(),
                                                                      vertex_->outputEdgeCount());
    for (const auto *edge : vertex_->inputEdges()) {
        i32 count = 0;
        const auto deps = spider::pisdf::computeExecDependency(vertex_, firing_, edge->sinkPortIx(), handler_, &count);
        if (count > 1) {
            /* == Allocating a merged edge == */
            sendMergeFifoMessage(message, edge, deps, count);
        } else {
            message.fifos_->setInputFifo(edge->sinkPortIx(), allocateDefaultInputFifo(*deps.begin()));
        }
    }
    if (vertex_->subtype() == pisdf::VertexType::FORK) {
        auto inputFifo = message.fifos_->inputFifo(0);
        u32 offset = 0;
        for (const auto *edge : vertex_->outputEdges()) {
            auto fifo = inputFifo;
            fifo.size_ = static_cast<u32>(handler_->getSourceRate(edge));
            fifo.offset_ += offset;
            if (fifo.size_) {
                const auto ix = edge->sourcePortIx();
                fifo.count_ = spider::pisdf::computeConsDependencyCount(vertex_, firing_, ix, handler_);
            }
            fifo.attribute_ = FifoAttribute::RW_ONLY;
            offset += fifo.size_;
            message.fifos_->setOutputFifo(edge->sourcePortIx(), fifo);
            handler_->registerEdgeAlloc(fifo, edge, firing_);
        }
    } else if (vertex_->subtype() == pisdf::VertexType::DUPLICATE) {
        auto inputFifo = message.fifos_->inputFifo(0);
        for (const auto *edge : vertex_->outputEdges()) {
            auto fifo = inputFifo;
            if (fifo.size_) {
                const auto ix = edge->sourcePortIx();
                fifo.count_ = spider::pisdf::computeConsDependencyCount(vertex_, firing_, ix, handler_);
            }
            fifo.attribute_ = FifoAttribute::RW_ONLY;
            message.fifos_->setOutputFifo(edge->sourcePortIx(), fifo);
            handler_->registerEdgeAlloc(fifo, edge, firing_);
        }
    } else if (vertex_->subtype() == pisdf::VertexType::REPEAT) {
        const auto *inputEdge = vertex_->inputEdge(0U);
        const auto *outputEdge = vertex_->outputEdge(0U);
        if (handler_->getSinkRate(inputEdge) == handler_->getSourceRate(outputEdge)) {
            auto inputFifo = message.fifos_->inputFifo(0);
            auto outputFifo = inputFifo;
            auto count = spider::pisdf::computeConsDependencyCount(vertex_, firing_, 0U, handler_);
            outputFifo.count_ = outputFifo.size_ ? count : 0;
            if (outputFifo.attribute_ != FifoAttribute::RW_EXT) {
                if (count < 0 && outputFifo.size_) {
                    outputFifo.count_ = 1;
                    outputFifo.attribute_ = FifoAttribute::W_SINK;
                } else {
                    outputFifo.attribute_ = FifoAttribute::RW_ONLY;
                }
            }
            message.fifos_->setOutputFifo(0U, outputFifo);
            handler_->registerEdgeAlloc(outputFifo, vertex_->outputEdge(0U), firing_);
        } else {
            message.fifos_->setOutputFifo(0U, handler_->getEdgeAlloc(outputEdge, firing_));
        }
    } else if (vertex_->subtype() == pisdf::VertexType::EXTERN_IN) {
        Fifo fifo{ };
        fifo.size_ = static_cast<u32>(handler_->getSourceRate(vertex_->outputEdge(0)));
        fifo.offset_ = 0;
        fifo.count_ = 1;
        fifo.virtualAddress_ = vertex_->convertTo<pisdf::ExternInterface>()->bufferIndex();
        fifo.attribute_ = FifoAttribute::RW_EXT;
        message.fifos_->setOutputFifo(0U, fifo);
        handler_->registerEdgeAlloc(fifo, vertex_->outputEdge(0U), firing_);
    } else {
        for (const auto *edge : vertex_->outputEdges()) {
            message.fifos_->setOutputFifo(edge->sourcePortIx(), handler_->getEdgeAlloc(edge, firing_));
        }
    }
    return message;
}

u32 spider::sched::PiSDFTask::color() const {
    const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 3u) * 50 + 100);
    const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 2u) * 50 + 100);
    const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 4u) * 50 + 100);
    return 0u | (red << 16u) | (green << 8u) | (blue);
}

std::string spider::sched::PiSDFTask::name() const {
    std::string name{ };
    const auto *vertex = vertex_;
    const auto *handler = handler_;
    while (handler) {
        const auto *graph = vertex->graph();
        const auto firing = handler->firingValue();
        name = graph->name() + std::string(":").append(std::to_string(firing)).append(":").append(name);
        handler = handler->getParent()->handler();
        vertex = graph;
    }
    return name.append(vertex_->name()).append(":").append(std::to_string(firing_));
}

void spider::sched::PiSDFTask::setIx(u32 ix) noexcept {
    Task::setIx(ix);
    handler_->registerTaskIx(vertex_, firing_, ix);
}

std::pair<ufast64, ufast64> spider::sched::PiSDFTask::computeCommunicationCost(const spider::PE */*mappedPE*/) const {
    return { };
}

bool spider::sched::PiSDFTask::isMappableOnPE(const spider::PE *pe) const {
    return vertex_->runtimeInformation()->isPEMappable(pe);
}

u64 spider::sched::PiSDFTask::timingOnPE(const spider::PE *pe) const {
    return static_cast<u64>(vertex_->runtimeInformation()->timingOnPE(pe, handler_->getParams()));
}

spider::sched::DependencyInfo spider::sched::PiSDFTask::getDependencyInfo(size_t /*size*/) const {
    return { };
}

size_t spider::sched::PiSDFTask::dependencyCount() const {
    return dependenciesCount_;
}

void spider::sched::PiSDFTask::setOutputFifo(size_t ix, spider::Fifo fifo) {
    fifos_->setOutputFifo(ix, fifo);
    handler_->registerEdgeAlloc(fifo, vertex_->outputEdge(ix), firing_);
}

void spider::sched::PiSDFTask::addMergeFifoInfo(const pisdf::Edge *edge, size_t address) {
    mergeFifoInfo_.emplace_back(edge->sinkPortIx(), address);
}

/* === Private method(s) === */

spider::Fifo spider::sched::PiSDFTask::allocateDefaultInputFifo(const pisdf::DependencyInfo &dep) {
    if (!dep.vertex_) {
        return Fifo{ };
    } else {
        const auto *srcVertex = dep.vertex_;
        const auto *edge = srcVertex->outputEdge(dep.edgeIx_);
        auto fifo = dep.handler_->getEdgeAlloc(edge, dep.firingStart_);
        fifo.size_ = (dep.rate_ > 0) * (dep.memoryEnd_ - dep.memoryStart_ + 1u);
        fifo.offset_ += dep.memoryStart_;
        fifo.count_ = 0U;
        if (fifo.attribute_ == FifoAttribute::RW_EXT) {
            fifo.attribute_ = FifoAttribute::RW_EXT;
        } else if (fifo.attribute_ == FifoAttribute::RW_AUTO) {
            fifo.attribute_ = FifoAttribute::RW_AUTO;
        }
        return fifo;
    }
}

void spider::sched::PiSDFTask::sendMergeFifoMessage(JobMessage &jobMessage,
                                                    const pisdf::Edge *edge,
                                                    const pisdf::DependencyIterator &dependencies,
                                                    i32 depCount) const {
    /* == Create the actual job message == */
    JobMessage message{ };
    message.kernelIx_ = rt::END_KERNEL_IX;
    message.ix_ = ix_;
    message.taskIx_ = jobExecIx_;
    message.nParamsOut_ = 0u;
    message.execConstraints_ = jobMessage.execConstraints_;
    /* == Allocate the Fifos == */
    message.fifos_ = spider::make_shared<JobFifos, StackID::RUNTIME>(depCount + 1, 0);
    size_t fifoIx = 1u;
    size_t inputSize = 0u;
    for (const auto &dep : dependencies) {
        for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
            const auto *srcVertex = dep.vertex_;
            const auto *srcEdge = srcVertex->outputEdge(dep.edgeIx_);
            const auto memStart = k == dep.firingStart_ ? dep.memoryStart_ : 0;
            const auto memEnd = k == dep.firingEnd_ ? dep.memoryEnd_ : static_cast<u32>(dep.rate_) - 1;
            auto fifo = dep.handler_->getEdgeAlloc(srcEdge, k);
            fifo.size_ = memEnd - memStart + 1u;
            fifo.offset_ += memStart;
            fifo.count_ = 0;
            if (fifo.attribute_ == FifoAttribute::RW_EXT) {
                fifo.attribute_ = FifoAttribute::RW_EXT;
            } else if (fifo.attribute_ == FifoAttribute::RW_AUTO) {
                fifo.attribute_ = FifoAttribute::RW_AUTO;
            }
            message.fifos_->setInputFifo(fifoIx, fifo);
            inputSize += fifo.size_;
            fifoIx++;
        }
    }
    /* == Allocate merged fifo == */
    const auto mergedSize = static_cast<size_t>(handler_->getSinkRate(edge));
    if (inputSize != mergedSize) {
        throwSpiderException("invalid rate.");
    }
    auto fifo = Fifo{ };
    fifo.size_ = static_cast<u32>(mergedSize);
    fifo.offset_ = static_cast<u32>(fifoIx - 1);
    fifo.count_ = 1;
    fifo.virtualAddress_ = getMergeFifoAddress(edge);
    fifo.attribute_ = FifoAttribute::R_MERGE;
    message.fifos_->setInputFifo(0, fifo);
    /* == Set the input fifo of the task == */
    fifo.count_ = 0;
    fifo.offset_ = 0;
    fifo.attribute_ = FifoAttribute::RW_OWN;
    jobMessage.fifos_->setInputFifo(edge->sinkPortIx(), fifo);
    /* == Send the message == */
    const auto grtIx = archi::platform()->getGRTIx();
    const auto mappedLRTIx = mappedLRT()->virtualIx();
    auto *communicator = rt::platform()->communicator();
    const auto messageIx = communicator->push(std::move(message), mappedLRTIx);
    communicator->push(Notification{ NotificationType::JOB_ADD, grtIx, messageIx }, mappedLRTIx);
}

size_t spider::sched::PiSDFTask::getMergeFifoAddress(const pisdf::Edge *edge) const {
    for (const auto &elt : mergeFifoInfo_) {
        if (elt.first == edge->sinkPortIx()) {
            return elt.second;
        }
    }
    throwSpiderException("unable to find merged fifo address.");
}

