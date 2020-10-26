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

#include <scheduling/task/pisdf-based/PiSDFTask.h>
#include <scheduling/task/pisdf-based/MergeTask.h>
#include <scheduling/task/SyncTask.h>
#include <scheduling/schedule/Schedule.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>
#include <runtime/common/Fifo.h>
#include <api/runtime-api.h>
#include <graphs-tools/numerical/dependencies.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::PiSDFTask::PiSDFTask(pisdf::GraphFiring *handler,
                                    const pisdf::Vertex *vertex,
                                    u32 firing) :
        Task(),
        handler_{ handler },
        firing_{ firing } {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    for (const auto *edge : vertex->outputEdges()) {
        const auto count = pisdf::computeConsDependencyCount(vertex, firing, edge->sourcePortIx(), handler);
        depOutCount_ += static_cast<u32>((count >= 0) * count);
    }
    vertexIx_ = static_cast<u32>(vertex->ix());
//    depIx = 0;
//    outputs_ = spider::make_unique(spider::make_n<u32, StackID::SCHEDULE>(depOutCount_, UINT32_MAX));
//    for (const auto *edge : vertex->outputEdges()) {
//        const auto deps = pisdf::computeConsDependency(vertex, firing, edge->sourcePortIx(), handler);
//        for (const auto &dep : deps) {
//            for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
//                const auto *snkTask = schedule->task(dep.handler_->getTaskIx(dep.vertex_, k));
//                outputs_.get()[depIx] = snkTask ? snkTask->ix() : UINT32_MAX;
//                depIx++;
//            }
//        }
//    }
}

spider::vector<spider::pisdf::DependencyIterator> spider::sched::PiSDFTask::computeAllDependencies() const {
    auto result = factory::vector<pisdf::DependencyIterator>(StackID::SCHEDULE);
    const auto *vertex = this->vertex();
    spider::reserve(result, vertex->inputEdgeCount());
    for (const auto *edge : vertex->inputEdges()) {
        result.emplace_back(pisdf::computeExecDependency(vertex, firing_, edge->sinkPortIx(), handler_));
    }
    return result;
}

void spider::sched::PiSDFTask::receiveParams(const spider::array<i64> &values) {
    const auto *vertex = this->vertex();
    if (vertex->subtype() != pisdf::VertexType::CONFIG) {
        throwSpiderException("Only config vertices can update parameter values.");
    }
    auto paramIterator = values.begin();
    for (const auto ix : vertex->outputParamIxVector()) {
        const auto value = *(paramIterator++);
        handler_->setParamValue(ix, value);
        if (log::enabled<log::TRANSFO>()) {
            log::info<log::TRANSFO>("Parameter [%12s]: received value #%" PRId64".\n",
                                    handler_->getParams()[ix]->name().c_str(), value);
        }
    }
}

void spider::sched::PiSDFTask::insertSyncTasks(SyncTask *sndTask, SyncTask *rcvTask, size_t ix, const Schedule *) {
    const auto fifo = fifos_->inputFifo(ix);
    sndTask->setAlloc(fifo);
    rcvTask->setAlloc(fifo);
}

i64 spider::sched::PiSDFTask::inputRate(size_t ix) const {
    return fifos_->inputFifo(ix).size_;
}

i64 spider::sched::PiSDFTask::outputRate(size_t ix) const {
    return fifos_->outputFifo(ix).size_;
}

spider::sched::Task *spider::sched::PiSDFTask::previousTask(size_t ix, const Schedule *schedule) const {
#ifndef NDEBUG
    if (ix >= depInCount_) {
        throwSpiderException("invalid dependency index.");
    }
#endif
    return schedule->task(inputs_.get()[ix]);
}

spider::sched::Task *spider::sched::PiSDFTask::nextTask(size_t ix, const Schedule *schedule) const {
#ifndef NDEBUG
    if (ix >= depOutCount_) {
        throwSpiderException("invalid dependency index.");
    }
#endif
    return schedule->task(outputs_.get()[ix]);
}

u32 spider::sched::PiSDFTask::color() const {
    const auto *vertex = this->vertex();
    const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex) >> 3u) * 50 + 100);
    const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex) >> 2u) * 50 + 100);
    const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex) >> 4u) * 50 + 100);
    return 0u | (red << 16u) | (green << 8u) | (blue);
}

std::string spider::sched::PiSDFTask::name() const {
    return vertex()->name();
}

u32 spider::sched::PiSDFTask::ix() const noexcept {
    return handler_->getTaskIx(vertex(), firing_);
}

void spider::sched::PiSDFTask::setIx(u32 ix) noexcept {
    handler_->registerTaskIx(vertex(), firing_, ix);
}

bool spider::sched::PiSDFTask::isMappableOnPE(const spider::PE *pe) const {
    return vertex()->runtimeInformation()->isPEMappable(pe);
}

