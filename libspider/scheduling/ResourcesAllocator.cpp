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

#include <scheduling/ResourcesAllocator.h>

#ifndef _NO_BUILD_LEGACY_RT

#include <scheduling/scheduler/srdag-based/ListScheduler.h>
#include <scheduling/scheduler/srdag-based/GreedyScheduler.h>
#include <scheduling/memory/srdag-based/NoSyncFifoAllocator.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <graphs/srdag/SRDAGEdge.h>
#include <graphs/sched/SRDAGSchedVertex.h>

#endif

#include <scheduling/scheduler/pisdf-based/PiSDFGreedyScheduler.h>
#include <scheduling/scheduler/pisdf-based/PiSDFListScheduler.h>
#include <scheduling/mapper/BestFitMapper.h>
#include <scheduling/memory/FifoAllocator.h>
#include <scheduling/memory/pisdf-based/PiSDFFifoAllocator.h>
#include <scheduling/task/Task.h>
#include <graphs/sched/SchedGraph.h>
#include <graphs/sched/SpecialSchedVertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <archi/PE.h>
#include <common/Time.h>

/* === Static function === */

static void checkFifoAllocatorTraits(const spider::sched::FifoAllocator *allocator, spider::ExecutionPolicy policy) {
    switch (policy) {
        case spider::ExecutionPolicy::JIT:
            if (!allocator->traits_.jitAllocator_) {
                throwSpiderException("Using a scheduler in JIT_SEND mode with incompatible fifo allocator.");
            }
            break;
        case spider::ExecutionPolicy::DELAYED:
            if (!allocator->traits_.postSchedulingAllocator_) {
                throwSpiderException("Using a scheduler in DELAYED_SEND mode with incompatible fifo allocator.");
            }
            break;
    }
}

/* === Method(s) implementation === */

spider::sched::ResourcesAllocator::ResourcesAllocator(SchedulingPolicy schedulingPolicy,
                                                      MappingPolicy mappingPolicy,
                                                      ExecutionPolicy executionPolicy,
                                                      FifoAllocatorType allocatorType,
                                                      bool legacy) :
        scheduler_{ spider::make_unique(allocateScheduler(schedulingPolicy, legacy)) },
        mapper_{ spider::make_unique(allocateMapper(mappingPolicy)) },
        schedule_{ spider::make_unique<Schedule, StackID::SCHEDULE>() },
        allocator_{ spider::make_unique(allocateAllocator(allocatorType, legacy)) },
        executionPolicy_{ executionPolicy } {
    if (allocator_) {
        checkFifoAllocatorTraits(allocator_.get(), executionPolicy);
        allocator_->setSchedule(schedule_.get());
    }
}

#ifndef _NO_BUILD_LEGACY_RT

void spider::sched::ResourcesAllocator::execute(const srdag::Graph *graph) {
    /* == Schedule the graph == */
    const auto result = scheduler_->schedule(graph);
    /* == Add vertices to the sched::Graph and allocate memory == */
    auto *schedGraph = schedule_->scheduleGraph();
    const auto currentSize = schedGraph->vertexCount();
    createScheduleVertices(result);
    /* == Map and execute the scheduled tasks == */
    applyExecPolicy(currentSize);
}

#endif

void spider::sched::ResourcesAllocator::execute(pisdf::GraphHandler *graphHandler) {
    /* == Schedule the graph == */
    scheduler_->schedule(graphHandler);
    /* == Map and execute the scheduled tasks == */
    applyExecPolicy();
}

void spider::sched::ResourcesAllocator::clear() {
    allocator_->clear();
    schedule_->clear();
    scheduler_->clear();
}

/* === Private method(s) implementation === */

spider::sched::Scheduler *
spider::sched::ResourcesAllocator::allocateScheduler(SchedulingPolicy policy, bool legacy) {
    switch (policy) {
        case SchedulingPolicy::LIST:
            if (legacy) {
#ifndef _NO_BUILD_LEGACY_RT
                return spider::make<sched::ListScheduler, StackID::SCHEDULE>();
#else
                return nullptr;
#endif
            }
            return spider::make<sched::PiSDFListScheduler, StackID::SCHEDULE>();
        case SchedulingPolicy::GREEDY:
            if (legacy) {
#ifndef _NO_BUILD_LEGACY_RT
                return spider::make<sched::GreedyScheduler, StackID::SCHEDULE>();
#else
                return nullptr;
#endif
            } else {
                return spider::make<sched::PiSDFGreedyScheduler, StackID::SCHEDULE>();
            }
        default:
            throwSpiderException("unsupported scheduling policy.");
    }
}

