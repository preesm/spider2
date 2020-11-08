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

#include <scheduling/task/VectPiSDFTask.h>
#include <scheduling/launcher/TaskLauncher.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Graph.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>

#include <archi/Platform.h>
#include <api/archi-api.h>

/* === Static function === */

namespace {
    static auto LRT_COUNT = UINT32_MAX;
}

/* === Method(s) implementation === */

spider::sched::VectPiSDFTask::VectPiSDFTask(pisdf::GraphFiring *handler, const pisdf::Vertex *vertex) :
        PiSDFTask(handler, vertex) {
    if (LRT_COUNT == UINT32_MAX) {
        LRT_COUNT = static_cast<u32>(archi::platform()->LRTCount());
    }
    const auto rv = handler->getRV(vertex);
    syncInfoArray_ = spider::make_unique(make_n<u32, StackID::SCHEDULE>(LRT_COUNT * rv, UINT32_MAX));
    endTimeArray_ = spider::make_unique(make_n<u64, StackID::SCHEDULE>(rv, 0));
    mappedPEIxArray_ = spider::make_unique(make_n<u32, StackID::SCHEDULE>(rv, UINT32_MAX));
    jobExecIxArray_ = spider::make_unique(make_n<u32, StackID::SCHEDULE>(rv, UINT32_MAX));
    stateArray_ = spider::make_unique(make_n<TaskState, StackID::SCHEDULE>(rv, TaskState::NOT_SCHEDULABLE));
}

void spider::sched::VectPiSDFTask::reset() {
    const auto rv = handler()->getRV(vertex());
    std::fill(endTimeArray_.get(), endTimeArray_.get() + rv, u64{ 0 });
    std::fill(syncInfoArray_.get(), syncInfoArray_.get() + rv * LRT_COUNT, UINT32_MAX);
    std::fill(jobExecIxArray_.get(), jobExecIxArray_.get() + rv, UINT32_MAX);
    std::fill(mappedPEIxArray_.get(), mappedPEIxArray_.get() + rv, UINT32_MAX);
    std::fill(stateArray_.get(), stateArray_.get() + rv, TaskState::NOT_SCHEDULABLE);
}

u64 spider::sched::VectPiSDFTask::endTime() const {
    return endTimeArray_[currentFiring_];
}

const spider::PE *spider::sched::VectPiSDFTask::mappedPe() const {
    return archi::platform()->peFromVirtualIx(mappedPEIxArray_[currentFiring_]);
}

spider::sched::TaskState spider::sched::VectPiSDFTask::state() const noexcept {
    return stateArray_[currentFiring_];
}

u32 spider::sched::VectPiSDFTask::jobExecIx() const noexcept {
    return jobExecIxArray_[currentFiring_];
}

u32 spider::sched::VectPiSDFTask::syncExecIxOnLRT(size_t lrtIx) const {
    return syncInfoArray_[lrtIx + currentOffset_];
}

void spider::sched::VectPiSDFTask::setEndTime(u64 time) {
    endTimeArray_[currentFiring_] = time;
}

void spider::sched::VectPiSDFTask::setMappedPE(const PE *pe) {
    mappedPEIxArray_[currentFiring_] = static_cast<u32>(pe->virtualIx());
}

void spider::sched::VectPiSDFTask::setState(TaskState state) noexcept {
    stateArray_[currentFiring_] = state;
}

void spider::sched::VectPiSDFTask::setJobExecIx(u32 ix) noexcept {
    jobExecIxArray_[currentFiring_] = ix;
}

void spider::sched::VectPiSDFTask::setSyncExecIxOnLRT(size_t lrtIx, u32 value) {
    syncInfoArray_[lrtIx + currentOffset_] = value;
}

void spider::sched::VectPiSDFTask::setOnFiring(u32 firing) {
        PiSDFTask::setOnFiring(firing);
        currentOffset_ = currentFiring_ * LRT_COUNT;
}

