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

#include <scheduling/task/SRDAGTask.h>
#include <scheduling/task/SyncTask.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/memory/FifoAllocator.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/srdag/SRDAGGraph.h>
#include <graphs/srdag/SRDAGEdge.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <graphs-tools/helper/srdag-helper.h>
#include <runtime/common/Fifo.h>
#include <api/runtime-api.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::SRDAGTask::SRDAGTask(srdag::Vertex *vertex) : Task(), vertex_{ vertex } {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
}

void spider::sched::SRDAGTask::receiveParams(const spider::array<i64> &values) {
    if (vertex_->subtype() != pisdf::VertexType::CONFIG) {
        throwSpiderException("Only config vertices can update parameter values.");
    }
    auto paramIterator = values.begin();
    for (const auto &param : vertex_->outputParamVector()) {
        param->setValue((*(paramIterator++)));
        if (log::enabled<log::TRANSFO>()) {
            log::info<log::TRANSFO>("Parameter [%12s]: received value #%" PRId64".\n",
                                    param->name().c_str(),
                                    param->value());
        }
    }
}

void spider::sched::SRDAGTask::insertSyncTasks(SyncTask *sndTask,
                                               SyncTask *rcvTask,
                                               size_t ix,
                                               const Schedule *schedule) {
    const auto fifo = buildInputFifo(vertex_->inputEdge(ix), schedule);
    sndTask->setAlloc(fifo);
    rcvTask->setAlloc(fifo);
}

i64 spider::sched::SRDAGTask::inputRate(size_t ix) const {
    return vertex_->inputEdge(ix)->rate();
}

i64 spider::sched::SRDAGTask::outputRate(size_t ix) const {
    return vertex_->outputEdge(ix)->rate();
}

spider::sched::Task *spider::sched::SRDAGTask::previousTask(size_t ix, const spider::sched::Schedule *schedule) const {
    const auto *source = vertex_->inputEdge(ix)->source();
    return schedule->task(source->scheduleTaskIx());
}

spider::sched::Task *spider::sched::SRDAGTask::nextTask(size_t ix, const spider::sched::Schedule *schedule) const {
    const auto *sink = vertex_->outputEdge(ix)->sink();
    return schedule->task(sink->scheduleTaskIx());
}

u32 spider::sched::SRDAGTask::color() const {
    const auto *reference = vertex_->reference();
    const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 3u) * 50 + 100);
    const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 2u) * 50 + 100);
    const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 4u) * 50 + 100);
    return 0u | (red << 16u) | (green << 8u) | (blue);
}

std::string spider::sched::SRDAGTask::name() const {
    return vertex_->name();
}

void spider::sched::SRDAGTask::setIx(u32 ix) noexcept {
    Task::setIx(ix);
    vertex_->setScheduleTaskIx(ix);
}

bool spider::sched::SRDAGTask::isMappableOnPE(const spider::PE *pe) const {
    return vertex_->runtimeInformation()->isPEMappable(pe);
}

u64 spider::sched::SRDAGTask::timingOnPE(const spider::PE *pe) const {
    return static_cast<u64>(vertex_->runtimeInformation()->timingOnPE(pe, vertex_->inputParamVector()));
}

size_t spider::sched::SRDAGTask::dependencyCount() const {
    return vertex_->inputEdgeCount();
}

size_t spider::sched::SRDAGTask::successorCount() const {
    return vertex_->outputEdgeCount();
}

/* === Private method(s) === */

u32 spider::sched::SRDAGTask::getOutputParamsCount() const {
    return static_cast<u32>(vertex_->reference()->outputParamCount());
}

u32 spider::sched::SRDAGTask::getKernelIx() const {
    return static_cast<u32>(vertex_->runtimeInformation()->kernelIx());
}

spider::unique_ptr<i64> spider::sched::SRDAGTask::buildInputParams() const {
    return srdag::buildVertexRuntimeInputParameters(vertex_);
}

std::shared_ptr<spider::JobFifos> spider::sched::SRDAGTask::buildJobFifos(const Schedule *schedule) const {
    auto fifos = spider::make_shared<JobFifos, StackID::RUNTIME>(vertex_->inputEdgeCount(), vertex_->outputEdgeCount());
    /* == Allocate input fifos == */
    for (const auto *edge : vertex_->inputEdges()) {
        fifos->setInputFifo(edge->sinkPortIx(), buildInputFifo(edge, schedule));
    }
    /* == Allocate output fifos == */
    switch (vertex_->subtype()) {
        case pisdf::VertexType::FORK:
            buildForkOutFifos(fifos->outputFifos().data(), fifos->inputFifo(0), schedule);
            break;
        case pisdf::VertexType::DUPLICATE:
            buildDupOutFifos(fifos->outputFifos().data(), fifos->inputFifo(0), schedule);
            break;
        case pisdf::VertexType::EXTERN_IN:
            buildExternINOutFifos(fifos->outputFifos().data(), schedule);
            break;
        default:
            buildDefaultOutFifos(fifos->outputFifos().data(), schedule);
            break;
    }
    return fifos;
}

