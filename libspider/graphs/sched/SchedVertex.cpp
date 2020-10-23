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

#include <graphs/sched/SchedEdge.h>
#include <graphs/sched/SchedVertex.h>
#include <graphs/sched/SchedGraph.h>
#include <runtime/message/JobMessage.h>
#include <runtime/message/Notification.h>
#include <runtime/communicator/RTCommunicator.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime-api.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::Vertex::Vertex(size_t edgeINCount,
                              size_t edgeOUTCount) :
        nINEdges_{ static_cast<u32>(edgeINCount) },
        nOUTEdges_{ static_cast<u32>(edgeOUTCount) } {
    inputEdgeArray_ = spider::make_n<sched::Edge *, StackID::SCHEDULE>(edgeINCount, nullptr);
    outputEdgeArray_ = spider::make_n<sched::Edge *, StackID::SCHEDULE>(edgeOUTCount, nullptr);
    const auto lrtCount{ archi::platform()->LRTCount() };
    notifications_ = make_unique<bool>(spider::allocate<bool, StackID::SCHEDULE>(lrtCount));
    std::fill(notifications_.get(), notifications_.get() + lrtCount, false);
}

spider::sched::Vertex::~Vertex() {
    deallocate(inputEdgeArray_);
    deallocate(outputEdgeArray_);
}

void spider::sched::Vertex::connectInputEdge(sched::Edge *edge, size_t pos) {
    connectEdge(inputEdgeArray_, edge, pos);
}

void spider::sched::Vertex::connectOutputEdge(sched::Edge *edge, size_t pos) {
    connectEdge(outputEdgeArray_, edge, pos);
}

spider::sched::Edge *spider::sched::Vertex::disconnectInputEdge(size_t ix) {
    auto *edge = disconnectEdge(inputEdgeArray_, ix);
    if (edge) {
        /* == Reset the Edge == */
        edge->setSink(nullptr, UINT32_MAX);
    }
    return edge;
}

spider::sched::Edge *spider::sched::Vertex::disconnectOutputEdge(size_t ix) {
    auto *edge = disconnectEdge(outputEdgeArray_, ix);
    if (edge) {
        /* == Reset the Edge == */
        edge->setSource(nullptr, UINT32_MAX);
    }
    return edge;
}

std::pair<ufast64, ufast64> spider::sched::Vertex::computeCommunicationCost(const spider::PE *mappedPE) const {
    const auto *platform = archi::platform();
    ufast64 externDataToReceive = 0u;
    /* == Compute communication cost == */
    ufast64 communicationCost = 0;
    for (const auto *edge : inputEdges()) {
        const auto rate = edge->rate();
        const auto source = edge->source();
        if (rate && source && source->state() != State::NOT_RUNNABLE) {
            const auto *mappedPESource = source->mappedPe();
            communicationCost += platform->dataCommunicationCostPEToPE(mappedPESource, mappedPE, rate);
            if (mappedPE->cluster() != mappedPESource->cluster()) {
                externDataToReceive += rate;
            }
        }
    }
    return { communicationCost, externDataToReceive };
}

void spider::sched::Vertex::send() {
    if (state_ != State::READY) {
        return;
    }
    JobMessage message{ };
    /* == Set core properties == */
    message.nParamsOut_ = this->getOutputParamsCount();
    message.kernelIx_ = this->getKernelIx();
    message.taskIx_ = ix_;
    message.ix_ = jobExecIx_;
    /* == Set the synchronization flags == */
    message.synchronizationFlags_ = buildJobNotificationFlags();
    /* == Set input params == */
    message.inputParams_ = this->buildInputParams();
    /* == Set Fifos == */
    message.fifos_ = buildJobFifos();
    /* == Send the job == */
    const auto grtIx = archi::platform()->getGRTIx();
    auto *communicator = rt::platform()->communicator();
    const auto mappedLRTIx = mappedLRT()->virtualIx();
    const auto messageIx = communicator->push(std::move(message), mappedLRTIx);
    communicator->push(Notification{ NotificationType::JOB_ADD, grtIx, messageIx }, mappedLRTIx);
    /* == Set job in TaskState::RUNNING == */
    state_ = State::RUNNING;
}

/* === Private method(s) implementation === */

spider::sched::Edge *spider::sched::Vertex::disconnectEdge(sched::Edge **edges, size_t ix) {
    auto *&edge = edges[ix];
    auto *ret = edge;
    if (edge) {
        edge = nullptr;
    }
    return ret;
}

void spider::sched::Vertex::connectEdge(sched::Edge **edges, sched::Edge *edge, size_t ix) {
    auto *&current = edges[ix];
    if (!current) {
        current = edge;
    } else {
        throwSpiderException("Edge already exists at position: %zu", ix);
    }
}

spider::unique_ptr<bool> spider::sched::Vertex::buildJobNotificationFlags() const {
    auto shouldBroadcast = false;
    /* == Check if task are not ready == */
    for (const auto *edge : outputEdges()) {
        const auto *sink = edge->sink();
        if (!sink || (sink->state() != State::READY && sink->state() != State::SKIPPED)) {
            shouldBroadcast = true;
            break;
        }
    }
    const auto lrtCount{ archi::platform()->LRTCount() };
    if (!shouldBroadcast) {
        /* == Update values == */
        auto *flags = notifications_.get();
        bool oneTrue = false;
        for (const auto *edge : outputEdges()) {
            const auto *sink = edge->sink();
            auto &currentFlag = flags[sink->mappedLRT()->virtualIx()];
            if (!currentFlag) {
                currentFlag = true;
                for (const auto *inEdge : sink->inputEdges()) {
                    const auto *source = inEdge->source();
                    if (source && (source != this) && (source->jobExecIx_ > jobExecIx_)) {
                        currentFlag = false;
                        break;
                    }
                }
            }
            oneTrue |= currentFlag;
        }
        if (oneTrue) {
            auto result = spider::allocate<bool, StackID::RUNTIME>(lrtCount);
            std::copy(flags, flags + lrtCount, result);
            return make_unique(result);
        } else {
            return spider::unique_ptr<bool>();
        }
    } else {
        /* == broadcast to every LRT == */
        return make_unique(spider::make_n<bool, StackID::RUNTIME>(lrtCount, true));
    }
}

std::shared_ptr<spider::JobFifos> spider::sched::Vertex::buildJobFifos() const {
    auto fifos = spider::make_shared<JobFifos, StackID::RUNTIME>(nINEdges_, nOUTEdges_);
    for (const auto *edge : inputEdges()) {
        auto fifo = edge->getAlloc();
        fifo.count_ = 0;
        if ((fifo.attribute_ != FifoAttribute::RW_EXT) && (fifo.attribute_ != FifoAttribute::RW_AUTO)) {
            fifo.attribute_ = FifoAttribute::RW_OWN;
        }
        fifos->setInputFifo(edge->sinkPortIx(), fifo);
    }
    for (const auto *edge : outputEdges()) {
        fifos->setOutputFifo(edge->sourcePortIx(), edge->getAlloc());
    }
    return fifos;
}
