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

#include <scheduling/schedule/Schedule.h>
#include <graphs/pisdf/Vertex.h>
#include <runtime/interface/RTCommunicator.h>
#include <runtime/platform/RTPlatform.h>
#include <api/archi-api.h>
#include <api/runtime-api.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

void spider::sched::Schedule::clear() {
    jobVector_.clear();
    stats_.reset();
    lastRunJob_ = 0;
    readyJobCount_ = 0;
}

void spider::sched::Schedule::reset() {
    for (auto &job : jobVector_) {
        job.setState(JobState::READY);
    }
    lastRunJob_ = 0;
    readyJobCount_ = static_cast<long>(jobVector_.size());
}

void spider::sched::Schedule::print() const {
    if (log::enabled<log::Type::SCHEDULE>()) {
        for (const auto &job : jobs()) {
            log::print<log::Type::SCHEDULE>(log::magenta, "INFO: ", "Schedule: \n");
            log::print<log::Type::SCHEDULE>(log::magenta, "INFO: ", "   >> job: %zu (runner: %zu) [%s]\n", job.ix(),
                                            job.mappingInfo().LRTIx, job.vertex()->reference()->name().c_str());
            size_t lrtIx = 0;
            for (const auto &index : job.scheduleConstraintsArray()) {
                if (index != SIZE_MAX) {
                    log::print<log::Type::SCHEDULE>(log::magenta, "INFO: ",
                                                    "           ----> job: %zu (runner: %zu) [%s]\n",
                                                    jobVector_[index].ix(), lrtIx,
                                                    jobVector_[index].vertex()->reference()->name().c_str());
                }
                lrtIx++;
            }
        }
    }
}

void spider::sched::Schedule::addJobToSchedule(pisdf::Vertex *vertex) {
    auto job = Job(vertex);
    /* == Set the schedule job ix of the vertex == */
    job.setIx(jobVector_.size());
    vertex->setScheduleJobIx(job.ix());

    /* == Add the job to the schedule == */
    jobVector_.emplace_back(std::move(job));
}

void spider::sched::Schedule::updateJobAndSetReady(size_t jobIx, size_t slave, uint64_t startTime, uint64_t endTime) {
    auto &job = jobVector_.at(jobIx);
    const auto &pe = archi::platform()->peFromVirtualIx(slave);
    const auto &peIx = pe->virtualIx();

    /* == Set job information == */
    job.setMappingLRT(pe->attachedLRT()->virtualIx());
    job.setMappingPE(peIx);
    job.setMappingStartTime(startTime);
    job.setMappingEndTime(endTime);

    /* == Set should notify value for previous jobs == */
    for (auto &constraint : job.scheduleConstraintsArray()) {
        if (constraint != SIZE_MAX) {
            jobVector_[constraint].setRunnerToNotify(job.mappingInfo().LRTIx, true);
        }
    }

    /* == Update schedule statistics == */
    stats_.updateStartTime(peIx, startTime);
    stats_.updateIDLETime(peIx, startTime - stats_.endTime(peIx));
    stats_.updateEndTime(peIx, endTime);
    stats_.updateLoadTime(peIx, endTime - startTime);
    stats_.updateJobCount(peIx);

    /* == Update job state == */
    job.setState(JobState::READY);
    readyJobCount_++;
}

void spider::sched::Schedule::sendReadyJobs() {
    const auto &grtIx = archi::platform()->spiderGRTPE()->virtualIx();
    auto startIterator = std::begin(jobVector_) + lastRunJob_;
    auto endIterator = startIterator + readyJobCount_;
    for (auto it = startIterator; it < endIterator; ++it) {
        auto &job = (*it);
        /* == Create job message and send the notification == */
        const auto &messageIx = rt::platform()->communicator()->push(job.createJobMessage(this),
                                                                     job.mappingInfo().LRTIx);

        rt::platform()->communicator()->push(Notification(NotificationType::JOB_ADD,
                                                          grtIx,
                                                          messageIx),
                                             job.mappingInfo().LRTIx);

        /* == Set job in JobState::RUNNING == */
        job.setState(sched::JobState::RUNNING);
    }
    /* == Reset last job and ready count == */
    lastRunJob_ += readyJobCount_;
    readyJobCount_ = 0;
}
