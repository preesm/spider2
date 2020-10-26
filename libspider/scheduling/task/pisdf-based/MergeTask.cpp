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

#include <scheduling/task/pisdf-based/MergeTask.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <scheduling/schedule/Schedule.h>
#include <archi/MemoryBus.h>
#include <archi/Platform.h>
#include <api/archi-api.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::MergeTask::MergeTask(const pisdf::DependencyIterator &dependencies,
                                    i32 depCount,
                                    Task *successor,
                                    const Schedule *schedule) :
        Task(), successorIx_{ static_cast<u32>(successor->ix()) } {
//    dependencies_ = factory::vector<pisdf::DependencyInfo>(static_cast<size_t>(depCount), StackID::SCHEDULE);
//    inputs_ = spider::make_unique(spider::make_n<Task *, StackID::SCHEDULE>(dependencies_.size()));
//    size_t i = 0;
//    for (const auto &dep : dependencies) {
//        for (auto k = dep.firingStart_; k <= dep.firingEnd_; ++k) {
//            dependencies_[i] = dep;
//            dependencies_[i].memoryStart_ = k == dep.firingStart_ ? dep.memoryStart_ : 0;
//            dependencies_[i].memoryEnd_ = k == dep.firingEnd_ ? dep.memoryEnd_ : static_cast<u32>(dep.rate_) - 1;
//            dependencies_[i].firingStart_ = k;
//            dependencies_[i].firingEnd_ = k;
//            inputs_.get()[i] = schedule->task(dep.handler_->getTaskIx(dep.vertex_, k));
//            i++;
//        }
//    }
}

spider::sched::Task *spider::sched::MergeTask::previousTask(size_t ix, const Schedule *schedule) const {
    return schedule->task(inputs_.get()[ix]);
}

spider::sched::Task *spider::sched::MergeTask::nextTask(size_t, const spider::sched::Schedule *schedule) const {
    return schedule->task(successorIx_);
}

u64 spider::sched::MergeTask::timingOnPE(const spider::PE *) const {
    u64 time = 0;
    for (size_t ix = 0; ix < dependencyCount(); ++ix) {
        time += static_cast<ufast64>(inputRate(ix));
    }
    return time;
}

/* === Private method(s) implementation === */

spider::unique_ptr<i64> spider::sched::MergeTask::buildInputParams() const {
    auto params = spider::allocate<i64, StackID::RUNTIME>(depInCount_ + 2);
#ifndef NDEBUG
    if (!params) {
        throwNullptrException();
    }
#endif
    params[0] = static_cast<i64>(outputRate(0));
    params[1] = static_cast<i64>(dependencyCount());
    for (u32 i = 0; i < depInCount_; ++i) {
        params[2 + i] = static_cast<i64>(inputRate(i));
    }
    return make_unique(params);
}

