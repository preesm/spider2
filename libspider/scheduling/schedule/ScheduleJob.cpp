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
    message.vertexIx_ = vertex_->ix();
    message.kernelIx_ = vertex_->runtimeInformation()->kernelIx();
    message.outputParamCount_ = vertex_->outputParamCount();

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
    const auto &inputEdgeCount = vertex_->inputEdgeCount();
    const auto &outputEdgeCount = vertex_->outputEdgeCount();
    message.inputFifoArray_ = spider::array<RTFifo>(inputEdgeCount, StackID::RUNTIME);
    message.outputFifoArray_ = spider::array<RTFifo>(outputEdgeCount, StackID::RUNTIME);
    for (size_t i = 0; i < inputEdgeCount; ++i) {
        const auto &edge = vertex_->inputEdge(i);
        const auto &source = edge->source();
        auto &fifo = message.inputFifoArray_[i];
        auto &srcJob = schedule->job(source->scheduleJobIx());
        fifo = srcJob.outputFIFO(edge->sourcePortIx());
    }

    for (size_t outputIx = 0; outputIx < outputEdgeCount; ++outputIx) {
        auto &fifo = message.outputFifoArray_[outputIx];
        fifo = outputFIFO(outputIx);
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
            message.inputParams_ = spider::array<int64_t>(outputEdgeCount + 2, StackID::RUNTIME);
            message.inputParams_[0] = vertex_->inputEdge(0)->sinkRateValue();
            message.inputParams_[1] = static_cast<int64_t>(outputEdgeCount);
            for (size_t i = 0; i < outputEdgeCount; ++i) {
                message.inputParams_[i + 2] = vertex_->outputEdge(i)->sourceRateValue();
            }
            break;
        case pisdf::VertexType::JOIN:
            message.inputParams_ = spider::array<int64_t>(inputEdgeCount + 2, StackID::RUNTIME);
            message.inputParams_[0] = vertex_->outputEdge(0)->sourceRateValue();
            message.inputParams_[1] = static_cast<int64_t>(inputEdgeCount);
            for (size_t i = 0; i < inputEdgeCount; ++i) {
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
            message.inputParams_[0] = static_cast<int64_t>(outputEdgeCount);
            message.inputParams_[1] = vertex_->inputEdge(0)->sinkRateValue();
            break;
        case pisdf::VertexType::TAIL: {
            size_t inputCount = 1;
            auto rate = vertex_->outputEdge(0)->sourceRateValue();
            for (auto it = vertex_->inputEdgeVector().rbegin(); it != vertex_->inputEdgeVector().rend(); ++it) {
                const auto &inRate = (*it)->sinkRateValue();
                if (inRate >= rate) {
                    break;
                }
                rate -= inRate;
                inputCount++;
            }
            message.inputParams_ = spider::array<int64_t>(4 + inputCount, StackID::RUNTIME);
            /* = Number of input = */
            message.inputParams_[0] = static_cast<int64_t>(inputEdgeCount);
            /* = First input to be considered = */
            message.inputParams_[1] = static_cast<int64_t>(inputEdgeCount - inputCount);
            /* = Offset in the first buffer if any = */
            message.inputParams_[2] = vertex_->inputEdge(inputEdgeCount - inputCount)->sinkRateValue() - rate;
            /* = Effective size to copy of the first input = */
            message.inputParams_[3] = rate;
            size_t i = 4;
            for (auto it = vertex_->inputEdgeVector().rbegin();
                 it != vertex_->inputEdgeVector().rbegin() + static_cast<long>(inputCount) - 1; ++it) {
                message.inputParams_[i++] = (*it)->sinkRateValue();
            }
        }
            break;
        case pisdf::VertexType::HEAD: {
            size_t inputCount = 1;
            auto rate = vertex_->outputEdge(0)->sourceRateValue();
            for (auto &edge : vertex_->inputEdgeVector()) {
                const auto &inRate = edge->sinkRateValue();
                if (inRate >= rate) {
                    break;
                }
                rate -= inRate;
                inputCount++;
            }
            message.inputParams_ = spider::array<int64_t>(1 + inputCount, StackID::RUNTIME);
            message.inputParams_[0] = static_cast<int64_t>(inputCount);
            rate = vertex_->outputEdge(0)->sourceRateValue();
            for (size_t i = 0; i < inputCount; ++i) {
                const auto &inRate = vertex_->inputEdge(i)->sinkRateValue();
                message.inputParams_[i + 1] = std::min(inRate, rate);
                rate -= inRate;
            }
        }
            break;
        case pisdf::VertexType::INIT:
            message.inputParams_ = spider::array<int64_t>(2, StackID::RUNTIME);
            message.inputParams_[0] = static_cast<int64_t>(0);
            message.inputParams_[1] = vertex_->outputEdge(0)->sourceRateValue();
            break;
        case pisdf::VertexType::END:
            message.inputParams_ = spider::array<int64_t>(2, StackID::RUNTIME);
            message.inputParams_[0] = static_cast<int64_t>(0);
            message.inputParams_[1] = vertex_->inputEdge(0)->sinkRateValue();
            break;
        case pisdf::VertexType::DELAY:
        case pisdf::VertexType::GRAPH:
        case pisdf::VertexType::INPUT:
        case pisdf::VertexType::OUTPUT:
        default:
            throwSpiderException("unhandled type of vertex.");
    }
    return message;
}