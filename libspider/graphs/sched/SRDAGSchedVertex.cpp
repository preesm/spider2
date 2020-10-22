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

#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <graphs/sched/SRDAGSchedVertex.h>
#include <graphs/sched/SchedGraph.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <graphs-tools/helper/srdag-helper.h>
#include <runtime/common/RTInfo.h>

/* === Static function === */

/* === Method(s) implementation === */

bool spider::sched::SRDAGVertex::isMappableOnPE(const spider::PE *pe) const {
    return vertex_->runtimeInformation()->isPEMappable(pe);
}

u64 spider::sched::SRDAGVertex::timingOnPE(const spider::PE *pe) const {
    return static_cast<u64>(vertex_->runtimeInformation()->timingOnPE(pe, vertex_->inputParamVector()));
}

std::string spider::sched::SRDAGVertex::name() const {
    return vertex_->name();
}

u32 spider::sched::SRDAGVertex::color() const {
    const auto *reference = vertex_->reference();
    const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 3u) * 50 + 100);
    const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 2u) * 50 + 100);
    const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 4u) * 50 + 100);
    return 0u | (red << 16u) | (green << 8u) | (blue);
}

void spider::sched::SRDAGVertex::reduce(sched::Graph *graph) {
    auto isOptimizable = Vertex::state() == State::READY;
    /* == Check if predecessors are running or not == */
    for (const auto *edge : Vertex::inputEdges()) {
        const auto *source = edge->source();
        if (!source || source->state() != State::READY) {
            isOptimizable = false;
            break;
        }
    }
    if (isOptimizable && vertex_->subtype() == pisdf::VertexType::REPEAT) {
        reduceRepeat(graph);
    }
}

void spider::sched::SRDAGVertex::setIx(u32 ix) {
    Vertex::setIx(ix);
    vertex_->setScheduleTaskIx(ix);
}

/* === Private method(s) === */

u32 spider::sched::SRDAGVertex::getOutputParamsCount() const {
    return static_cast<u32>(vertex_->reference()->outputParamCount());
}

u32 spider::sched::SRDAGVertex::getKernelIx() const {
    return static_cast<u32>(vertex_->runtimeInformation()->kernelIx());
}

spider::unique_ptr<i64> spider::sched::SRDAGVertex::buildInputParams() const {
    return srdag::buildVertexRuntimeInputParameters(vertex_);
}

void spider::sched::SRDAGVertex::reduceRepeat(sched::Graph *graph) {
    auto *edgeIn = Vertex::inputEdge(0);
    auto *edgeOut = Vertex::outputEdge(0);
    auto inputFifo = edgeIn->getAlloc();
    const auto outputFifo = edgeOut->getAlloc();
    if (inputFifo.virtualAddress_ == outputFifo.virtualAddress_ &&
        outputFifo.attribute_ != FifoAttribute::RW_OWN) {
        inputFifo.count_ += outputFifo.count_;
        edgeIn->setAlloc(inputFifo);
        edgeIn->setSink(edgeOut->sink(), edgeOut->sinkPortIx());
        graph->removeEdge(edgeOut);
        graph->removeVertex(this);
    }
}

#endif
