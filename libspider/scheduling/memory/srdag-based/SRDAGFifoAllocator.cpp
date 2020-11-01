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

#include <scheduling/memory/srdag-based/SRDAGFifoAllocator.h>
#include <scheduling/task/SRDAGTask.h>
#include <graphs/srdag/SRDAGEdge.h>
#include <graphs/srdag/SRDAGGraph.h>
#include <graphs/pisdf/ExternInterface.h>
#include <archi/MemoryInterface.h>
#include <api/archi-api.h>

/* === Function(s) definition === */

spider::unique_ptr<spider::JobFifos> spider::sched::SRDAGFifoAllocator::buildJobFifos(SRDAGTask *task) {
    const auto *vertex = task->vertex();
    auto fifos = spider::make_unique<JobFifos, StackID::RUNTIME>(static_cast<u32>(vertex->inputEdgeCount()),
                                                                 static_cast<u32>(vertex->outputEdgeCount()));
    /* == Allocate input fifos == */
    for (const auto *edge : vertex->inputEdges()) {
        fifos->setInputFifo(edge->sinkPortIx(), buildInputFifo(edge));
    }
    /* == Allocate output fifos == */
    allocate(task);
    for (const auto *edge : vertex->outputEdges()) {
        fifos->setOutputFifo(edge->sourcePortIx(), buildOutputFifo(edge));
    }
    return fifos;
}

/* === Private methods === */

void spider::sched::SRDAGFifoAllocator::allocate(SRDAGTask *task) {
    u32 offset = 0;
    const auto *vertex = task->vertex();
    switch (vertex->subtype()) {
        case pisdf::VertexType::EXTERN_IN: {
            const auto *ext = vertex->reference()->convertTo<pisdf::ExternInterface>();
            vertex->outputEdge(0)->setAddress(ext->address());
        }
            break;
        case pisdf::VertexType::FORK:
            for (auto *edge : vertex->outputEdges()) {
                const auto *inputEdge = vertex->inputEdge(0);
                edge->setAddress(inputEdge->address());
                edge->setOffset(inputEdge->offset() + offset);
                offset += static_cast<u32>(edge->rate());
            }
            break;
        case pisdf::VertexType::DUPLICATE:
            for (auto *edge : vertex->outputEdges()) {
                const auto *inputEdge = vertex->inputEdge(0);
                edge->setAddress(inputEdge->address());
                edge->setOffset(inputEdge->offset());
            }
            break;
        default:
            for (auto *edge : vertex->outputEdges()) {
                if (edge->sink()->subtype() == pisdf::VertexType::EXTERN_OUT) {
                    const auto *ext = edge->sink()->reference()->convertTo<pisdf::ExternInterface>();
                    edge->setAddress(ext->address());
                } else {
                    edge->setAddress(FifoAllocator::allocate(static_cast<size_t>(edge->rate())));
                    edge->setOffset(0);
                }
            }
            break;
    }
}

spider::Fifo spider::sched::SRDAGFifoAllocator::buildInputFifo(const srdag::Edge *edge) {
    Fifo fifo{ };
    fifo.address_ = edge->address();
    fifo.offset_ = edge->offset();
    fifo.size_ = static_cast<u32>(edge->rate());
    fifo.count_ = 0;
    fifo.attribute_ = FifoAttribute::RW_OWN;
    if (edge->source()->subtype() == pisdf::VertexType::EXTERN_IN ||
        edge->sink()->subtype() == pisdf::VertexType::EXTERN_OUT) {
        fifo.attribute_ = FifoAttribute::RW_EXT;
    }
    return fifo;
}

spider::Fifo spider::sched::SRDAGFifoAllocator::buildOutputFifo(const srdag::Edge *edge) {
    Fifo fifo{ };
    fifo.address_ = edge->address();
    fifo.offset_ = edge->offset();
    fifo.size_ = static_cast<u32>(edge->rate());
    fifo.count_ = 1;
    fifo.attribute_ = FifoAttribute::RW_OWN;
    /* == Set attribute == */
    const auto sourceSubType = edge->source()->subtype();
    const auto sinkSubType = edge->sink()->subtype();
    if (sourceSubType == pisdf::VertexType::EXTERN_IN || sinkSubType == pisdf::VertexType::EXTERN_OUT) {
        fifo.attribute_ = FifoAttribute::RW_EXT;
    } else if (sourceSubType == pisdf::VertexType::FORK || sourceSubType == pisdf::VertexType::DUPLICATE) {
        fifo.attribute_ = FifoAttribute::RW_ONLY;
    }
    return fifo;
}


#endif