spider::sched::FifoAllocator *
spider::sched::ResourcesAllocator::allocateAllocator(FifoAllocatorType type, bool legacy) {
    switch (type) {
        case spider::FifoAllocatorType::DEFAULT:
            if (!legacy) {
                return spider::make<spider::sched::PiSDFFifoAllocator, StackID::RUNTIME>();
            }
#ifndef _NO_BUILD_LEGACY_RT
            return spider::make<spider::sched::SRDAGFifoAllocator, StackID::RUNTIME>();
#else
            printer::fprintf(stderr, "Default allocator is part of the legacy runtime which was not built.\n"
                                     "Rebuild the Spider 2.0 library with the cmake flag -DBUILD_LEGACY_RUNTIME=ON.\n");
            return nullptr;
#endif
        case spider::FifoAllocatorType::DEFAULT_NOSYNC:
            if (!legacy) {
                return spider::make<spider::sched::PiSDFFifoAllocator, StackID::RUNTIME>();
            }
#ifndef _NO_BUILD_LEGACY_RT
            return spider::make<spider::sched::NoSyncFifoAllocator, StackID::RUNTIME>();
#else
            printer::fprintf(stderr, "NO_SYNC allocator is part of the legacy runtime which was not built.\n"
                                     "Rebuild the Spider 2.0 library with the cmake flag -DBUILD_LEGACY_RUNTIME=ON.\n");
            return nullptr;
#endif
        case spider::FifoAllocatorType::ARCHI_AWARE:
        default:
            throwSpiderException("unsupported type of FifoAllocator.");
    }
}

spider::sched::Mapper *spider::sched::ResourcesAllocator::allocateMapper(MappingPolicy policy) {
    switch (policy) {
        case MappingPolicy::BEST_FIT:
            return spider::make<sched::BestFitMapper, StackID::SCHEDULE>();
        default:
            throwSpiderException("unsupported mapping policy.");
    }
}

ufast64 spider::sched::ResourcesAllocator::computeMinStartTime() const {
    ufast64 minStartTime = UINT_FAST64_MAX;
    for (const auto *pe : archi::platform()->peArray()) {
        minStartTime = std::min(minStartTime, schedule_->stats().endTime(pe->virtualIx()));
    }
    return minStartTime;
}

void spider::sched::ResourcesAllocator::applyExecPolicy(size_t vertexOffset) {
    mapper_->setStartTime(computeMinStartTime());
    auto *schedGraph = schedule_->scheduleGraph();
    switch (executionPolicy_) {
        case ExecutionPolicy::JIT:
            /* == Map and send tasks == */
            for (auto k = vertexOffset; k < schedGraph->vertexCount(); ++k) {
                /* == Map the task == */
                mapper_->map(schedGraph, schedGraph->vertex(k), schedule_.get());
                /* == Send the task == */
                schedGraph->vertex(k)->send();
            }
            break;
        case ExecutionPolicy::DELAYED: {
            /* == Map every tasks == */
            for (auto k = vertexOffset; k < schedGraph->vertexCount(); ++k) {
                /* == Map the task == */
                mapper_->map(schedGraph, schedGraph->vertex(k), schedule_.get());
            }
            /* == Send every tasks == */
            for (auto k = vertexOffset; k < schedGraph->vertexCount(); ++k) {
                /* == Send the task == */
                schedGraph->vertex(k)->send();
            }
        }
            break;
        default:
            throwSpiderException("unsupported execution policy.");
    }
}

#ifndef _NO_BUILD_LEGACY_RT

