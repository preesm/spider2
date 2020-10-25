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
#include <scheduling/task/SyncTask.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/memory/FifoAllocator.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <runtime/common/Fifo.h>
#include <api/runtime-api.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::PiSDFTask::PiSDFTask(pisdf::GraphFiring *handler, pisdf::Vertex *vertex, u32 firing) :
        Task(), handler_{ handler }, vertex_{ vertex }, firing_{ firing } {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    inputs_ = spider::make_unique(spider::make_n<Task *, StackID::SCHEDULE>(vertex->inputEdgeCount()));
    outputs_ = spider::make_unique(spider::make_n<Task *, StackID::SCHEDULE>(vertex->outputEdgeCount()));
}

void spider::sched::PiSDFTask::receiveParams(const spider::array<i64> &values) {
    if (vertex_->subtype() != pisdf::VertexType::CONFIG) {
        throwSpiderException("Only config vertices can update parameter values.");
    }
    auto paramIterator = values.begin();
    for (const auto ix : vertex_->outputParamIxVector()) {
        const auto value = *(paramIterator++);
        handler_->setParamValue(ix, value);
        if (log::enabled<log::TRANSFO>()) {
            log::info<log::TRANSFO>("Parameter [%12s]: received value #%" PRId64".\n",
                                    handler_->getParams()[ix]->name().c_str(), value);
        }
    }
}

void spider::sched::PiSDFTask::insertSyncTasks(SyncTask *sndTask,
                                               SyncTask *rcvTask,
                                               size_t ix,
                                               const Schedule *schedule) {
    const auto fifo = buildInputFifo(vertex_->inputEdge(ix), schedule);
    sndTask->setAlloc(fifo);
    rcvTask->setAlloc(fifo);
}

i64 spider::sched::PiSDFTask::inputRate(size_t ix) const {
    return handler_->getSinkRate(vertex_->inputEdge(ix));
}

i64 spider::sched::PiSDFTask::outputRate(size_t ix) const {
    return handler_->getSourceRate(vertex_->outputEdge(ix));
}

spider::sched::Task *spider::sched::PiSDFTask::previousTask(size_t ix, const spider::sched::Schedule *) const {
    return inputs_.get()[ix];
}

spider::sched::Task *spider::sched::PiSDFTask::nextTask(size_t ix, const spider::sched::Schedule *) const {
    return outputs_.get()[ix];
}

u32 spider::sched::PiSDFTask::color() const {
    const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 3u) * 50 + 100);
    const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 2u) * 50 + 100);
    const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex_) >> 4u) * 50 + 100);
    return 0u | (red << 16u) | (green << 8u) | (blue);
}

std::string spider::sched::PiSDFTask::name() const {
    return vertex_->name();
}

void spider::sched::PiSDFTask::setIx(u32 ix) noexcept {
    Task::setIx(ix);
    handler_->registerTaskIx(vertex_, firing_, ix);
}

bool spider::sched::PiSDFTask::isMappableOnPE(const spider::PE *pe) const {
    return vertex_->runtimeInformation()->isPEMappable(pe);
}

u64 spider::sched::PiSDFTask::timingOnPE(const spider::PE *pe) const {
    return static_cast<u64>(vertex_->runtimeInformation()->timingOnPE(pe, handler_->getParams()));
}

size_t spider::sched::PiSDFTask::dependencyCount() const {
    return vertex_->inputEdgeCount();
}

/* === Private method(s) === */

u32 spider::sched::PiSDFTask::getOutputParamsCount() const {
    return static_cast<u32>(vertex_->outputParamCount());
}

u32 spider::sched::PiSDFTask::getKernelIx() const {
    return static_cast<u32>(vertex_->runtimeInformation()->kernelIx());
}

spider::unique_ptr<i64> spider::sched::PiSDFTask::buildInputParams() const {
    return pisdf::buildVertexRuntimeInputParameters(vertex_, handler_->getParams());
}

bool spider::sched::PiSDFTask::updateNotificationFlags(bool *flags, const Schedule *schedule) const {
    auto oneTrue = false;
    for (const auto *edge : vertex_->outputEdges()) {
        auto *sink = edge->sink();
        auto *sinkTask = outputs_.get()[edge->sourcePortIx()];
        if (sinkTask->state() == TaskState::SKIPPED) {
            sinkTask->updateNotificationFlags(flags, schedule);
        }
        auto &currentFlag = flags[sinkTask->mappedLRT()->virtualIx()];
        if (!currentFlag) {
            currentFlag = true;
            for (size_t ix = 0; ix < sinkTask->dependencyCount(); ++ix) {
                auto *sourceTask = sinkTask->previousTask(ix, schedule);
                if (sourceTask && (sourceTask->mappedLRT() == mappedLRT()) && (sourceTask->jobExecIx() > jobExecIx())) {
                    currentFlag = false;
                    break;
                }
            }
        }
        oneTrue |= currentFlag;
    }
    return oneTrue;
}