u64 spider::sched::PiSDFTask::timingOnPE(const spider::PE *pe) const {
    return static_cast<u64>(vertex()->runtimeInformation()->timingOnPE(pe, handler_->getParams()));
}

size_t spider::sched::PiSDFTask::dependencyCount() const {
    return depInCount_;
}

size_t spider::sched::PiSDFTask::successorCount() const {
    return depOutCount_;
}

const spider::pisdf::Vertex *spider::sched::PiSDFTask::vertex() const {
    return handler_->vertex(vertexIx_);
}

/* === Private method(s) === */

u32 spider::sched::PiSDFTask::getOutputParamsCount() const {
    return static_cast<u32>(vertex()->outputParamCount());
}

u32 spider::sched::PiSDFTask::getKernelIx() const {
    return static_cast<u32>(vertex()->runtimeInformation()->kernelIx());
}

spider::unique_ptr<i64> spider::sched::PiSDFTask::buildInputParams() const {
    return pisdf::buildVertexRuntimeInputParameters(vertex(), handler_->getParams());
}

std::shared_ptr<spider::JobFifos> spider::sched::PiSDFTask::buildJobFifos(const Schedule *) const {
//    const auto *vertex = this->vertex();
//    auto fifos = spider::make_shared<JobFifos, StackID::RUNTIME>(vertex->inputEdgeCount(), vertex->outputEdgeCount());
//    /* == Allocate input fifos == */
//    for (const auto *edge : vertex->inputEdges()) {
//        fifos->setInputFifo(edge->sinkPortIx(), buildInputFifo(edge, schedule));
//    }
//    /* == Allocate output fifos == */
//    switch (vertex->subtype()) {
//        case pisdf::VertexType::FORK:
//            buildForkOutFifos(fifos->outputFifos().data(), fifos->inputFifo(0), schedule);
//            break;
//        case pisdf::VertexType::DUPLICATE:
//            buildDupOutFifos(fifos->outputFifos().data(), fifos->inputFifo(0), schedule);
//            break;
//        case pisdf::VertexType::EXTERN_IN:
//            buildExternINOutFifos(fifos->outputFifos().data(), schedule);
//            break;
//        default:
//            buildDefaultOutFifos(fifos->outputFifos().data(), schedule);
//            break;
//    }
    return fifos_;
}

spider::Fifo spider::sched::PiSDFTask::buildInputFifo(const pisdf::Edge *edge, const Schedule *schedule) const {
//    i32 count = 0;
//    const auto deps = spider::pisdf::computeExecDependency(vertex_, firing_, edge->sinkPortIx(), handler_, &count);
//    if (count > 1) {
//        /* == Allocating a merged edge == */
//        const auto *mergeTask = static_cast<MergeTask *>(previousTask(edge->sinkPortIx(), schedule));
//        Fifo fifo{ };
//        fifo.virtualAddress_ = mergeTask->outputAlloc();
//        fifo.size_ = static_cast<u32>(mergeTask->outputRate(0));
//        fifo.offset_ = 0;
//        fifo.count_ = 0;
//        fifo.attribute_ = FifoAttribute::RW_OWN;
//        return fifo;
//    } else {
//        const auto &dep = *(deps.begin());
//        Fifo fifo{ };
//        if (dep.vertex_) {
//            const auto *srcVertex = dep.vertex_;
//            const auto *srcEdge = srcVertex->outputEdge(dep.edgeIx_);
//            fifo.virtualAddress_ = dep.handler_->getEdgeAlloc(srcEdge);
//            fifo.virtualAddress_ += static_cast<size_t>(dep.firingStart_ * dep.handler_->getSourceRate(srcEdge));
//            fifo.size_ = (dep.rate_ > 0) * (dep.memoryEnd_ - dep.memoryStart_ + 1u);
//            fifo.offset_ += dep.memoryStart_;
//            fifo.count_ = 0U;
//            const auto *sourceTask = schedule->task(dep.handler_->getTaskIx(srcVertex, dep.firingStart_));
//            if (sourceTask && (sourceTask->state() == TaskState::SKIPPED ||
//                               sourceTask->state() == TaskState::RUNNING)) {
//                fifo.attribute_ = FifoAttribute::RW_AUTO;
//            }
//            if (srcVertex->subtype() == pisdf::VertexType::EXTERN_IN) {
//                fifo.attribute_ = FifoAttribute::RW_EXT;
//            } else if (srcVertex->subtype() == pisdf::VertexType::FORK) {
//                for (size_t ix = 0; ix < dep.edgeIx_; ++ix) {
//                    const auto *outEdge = srcVertex->outputEdge(ix);
//                    fifo.offset_ += static_cast<u32>(dep.handler_->getSourceRate(outEdge));
//                }
//            }
//            if ((fifo.attribute_ != FifoAttribute::RW_EXT) && (fifo.attribute_ != FifoAttribute::RW_AUTO)) {
//                fifo.attribute_ = FifoAttribute::RW_OWN;
//            }
//        }
//        return fifo;
//    }
}

