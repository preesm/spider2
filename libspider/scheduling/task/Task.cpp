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

#include <scheduling/task/Task.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <archi/PE.h>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::sched::Task::Task() : mappingInfo_{
        spider::make_unique<detail::MappingInfo, StackID::SCHEDULE>(detail::MappingInfo{ }) } {
    const auto lrtCount{ archi::platform()->LRTCount() };
    execInfo_.constraints_ = make_unique<size_t>(allocate<size_t, StackID::SCHEDULE>(lrtCount));
    execInfo_.notifications_ = make_unique<bool>(allocate<bool, StackID::SCHEDULE>(lrtCount));
}

u64 spider::sched::Task::startTime() const {
    return mappingInfo_->startTime_;
}

u64 spider::sched::Task::endTime() const {
    return mappingInfo_->endTime_;
}

const spider::PE *spider::sched::Task::mappedPe() const {
    return mappingInfo_->mappedPE_;
}

const spider::PE *spider::sched::Task::mappedLRT() const {
    return mappingInfo_->mappedPE_->attachedLRT();
}

void spider::sched::Task::setStartTime(u64 time) {
    mappingInfo_->startTime_ = time;
}

void spider::sched::Task::setEndTime(u64 time) {
    mappingInfo_->endTime_ = time;
}

void spider::sched::Task::setMappedPE(const spider::PE *const pe) {
    mappingInfo_->mappedPE_ = pe;
}