bool spider::sched::PiSDFTask::shouldBroadCast(const Schedule *schedule) const {
    const auto &outputEdges = vertex_->outputEdges();
    const auto *outputs = outputs_.get();
    return std::any_of(std::begin(outputEdges), std::end(outputEdges),
                       [outputs](const pisdf::Edge *edge) {
                           const auto *sinkTask = outputs[edge->sourcePortIx()];
                           return !sinkTask || (sinkTask->state() != TaskState::READY &&
                                                sinkTask->state() != TaskState::SKIPPED);
                       });
}

std::shared_ptr<spider::JobFifos> spider::sched::PiSDFTask::buildJobFifos(const Schedule *schedule) const {
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

spider::Fifo spider::sched::PiSDFTask::buildInputFifo(const pisdf::Edge *edge, const Schedule *schedule) {
    Fifo fifo{ };
//    fifo.virtualAddress_ = edge->allocatedAddress();
//    fifo.size_ = static_cast<u32>(edge->rate());
//    fifo.offset_ = 0;
//    fifo.count_ = 0;
//    const auto *source = edge->source();
//    const auto *sourceTask = schedule->task(source->scheduleTaskIx());
//    if (sourceTask && sourceTask->state() == TaskState::SKIPPED) {
//        fifo.attribute_ = FifoAttribute::RW_AUTO;
//    }
//    if (source->subtype() == pisdf::VertexType::EXTERN_IN) {
//        fifo.attribute_ = FifoAttribute::RW_EXT;
//    } else if (source->subtype() == pisdf::VertexType::FORK) {
//        for (size_t ix = 0; ix < edge->sourcePortIx(); ++ix) {
//            fifo.offset_ += static_cast<u32>(source->outputEdge(ix)->rate());
//        }
//    }
//    if ((fifo.attribute_ != FifoAttribute::RW_EXT) && (fifo.attribute_ != FifoAttribute::RW_AUTO)) {
//        fifo.attribute_ = FifoAttribute::RW_OWN;
//    }
    return fifo;
}

void spider::sched::PiSDFTask::buildDefaultOutFifos(Fifo *outputFifos, const Schedule *schedule) const {
    for (const auto *edge : vertex_->outputEdges()) {
        auto &fifo = outputFifos[edge->sourcePortIx()];
        fifo.virtualAddress_ = handler_->getEdgeAlloc(edge);
        fifo.virtualAddress_ += static_cast<size_t>(firing_ * handler_->getSourceRate(edge));
        fifo.size_ = static_cast<u32>(handler_->getSourceRate(edge));
        fifo.offset_ = 0;
        fifo.count_ = fifo.size_ ? 1 : 0;
//        const auto *sink = edge->sink();
//        const auto *sinkTask = schedule->task(sink->scheduleTaskIx());
//        if (sinkTask && sinkTask->state() == TaskState::SKIPPED) {
//            fifo.attribute_ = FifoAttribute::RW_AUTO;
//        } else if (sink->subtype() == pisdf::VertexType::EXTERN_OUT) {
//            const auto *reference = sink->convertTo<pisdf::ExternInterface>();
//            fifo.virtualAddress_ = reference->bufferIndex();
//            fifo.attribute_ = FifoAttribute::RW_EXT;
//        } else {
//            fifo.attribute_ = FifoAttribute::RW_OWN;
//        }
    }
}

void spider::sched::PiSDFTask::buildExternINOutFifos(Fifo *outputFifos, const Schedule *) const {
    auto &fifo = outputFifos[0];
    fifo.virtualAddress_ = vertex_->convertTo<pisdf::ExternInterface>()->bufferIndex();
    fifo.size_ = static_cast<u32>(handler_->getSourceRate(vertex_->outputEdge(0)));
    fifo.offset_ = 0;
    fifo.count_ = fifo.size_ ? 1 : 0;
    fifo.attribute_ = FifoAttribute::RW_ONLY;
}

void spider::sched::PiSDFTask::buildForkOutFifos(Fifo *outputFifos, Fifo inputFifo, const Schedule *) const {
    u32 offset = 0;
    for (const auto *edge : vertex_->outputEdges()) {
        auto &fifo = outputFifos[edge->sourcePortIx()];
        fifo.virtualAddress_ = handler_->getEdgeAlloc(edge);
        fifo.virtualAddress_ += static_cast<size_t>(firing_ * handler_->getSourceRate(edge));
        fifo.size_ = static_cast<u32>(handler_->getSourceRate(edge));
        fifo.offset_ = inputFifo.offset_ + offset;
        fifo.count_ = fifo.size_ ? 1 : 0;
        fifo.attribute_ = FifoAttribute::RW_ONLY;
        offset += fifo.size_;
    }
}

void spider::sched::PiSDFTask::buildDupOutFifos(Fifo *outputFifos, Fifo inputFifo, const Schedule *) const {
    for (const auto *edge : vertex_->outputEdges()) {
        auto &fifo = outputFifos[edge->sourcePortIx()];
        fifo = inputFifo;
        fifo.count_ = fifo.size_ ? 1 : 0;
        fifo.attribute_ = FifoAttribute::RW_ONLY;
    }
}
