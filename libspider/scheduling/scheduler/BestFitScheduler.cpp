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

#include <scheduling/scheduler/BestFitScheduler.h>
#include <runtime/interface/Message.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/runner/RTRunner.h>
#include <api/runtime-api.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::sched::Schedule &spider::BestFitScheduler::mappingScheduling() {
    /* == Save the old number of job count == */
    auto oldJobCountVector = containers::vector<size_t>(archi::platform()->LRTCount(), StackID::SCHEDULE);
    for (auto &lrt : archi::platform()->lrtVector()) {
        const auto &jobCount = schedule_.stats().jobCount(lrt->virtualIx());
        oldJobCountVector[lrt->virtualIx()] = jobCount;
    }

    /* == Schedule and map the vertex onto available ressource == */
    auto iterator = sortedVertexVector_.begin() + static_cast<long>(lastScheduledVertex_);
    auto endIterator = sortedVertexVector_.begin() + static_cast<long>(lastSchedulableVertex_);
    while (iterator != endIterator) {
        auto &listVertex = (*(iterator++));
        Scheduler::vertexMapper(listVertex.vertex_);

//        /* == Send the job message == */
//        auto &job = schedule_.job(listVertex.vertex_->scheduleJobIx());
//        const auto &messageIx = rt::platform()->communicator()->push(job.message(), job.mappingInfo().LRTIx);
//
//        rt::platform()->communicator()->push(Notification(NotificationType::JOB_ADD,
//                                                          0,
//                                                          messageIx),
//                                             job.mappingInfo().LRTIx);
//        job.setState(sched::JobState::RUNNING);
    }
    lastScheduledVertex_ = lastSchedulableVertex_;
    schedule_.print();

    /* == Send the job count to each runner == */
    const auto &grtIx = archi::platform()->spiderGRTPE()->virtualIx();
    for (auto &lrt : archi::platform()->lrtVector()) {
        const auto &jobCount = schedule_.stats().jobCount(lrt->virtualIx());
        const auto &newJobCount = jobCount - oldJobCountVector[lrt->virtualIx()];
        if (newJobCount) {
            rt::platform()->communicator()->push(Notification(NotificationType::JOB_JOB_COUNT,
                                                              grtIx,
                                                              newJobCount),
                                                 lrt->virtualIx());
        }
    }

//    /* == Run the jobs of GRT (if any) == */
//    rt::platform()->runner(grtIx)->run(false);
    return schedule_;
}