spider::Fifo spider::sched::SRDAGTask::buildInputFifo(const srdag::Edge *edge, const Schedule *schedule) {
    Fifo fifo{ };
    fifo.virtualAddress_ = edge->allocatedAddress();
    fifo.size_ = static_cast<u32>(edge->rate());
    fifo.offset_ = 0;
    fifo.count_ = 0;
    const auto *source = edge->source();
    const auto *sourceTask = schedule->task(source->scheduleTaskIx());
    if (sourceTask && sourceTask->state() == TaskState::SKIPPED) {
        fifo.attribute_ = FifoAttribute::RW_AUTO;
    }
    if (source->subtype() == pisdf::VertexType::EXTERN_IN) {
        fifo.attribute_ = FifoAttribute::RW_EXT;
    } else if (source->subtype() == pisdf::VertexType::FORK) {
        for (size_t ix = 0; ix < edge->sourcePortIx(); ++ix) {
            fifo.offset_ += static_cast<u32>(source->outputEdge(ix)->rate());
        }
    }
    if ((fifo.attribute_ != FifoAttribute::RW_EXT) && (fifo.attribute_ != FifoAttribute::RW_AUTO)) {
        fifo.attribute_ = FifoAttribute::RW_OWN;
    }
    return fifo;
}

void spider::sched::SRDAGTask::buildDefaultOutFifos(Fifo *outputFifos, const Schedule *schedule) const {
    for (const auto *edge : vertex_->outputEdges()) {
        auto &fifo = outputFifos[edge->sourcePortIx()];
        fifo.virtualAddress_ = edge->allocatedAddress();
        fifo.size_ = static_cast<u32>(edge->rate());
        fifo.offset_ = 0;
        fifo.count_ = fifo.size_ ? 1 : 0;
        const auto *sink = edge->sink();
        const auto *sinkTask = schedule->task(sink->scheduleTaskIx());
        if (sinkTask && sinkTask->state() == TaskState::SKIPPED) {
            fifo.attribute_ = FifoAttribute::RW_AUTO;
        } else if (sink->subtype() == pisdf::VertexType::EXTERN_OUT) {
            const auto *reference = sink->reference()->convertTo<pisdf::ExternInterface>();
            fifo.virtualAddress_ = reference->bufferIndex();
            fifo.attribute_ = FifoAttribute::RW_EXT;
        } else {
            fifo.attribute_ = FifoAttribute::RW_OWN;
        }
    }
}

void spider::sched::SRDAGTask::buildExternINOutFifos(Fifo *outputFifos, const Schedule *) const {
    auto &fifo = outputFifos[0];
    fifo.virtualAddress_ = vertex_->reference()->convertTo<pisdf::ExternInterface>()->bufferIndex();
    fifo.size_ = static_cast<u32>(vertex_->outputEdge(0)->rate());
    fifo.offset_ = 0;
    fifo.count_ = fifo.size_ ? 1 : 0;
    fifo.attribute_ = FifoAttribute::RW_ONLY;
}

void spider::sched::SRDAGTask::buildForkOutFifos(Fifo *outputFifos, Fifo inputFifo, const Schedule *) const {
    u32 offset = 0;
    for (const auto *edge : vertex_->outputEdges()) {
        auto &fifo = outputFifos[edge->sourcePortIx()];
        fifo.virtualAddress_ = edge->allocatedAddress();
        fifo.size_ = static_cast<u32>(edge->rate());
        fifo.offset_ = inputFifo.offset_ + offset;
        fifo.count_ = fifo.size_ ? 1 : 0;
        fifo.attribute_ = FifoAttribute::RW_ONLY;
        offset += fifo.size_;
    }
}

void spider::sched::SRDAGTask::buildDupOutFifos(Fifo *outputFifos, Fifo inputFifo, const Schedule *) const {
    for (const auto *edge : vertex_->outputEdges()) {
        auto &fifo = outputFifos[edge->sourcePortIx()];
        fifo = inputFifo;
        fifo.count_ = fifo.size_ ? 1 : 0;
        fifo.attribute_ = FifoAttribute::RW_ONLY;
    }
}

#endif
