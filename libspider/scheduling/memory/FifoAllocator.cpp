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

#include <scheduling/memory/FifoAllocator.h>
#include <scheduling/task/Task.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Graph.h>
#include <api/archi-api.h>
#include <archi/MemoryInterface.h>

/* === Function(s) definition === */

void spider::sched::FifoAllocator::clear() noexcept {
    virtualMemoryAddress_ = reservedMemory_;
}

void spider::sched::FifoAllocator::allocatePersistentDelays(pisdf::Graph *graph) {
    const auto *grt = archi::platform()->spiderGRTPE();
    auto *interface = grt->cluster()->memoryInterface();
    for (const auto &edge : graph->edges()) {
        const auto &delay = edge->delay();
        if (delay && delay->isPersistent()) {
            const auto value = static_cast<size_t>(delay->value());
            auto *buffer = interface->allocate(static_cast<uint64_t>(reservedMemory_), value);
            memset(buffer, 0, value * sizeof(char));
            delay->setMemoryAddress(static_cast<uint64_t>(reservedMemory_));
            delay->setMemoryInterface(interface);
            log::info("Reserving #%.8ld bytes of memory.\n", value);
            reservedMemory_ += value;
        }
    }
    virtualMemoryAddress_ = reservedMemory_;
}

void spider::sched::FifoAllocator::allocate(sched::Task *task) {
    if (!task) {
        return;
    }
    /* == Allocating input FIFOs == */
    size_t ix{ 0u };
    auto inputFifos = task->fifos().inputFifos();
    for (auto &fifo : inputFifos) {
        const auto rule = task->allocationRuleForInputFifo(ix);
        const auto *prevTask = task->previousTask(ix);
        if (prevTask) {
            if (rule.type_ == AllocType::SAME_IN) {
                fifo = prevTask->fifos().outputFifo(rule.index_);
                if (fifo.attribute_ != FifoAttribute::RW_EXT) {
                    fifo.attribute_ = rule.attribute_;
                    fifo.count_ = 0u;
                }
            } else {
                throwSpiderException("invalid AllocAttribute for input FIFO.");
            }
        } else {
            fifo = Fifo{ };
        }
        ix++;
    }

    /* == Allocating output FIFOs == */
    ix = 0u;
    auto outputFifos = task->fifos().outputFifos();
    for (auto &fifo : outputFifos) {
        const auto rule = task->allocationRuleForOutputFifo(ix);
        switch (rule.type_) {
            case NEW:
                fifo.virtualAddress_ = virtualMemoryAddress_;
                virtualMemoryAddress_ += rule.size_;
                fifo.offset_ = 0u;
                break;
            case SAME_IN: {
                auto &inputFifo = inputFifos[rule.index_];
                fifo.virtualAddress_ = inputFifo.virtualAddress_;
                fifo.offset_ = inputFifo.offset_ + static_cast<u32>(rule.offset_);
            }
                break;
            case SAME_OUT: {
                auto &outputFifo = outputFifos[rule.index_];
                fifo.virtualAddress_ = outputFifo.virtualAddress_;
                fifo.offset_ = outputFifo.offset_ + static_cast<u32>(rule.offset_);
            }
                break;
            case EXT:
                fifo.virtualAddress_ = rule.index_;
                break;
        }
        fifo.size_ = static_cast<u32>(rule.size_);
        fifo.attribute_ = rule.attribute_;
        fifo.count_ = (fifo.size_ != 0u);
        ix++;
    }
}