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

#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Edge.h>
#include <scheduling/task/TaskVertex.h>

/* === Static function === */

/* === Method(s) implementation === */

/* === Private method(s) implementation === */

spider::sched::TaskVertex::TaskVertex(pisdf::Vertex *vertex) : Task(), vertex_{ vertex } {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    fifos_ = spider::make_shared<TaskFifos, StackID::SCHEDULE>(vertex->inputEdgeCount(), vertex->outputEdgeCount());
}

spider::sched::AllocationRule spider::sched::TaskVertex::allocationRuleForInputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->inputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto size = static_cast<size_t>(vertex_->inputEdge(ix)->sinkRateValue());
    switch (vertex_->subtype()) {
        case pisdf::VertexType::FORK:
            return { 0U, size, AllocType::SAME, FifoAttribute::RW_ONLY };
        case pisdf::VertexType::DUPLICATE:
            return { 0U, size, AllocType::SAME, FifoAttribute::RW_ONLY };
        case pisdf::VertexType::EXTERN_OUT: {
            const auto ref = vertex_->reference()->convertTo<pisdf::ExternInterface>();
            return { ref->bufferIndex(), size, AllocType::NEW, FifoAttribute::RW_EXT };
        }
        case pisdf::VertexType::REPEAT:
            if (size == static_cast<size_t>(vertex_->outputEdge(ix)->sourceRateValue())) {
                return { 0U, size, AllocType::SAME, FifoAttribute::RW_ONLY };
            }
            return { 0U, size, AllocType::SAME, FifoAttribute::RW_OWN };
        default:
            return { 0U, size, AllocType::SAME, FifoAttribute::RW_OWN };
    }
}

spider::sched::AllocationRule spider::sched::TaskVertex::allocationRuleForOutputFifo(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->outputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    const auto size = static_cast<size_t>(vertex_->outputEdge(ix)->sourceRateValue());
    switch (vertex_->subtype()) {
        case pisdf::VertexType::FORK: {
            size_t offset{ 0U };
            for (size_t i = 0U; i < ix; ++i) {
                offset += static_cast<size_t>(vertex_->outputEdge(i)->sourceRateValue());
            }
            return { offset, size, AllocType::SAME, FifoAttribute::RW_ONLY };
        }
        case pisdf::VertexType::DUPLICATE:
            return { 0U, size, AllocType::SAME, FifoAttribute::RW_ONLY };
        case pisdf::VertexType::EXTERN_IN: {
            const auto ref = vertex_->reference()->convertTo<pisdf::ExternInterface>();
            return { ref->bufferIndex(), size, AllocType::NEW, FifoAttribute::RW_EXT };
        }
        case pisdf::VertexType::REPEAT:
            if (size == static_cast<size_t>(vertex_->inputEdge(ix)->sinkRateValue())) {
                return { 0U, size, AllocType::SAME, FifoAttribute::RW_ONLY };
            }
            return { 0U, size, AllocType::NEW, FifoAttribute::RW_OWN };
        default:
            return { 0U, size, AllocType::NEW, FifoAttribute::RW_OWN };
    }
}

spider::sched::Task *spider::sched::TaskVertex::previousTask(size_t ix) const {
#ifndef NDEBUG
    if (ix >= vertex_->inputEdgeCount()) {
        throwSpiderException("index out of bound.");
    }
#endif
    return nullptr;
}

u32 spider::sched::TaskVertex::color() const {
    const auto *reference = vertex_->reference();
    const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 3u) * 50 + 100);
    const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 2u) * 50 + 100);
    const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(reference) >> 4u) * 50 + 100);
    return 0u | (red << 16u) | (green << 8u) | (blue);
}
