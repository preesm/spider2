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
    memoryAddesses_.resize(graph_->edgeCount(), UINT64_MAX);
    auto iterator = sortedVertexVector_.begin() + static_cast<long>(lastScheduledVertex_);
    while (iterator != std::end(sortedVertexVector_)) {
        /* == Schedule and map the vertex onto available ressource == */
        Scheduler::vertexMapper((*iterator).vertex_);
        iterator++;

        /* == Do the memory allocation and build the job message == */
//        auto message = buildJobMessage(listVertex.vertex_);
//
//        /* == Send the job message == */
//        auto &job = schedule_.job(listVertex.vertex_->ix());
//        const auto &messageIx = rt::platform()->communicator()->push(message, job.mappingInfo().LRTIx);
//
//        rt::platform()->communicator()->push(Notification(NotificationType::JOB,
//                                                          JobNotification::ADD,
//                                                          0,
//                                                          messageIx),
//                                             job.mappingInfo().LRTIx);
//        job.setState(sched::JobState::RUNNING);
    }
    lastScheduledVertex_ = sortedVertexVector_.size();

//    for (auto &lrt : archi::platform()->lrtVector()) {
//        const auto &jobCount = schedule_.stats().jobCount(lrt->virtualIx());
//        rt::platform()->communicator()->push(Notification(NotificationType::JOB,
//                                                          JobNotification::JOB_COUNT,
//                                                          0,
//                                                          jobCount),
//                                             lrt->virtualIx());
//    }
//
//    /* == Run the jobs of GRT (if any) == */
//    rt::platform()->runner(0)->run(false);
    return schedule_;
}

/* === Private method(s) === */

spider::JobMessage spider::BestFitScheduler::buildJobMessage(const pisdf::Vertex *vertex) {
    const auto &job = schedule_.job(vertex->ix());
    JobMessage message;
    message.LRTs2Notify_ = spider::array<bool>(archi::platform()->LRTCount(), true);

    /* == Set the kernel ix == */
    message.kernelIx_ = vertex->runtimeInformation()->kernelIx();

    /* == Set the job ix == */
    message.ix_ = job.ix();

    /* == Create input Fifos == */
    message.inputFifoArray_ = spider::array<RTFifo>(vertex->inputEdgeCount(), StackID::RUNTIME);
    size_t i = 0;
    for (const auto &edge : vertex->inputEdgeVector()) {
        auto &fifo = message.inputFifoArray_[i++];
        fifo.size_ = static_cast<size_t>(edge->sourceRateExpression().evaluate(params_));
        fifo.virtualAddress_ = memoryAddesses_[edge->ix()];
        fifo.memoryInterface_ = archi::platform()->peFromVirtualIx(
                job.mappingInfo().PEIx)->cluster()->memoryInterface();
    }

    /* == Create output Fifos == */
    message.outputFifoArray_ = spider::array<RTFifo>(vertex->outputEdgeCount(), StackID::RUNTIME);
    for (const auto &edge : vertex->outputEdgeVector()) {
        memoryAddesses_[edge->ix()] = virtualMemoryAddress_++;
        auto &fifo = message.outputFifoArray_[i++];
        fifo.size_ = static_cast<size_t>(edge->sourceRateExpression().evaluate(params_));
        fifo.virtualAddress_ = memoryAddesses_[edge->ix()];;
        fifo.memoryInterface_ = archi::platform()->peFromVirtualIx(
                job.mappingInfo().PEIx)->cluster()->memoryInterface();

        /* == Register the LRTs to notify on job completion == */
//        const auto &sink = edge->sink();
//        const auto &snkJob = schedule_.job(sink->ix());
//        message.LRTs2Notify_[snkJob.mappingInfo().LRTIx] = (snkJob.mappingInfo().LRTIx != job.mappingInfo().LRTIx);
    }

    /* == Create the jobs 2 wait array == */
    const auto &numberOfConstraints = job.numberOfConstraints();
    message.jobs2Wait_ = spider::array<std::pair<size_t, size_t>>(numberOfConstraints, StackID::RUNTIME);
    auto jobIterator = message.jobs2Wait_.begin();
    for (auto &srcJob : job.jobConstraintVector()) {
        if (srcJob) {
            (*(jobIterator++)) = std::make_pair(srcJob->mappingInfo().LRTIx, srcJob->ix());
        }
    }

    /* == Set the number of output parameters to be set == */
    message.outputParamCount_ = rt::platform()->runtimeKernels()[message.kernelIx_]->outputParamsValue().size();

    /* == Creates the input parameters array == */
    const auto &inputParamIndexArray = rt::platform()->runtimeKernels()[message.kernelIx_]->inputParamsValue();
    message.inputParams_ = spider::array<int64_t>(inputParamIndexArray.size(), StackID::RUNTIME);
    auto paramIterator = message.inputParams_.begin();
    for (auto &index : inputParamIndexArray) {
        // TODO: fix it with proper values
        (*(paramIterator++)) = vertex->reference()->graph()->param(index)->value();
    }
    return message;
}