void spider::sched::PiSDFTask::buildDefaultOutFifos(Fifo *outputFifos, const Schedule *) const {
//    for (const auto *edge : vertex_->outputEdges()) {
//        auto &fifo = outputFifos[edge->sourcePortIx()];
//        const auto *sink = edge->sink();
//        const auto rate = static_cast<u32>(handler_->getSourceRate(edge));
//        if (sink && sink->subtype() == pisdf::VertexType::EXTERN_OUT) {
//            fifo.size_ = rate;
//            fifo.offset_ = 0;
//            fifo.count_ = fifo.size_ ? 1 : 0;
//            fifo.virtualAddress_ = sink->convertTo<pisdf::ExternInterface>()->bufferIndex();
//            fifo.attribute_ = FifoAttribute::RW_EXT;
//        } else {
//            fifo.virtualAddress_ = handler_->getEdgeAlloc(edge);
//            fifo.virtualAddress_ += static_cast<size_t>(firing_ * handler_->getSourceRate(edge));
//            fifo.size_ = static_cast<u32>(handler_->getSourceRate(edge));
//            fifo.offset_ = 0;
//            fifo.count_ = fifo.size_ ? 1 : 0;
//            fifo.attribute_ = FifoAttribute::RW_OWN;
//            auto consCount = pisdf::computeConsDependencyCount(vertex_, firing_, edge->sourcePortIx(), handler_);
//            if (!consCount) {
//                /* == Dynamic case, the FIFO will be automatically managed == */
//                fifo.count_ = -1;
//                fifo.attribute_ = FifoAttribute::RW_AUTO;
//            } else if (consCount < 0) {
//                fifo.attribute_ = FifoAttribute::W_SINK;
//            } else {
//                fifo.count_ = consCount;
//            }
//        }
//    }
}

void spider::sched::PiSDFTask::buildExternINOutFifos(Fifo *outputFifos, const Schedule *) const {
//    auto &fifo = outputFifos[0];
//    fifo.virtualAddress_ = vertex_->convertTo<pisdf::ExternInterface>()->bufferIndex();
//    fifo.size_ = static_cast<u32>(handler_->getSourceRate(vertex_->outputEdge(0)));
//    fifo.offset_ = 0;
//    fifo.count_ = fifo.size_ ? 1 : 0;
//    fifo.attribute_ = FifoAttribute::RW_ONLY;
}

void spider::sched::PiSDFTask::buildForkOutFifos(Fifo *outputFifos, Fifo inputFifo, const Schedule *) const {
//    u32 offset = 0;
//    for (const auto *edge : vertex_->outputEdges()) {
//        auto &fifo = outputFifos[edge->sourcePortIx()];
//        fifo.virtualAddress_ = handler_->getEdgeAlloc(edge);
//        fifo.virtualAddress_ += static_cast<size_t>(firing_ * handler_->getSourceRate(edge));
//        fifo.size_ = static_cast<u32>(handler_->getSourceRate(edge));
//        fifo.offset_ = inputFifo.offset_ + offset;
//        fifo.count_ = fifo.size_ ? 1 : 0;
//        fifo.attribute_ = FifoAttribute::RW_ONLY;
//        auto consCount = pisdf::computeConsDependencyCount(vertex_, firing_, edge->sourcePortIx(), handler_);
//        if (!consCount) {
//            /* == Dynamic case, the FIFO will be automatically managed == */
//            fifo.count_ = -1;
//            fifo.attribute_ = FifoAttribute::RW_AUTO;
//        } else if (consCount < 0) {
//            fifo.attribute_ = FifoAttribute::W_SINK;
//        } else {
//            fifo.count_ = consCount;
//        }
//        offset += fifo.size_;
//    }
}

void spider::sched::PiSDFTask::buildDupOutFifos(Fifo *outputFifos, Fifo inputFifo, const Schedule *) const {
//    for (const auto *edge : vertex_->outputEdges()) {
//        auto &fifo = outputFifos[edge->sourcePortIx()];
//        fifo = inputFifo;
//        fifo.count_ = fifo.size_ ? 1 : 0;
//        auto consCount = pisdf::computeConsDependencyCount(vertex_, firing_, edge->sourcePortIx(), handler_);
//        if (!consCount) {
//            /* == Dynamic case, the FIFO will be automatically managed == */
//            fifo.count_ = -1;
//            fifo.attribute_ = FifoAttribute::RW_AUTO;
//        } else if (consCount < 0) {
//            fifo.attribute_ = FifoAttribute::W_SINK;
//        } else {
//            fifo.count_ = consCount;
//        }
//        fifo.attribute_ = FifoAttribute::RW_ONLY;
//    }
}


