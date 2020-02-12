/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
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

#include <scheduling/schedule/ScheduleTask.h>
#include <archi/Platform.h>
#include <archi/PE.h>
#include <api/archi-api.h>
#include <iostream>
#include <fstream>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::ScheduleTask::ScheduleTask(TaskType type) : type_{ type } {
    const auto lrtCount = archi::platform()->LRTCount();
    executionConstraints_ = make_unique<int32_t>(allocate<int32_t, StackID::SCHEDULE>(lrtCount));
    std::fill(executionConstraints_.get(), executionConstraints_.get() + lrtCount, -1);
    notificationFlags_ = make_unique<bool>(allocate<bool, StackID::SCHEDULE>(lrtCount));
    std::fill(notificationFlags_.get(), notificationFlags_.get() + lrtCount, false);
}

void spider::ScheduleTask::enableBroadcast() {
    const auto lrtCount = archi::platform()->LRTCount();
    std::fill(notificationFlags_.get(), notificationFlags_.get() + lrtCount, true);
}

void spider::ScheduleTask::exportXML(std::ofstream &file) const {
    const auto *platform = archi::platform();
    auto PEIx = platform->peFromVirtualIx(mappedPE())->hardwareIx();
    file << '\t' << "<event" << '\n';
    file << '\t' << '\t' << R"(start=")" << startTime() << R"(")" << '\n';
    file << '\t' << '\t' << R"(end=")" << endTime_ << R"(")" << '\n';
    file << '\t' << '\t' << R"(mapping="PE)" << PEIx << R"(")" << '\n';
    file << '\t' << '\t' << ">.</event>" << '\n';
}

void spider::ScheduleTask::setNumberOfDependencies(size_t count) {
    if (dependenciesArray_.empty()) {
        dependenciesArray_ = array<ScheduleTask *>(count, nullptr, StackID::SCHEDULE);
    }
}

void spider::ScheduleTask::setDependency(ScheduleTask *task, size_t pos) {
    if (!task || dependenciesArray_.empty()) {
        return;
    }
    dependenciesArray_.at(pos) = task;
}
