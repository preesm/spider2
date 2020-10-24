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

#include <graphs/sched/SpecialSchedVertex.h>
#include <graphs/sched/SchedGraph.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <archi/MemoryBus.h>
#include <api/archi-api.h>
#include <runtime/common/RTKernel.h>
#include <runtime/special-kernels/specialKernels.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::SpecialVertex::SpecialVertex(spider::sched::Type type, size_t edgeINCount, size_t edgeOUTCount) :
        Vertex(edgeINCount, edgeOUTCount), type_{ type } {
    const auto isValidType = type == Type::MERGE || type == Type::FORK || type == Type::DUPLICATE;
    if (!isValidType) {
        // LCOV_IGNORE
        throwSpiderException("Special schedule vertex expected one the three types: MERGE, FORK or DUPLICATE.");
    }
    if (type == Type::MERGE && edgeOUTCount > 1) {
        // LCOV_IGNORE
        throwSpiderException("MERGE schedule vertex has too many output edges.");
    } else if (edgeINCount > 1) {
        // LCOV_IGNORE
        throwSpiderException("FORK / DUPLICATE schedule vertex has too many input edges.");
    }
}

u64 spider::sched::SpecialVertex::timingOnPE(const spider::PE *pe) const {
    u64 time = 0;
    if (type_ == Type::MERGE) {
        for (const auto *edge : Vertex::inputEdges()) {
            const auto *source = edge->source();
            if (source) {
                time += archi::platform()->dataCommunicationCostPEToPE(source->mappedPe(), pe, edge->rate());
            }
        }
    } else {
        for (const auto *edge : Vertex::outputEdges()) {
            const auto *sink = edge->sink();
            if (sink) {
                time += archi::platform()->dataCommunicationCostPEToPE(pe, sink->mappedPe(), edge->rate());
            }
        }
    }
    return time;
}

bool spider::sched::SpecialVertex::reduce(spider::sched::Graph *) {
    switch (type()) {
        case Type::FORK:
        case Type::DUPLICATE:
            reduceForkDuplicate();
            break;
        default:
            break;
    }
    return false;
}

/* === Private method(s) === */

void spider::sched::SpecialVertex::reduceForkDuplicate() {
    auto isOptimizable = Vertex::state() == State::READY;
    /* == Check if predecessors are running or not == */
    auto *edgeIn = Vertex::inputEdge(0);
    auto *source = edgeIn->source();
    if (!source || source->state() != State::READY) {
        isOptimizable = false;
    }
    if (isOptimizable) {
        i32 count = 0;
        for (const auto *edge : Vertex::outputEdges()) {
            count += edge->getAlloc().count_;
        }
        auto fifo = edgeIn->getAlloc();
        fifo.count_ += count;
        source->setOutputFifo(edgeIn->sourcePortIx(), fifo);
        /* == update input vertex with notification flags == */
        Vertex::setState(State::SKIPPED);
    }
}

u32 spider::sched::SpecialVertex::getKernelIx() const {
    if (type_ == Type::MERGE) {
        return rt::JOIN_KERNEL_IX;
    } else if (type_ == Type::FORK) {
        return rt::FORK_KERNEL_IX;
    } else {
        return rt::DUPLICATE_KERNEL_IX;
    }
}

spider::unique_ptr<i64> spider::sched::SpecialVertex::buildInputParams() const {
    if (type_ == Type::MERGE) {
        return buildMergeParams();
    } else if (type_ == Type::FORK) {
        return buildForkParams();
    } else {
        return buildDuplicateParams();
    }
}

spider::unique_ptr<i64> spider::sched::SpecialVertex::buildMergeParams() const {
    auto params = spider::allocate<i64, StackID::RUNTIME>(Vertex::inputEdgeCount() + 2);
#ifndef NDEBUG
    if (!params) {
        throwNullptrException();
    }
#endif
    params[0] = static_cast<i64>(Vertex::outputEdge(0)->rate());
    params[1] = static_cast<i64>(Vertex::inputEdgeCount());
    for (const auto *edge : Vertex::inputEdges()) {
        params[2 + edge->sinkPortIx()] = static_cast<i64>(edge->rate());
    }
    return make_unique(params);
}

spider::unique_ptr<i64> spider::sched::SpecialVertex::buildForkParams() const {
    auto params = spider::allocate<i64, StackID::RUNTIME>(Vertex::outputEdgeCount() + 2);
#ifndef NDEBUG
    if (!params) {
        throwNullptrException();
    }
#endif
    params[0] = static_cast<i64>(Vertex::inputEdge(0)->rate());
    params[1] = static_cast<i64>(Vertex::outputEdgeCount());
    for (const auto *edge : Vertex::outputEdges()) {
        params[2 + edge->sourcePortIx()] = static_cast<i64>(edge->rate());
    }
    return make_unique(params);
}

spider::unique_ptr<i64> spider::sched::SpecialVertex::buildDuplicateParams() const {
    auto params = spider::allocate<i64, StackID::RUNTIME>(2);
#ifndef NDEBUG
    if (!params) {
        throwNullptrException();
    }
#endif
    params[0] = static_cast<i64>(Vertex::outputEdgeCount());
    params[1] = static_cast<i64>(Vertex::inputEdge(0)->rate());
    return make_unique(params);
}
