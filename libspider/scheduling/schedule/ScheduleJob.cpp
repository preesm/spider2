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

#include <scheduling/schedule/ScheduleJob.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <graphs/pisdf/Graph.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/allocator/FifoAllocator.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::sched::Job::Job() :
        scheduleConstraintsArray_{ archi::platform()->LRTCount(), SIZE_MAX, StackID::SCHEDULE },
        runnerToNotifyArray_{ archi::platform()->LRTCount(), true, StackID::SCHEDULE } { }

spider::sched::Job::Job(pisdf::Vertex *vertex) : Job() {
    vertex_ = vertex;
}

void spider::sched::Job::setVertex(const spider::pisdf::Vertex *vertex) {
    if (vertex) {
        vertex_ = vertex;
        outputFifoVector_.clear();
        outputFifoVector_.reserve(vertex->outputEdgeCount());
    }
}

spider::JobMessage spider::sched::Job::createJobMessage(const Schedule *schedule) {
    if (!vertex_) {
        throwSpiderException("no vertex has been set on this Job.");
    }
    /* == Update the JobMessage == */
    auto message = JobMessage();
    message.LRTs2Notify_ = runnerToNotifyArray_;

    /* == Set essential properties == */
    message.ix_ = ix_;
    message.kernelIx_ = vertex_->runtimeInformation()->kernelIx();
    message.outputParamCount_ = vertex_->reference()->outputParamCount();
    message.vertexIx_ = vertex_->ix();

    /* == Create the jobs 2 wait array == */
    message.jobs2Wait_ = spider::array<JobConstraint>(this->numberOfConstraints(), StackID::RUNTIME);
    auto jobIterator = message.jobs2Wait_.begin();
    size_t lrtIt = 0;
    for (auto &srcJobIx : scheduleConstraintsArray_) {
        if (srcJobIx != SIZE_MAX) {
            (*jobIterator).lrtToWait_ = lrtIt;
            (*jobIterator).jobToWait_ = srcJobIx;
            jobIterator++;
        }
        lrtIt++;
    }

    /* == Creates FIFOs == */
    message.inputFifoArray_ = spider::array<RTFifo>(vertex_->inputEdgeCount(), StackID::RUNTIME);
    message.outputFifoArray_ = spider::array<RTFifo>(vertex_->outputEdgeCount(), StackID::RUNTIME);
//    log::print<log::Type::GENERAL>(log::white, "INFO: ", "Vertex: %s\n", vertex_->name().c_str());
    for (size_t i = 0; i < vertex_->inputEdgeCount(); ++i) {
        const auto &edge = vertex_->inputEdge(i);
        const auto &source = edge->source();
        auto &fifo = message.inputFifoArray_[i];
        auto &srcJob = schedule->job(source->scheduleJobIx());
        fifo = srcJob.outputFIFO(edge->sourcePortIx());
//        log::print<log::Type::GENERAL>(log::green, "INFO: ", "   <- edge [%zu]: %zu\n", i, fifo.virtualAddress_);
    }


    for (size_t outputIx = 0; outputIx < vertex_->outputEdgeCount(); ++outputIx) {
        auto &fifo = message.outputFifoArray_[outputIx];
        fifo = outputFIFO(outputIx);
//        log::print<log::Type::GENERAL>(log::red, "INFO: ", "   -> edge [%zu]: %zu\n", outputIx, fifo.virtualAddress_);
    }

    /* == Set the input parameters == */
    switch (vertex_->reference()->subtype()) {
        case pisdf::VertexType::CONFIG:
        case pisdf::VertexType::NORMAL: {
            message.inputParams_ = spider::array<int64_t>(vertex_->reference()->refinementParamVector().size(),
                                                          StackID::RUNTIME);
            auto paramIterator = message.inputParams_.begin();
            for (auto &param : vertex_->refinementParamVector()) {
                (*(paramIterator++)) = param->value();
            }
        }
            break;
        case pisdf::VertexType::FORK:
            message.inputParams_ = spider::array<int64_t>(vertex_->outputEdgeCount() + 2, StackID::RUNTIME);
            message.inputParams_[0] = vertex_->inputEdge(0)->sinkRateValue();
            message.inputParams_[1] = static_cast<int64_t>(vertex_->outputEdgeCount());
            for (size_t i = 0; i < vertex_->outputEdgeCount(); ++i) {
                message.inputParams_[i + 2] = vertex_->outputEdge(i)->sourceRateValue();
            }
            break;
        case pisdf::VertexType::JOIN:
            message.inputParams_ = spider::array<int64_t>(vertex_->inputEdgeCount() + 2, StackID::RUNTIME);
            message.inputParams_[0] = vertex_->outputEdge(0)->sourceRateValue();
            message.inputParams_[1] = static_cast<int64_t>(vertex_->inputEdgeCount());
            for (size_t i = 0; i < vertex_->inputEdgeCount(); ++i) {
                message.inputParams_[i + 2] = vertex_->inputEdge(i)->sinkRateValue();
            }
            break;
        case pisdf::VertexType::REPEAT:
            message.inputParams_ = spider::array<int64_t>(2, StackID::RUNTIME);
            message.inputParams_[0] = vertex_->inputEdge(0)->sinkRateValue();
            message.inputParams_[1] = vertex_->outputEdge(0)->sourceRateValue();
            break;
        case pisdf::VertexType::DUPLICATE:
            message.inputParams_ = spider::array<int64_t>(2, StackID::RUNTIME);
            message.inputParams_[0] = static_cast<int64_t>(vertex_->outputEdgeCount());
            message.inputParams_[1] = vertex_->inputEdge(0)->sinkRateValue();
            break;
        case pisdf::VertexType::TAIL:
            break;
        case pisdf::VertexType::HEAD:
            break;
        case pisdf::VertexType::INIT:
            break;
        case pisdf::VertexType::END:
            break;
        case pisdf::VertexType::INPUT:
            break;
        case pisdf::VertexType::OUTPUT:
            break;
        default:
            throwSpiderException("unhandled type of vertex.");
    }
    return message;
}