void spider::sched::ResourcesAllocator::createScheduleVertices(const spider::vector<srdag::Vertex *> &vertices) {
    auto *schedGraph = schedule_->scheduleGraph();
    for (auto *vertex : vertices) {
        auto *schedVertex = spider::make<sched::SRDAGVertex, StackID::SCHEDULE>(vertex);
        /* == Connect input edges == */
        for (const auto *edge : vertex->inputEdges()) {
            const auto *source = edge->source();
            if (!edge->rate() || !source->executable()) {
                continue;
            }
            const auto *srcSchedVertex = schedGraph->vertex(source->scheduleTaskIx());
#ifndef NDEBUG
            if (!srcSchedVertex) {
                throwNullptrException();
            }
#endif
            auto *schedEdge = srcSchedVertex->outputEdge(edge->sourcePortIx());
            if (schedEdge->sink()) {
                throwSpiderException("edge already has a sink.");
            }
            schedEdge->setSink(schedVertex, static_cast<u32>(edge->sinkPortIx()));
        }
        /* == Allocate output edges (if needed) == */
        if (vertex->subtype() == pisdf::VertexType::FORK) {
            allocateForkOutputEdges(vertex, schedVertex, schedGraph);
        } else if (vertex->subtype() == pisdf::VertexType::DUPLICATE) {
            allocateDupOutputEdges(vertex, schedVertex, schedGraph);
        } else {
            allocateOutputEdges(vertex, schedVertex, schedGraph);
        }
        /* == Add the vertex to the sched::Graph == */
        schedGraph->addVertex(schedVertex);
    }
}

void spider::sched::ResourcesAllocator::allocateOutputEdges(srdag::Vertex *vertex,
                                                            sched::Vertex *schedVertex,
                                                            sched::Graph *schedGraph) {
    for (const auto *edge : vertex->outputEdges()) {
        Fifo fifo{ };
        auto *sink = edge->sink();
        if (sink && sink->subtype() == pisdf::VertexType::EXTERN_OUT) {
            const auto *reference = sink->reference()->convertTo<pisdf::ExternInterface>();
            fifo.size_ = static_cast<u32>(edge->rate());
            fifo.offset_ = 0;
            fifo.count_ = fifo.size_ ? 1 : 0;
            fifo.virtualAddress_ = reference->bufferIndex();
            fifo.attribute_ = FifoAttribute::RW_EXT;
        } else {
            fifo = allocator_->allocate(static_cast<size_t>(edge->rate()));
        }
        schedGraph->createEdge(schedVertex, static_cast<u32>(edge->sourcePortIx()), nullptr, 0, fifo);
    }
}

void spider::sched::ResourcesAllocator::allocateForkOutputEdges(srdag::Vertex *vertex,
                                                                sched::Vertex *schedVertex,
                                                                sched::Graph *schedGraph) {
    const auto *inputEdge = schedVertex->inputEdge(0U);
    const auto inputFifo = inputEdge->getAlloc();
    u32 offset = 0;
    for (const auto *edge : vertex->outputEdges()) {
        Fifo fifo{ };
        fifo.size_ = static_cast<u32>(edge->sourceRateValue());
        fifo.offset_ = inputFifo.offset_ + offset;
        fifo.count_ = fifo.size_ ? 1 : 0;
        fifo.virtualAddress_ = inputFifo.virtualAddress_;
        fifo.attribute_ = FifoAttribute::RW_ONLY;
        offset += fifo.size_;
        schedGraph->createEdge(schedVertex, static_cast<u32>(edge->sourcePortIx()), nullptr, 0, fifo);
    }
}

void spider::sched::ResourcesAllocator::allocateDupOutputEdges(srdag::Vertex *vertex,
                                                               sched::Vertex *schedVertex,
                                                               sched::Graph *schedGraph) {
    const auto *inputEdge = schedVertex->inputEdge(0U);
    const auto inputFifo = inputEdge->getAlloc();
    for (const auto *edge : vertex->outputEdges()) {
        auto fifo = inputFifo;
        fifo.count_ = fifo.size_ ? 1 : 0;
        fifo.attribute_ = FifoAttribute::RW_ONLY;
        schedGraph->createEdge(schedVertex, static_cast<u32>(edge->sourcePortIx()), nullptr, 0, fifo);
    }
}

#endif
