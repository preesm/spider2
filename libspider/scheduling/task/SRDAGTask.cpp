/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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
#include <scheduling/launcher/TaskLauncher.h>
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
    syncInfoArray_ = make_unique(make_n<SyncInfo, StackID::SCHEDULE>(archi::platform()->LRTCount(), { UINT32_MAX, 0 }));
}

void spider::sched::SRDAGTask::visit(TaskLauncher *launcher) {
    launcher->visit(this);
}

bool spider::sched::SRDAGTask::receiveParams(const spider::array<i64> &values) {
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
    return false;
}

i64 spider::sched::SRDAGTask::inputRate(size_t ix) const {
    return vertex_->inputEdge(ix)->rate();
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
    return vertex_->vertexPath();
}

u32 spider::sched::SRDAGTask::ix() const noexcept {
    return static_cast<u32>(vertex_->scheduleTaskIx());
}

u64 spider::sched::SRDAGTask::startTime() const {
    return endTime_ - timingOnPE(mappedPe());
}

void spider::sched::SRDAGTask::setIx(u32 ix) noexcept {
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

const spider::PE *spider::sched::SRDAGTask::mappedPe() const {
    return archi::platform()->peFromVirtualIx(mappedPEIx_);
}

const spider::PE *spider::sched::SRDAGTask::mappedLRT() const {
    return mappedPe()->attachedLRT();
}

void spider::sched::SRDAGTask::setMappedPE(const spider::PE *pe) {
    mappedPEIx_ = static_cast<u32>(pe->virtualIx());
    syncInfoArray_[pe->attachedLRT()->virtualIx()].jobExecIx = UINT32_MAX;
    syncInfoArray_[pe->attachedLRT()->virtualIx()].rate = 0;
}

#endif
