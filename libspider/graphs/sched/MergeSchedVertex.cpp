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

#include <graphs/sched/MergeSchedVertex.h>
#include <graphs/sched/SchedGraph.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <archi/MemoryBus.h>
#include <api/archi-api.h>
#include <runtime/common/RTKernel.h>
#include <runtime/special-kernels/specialKernels.h>

/* === Static function === */

/* === Method(s) implementation === */

u64 spider::sched::MergeVertex::timingOnPE(const spider::PE *pe) const {
    u64 time = 0;
    for (const auto *edge : Vertex::inputEdges()) {
        const auto *source = edge->source();
        if (source) {
            const auto *srcPe = edge->source()->mappedPe();
            time += archi::platform()->dataCommunicationCostPEToPE(srcPe, pe, edge->getAlloc().size_);
        }
    }
    return time;
}

/* === Private method(s) === */

u32 spider::sched::MergeVertex::getKernelIx() const {
    return rt::JOIN_KERNEL_IX;
}

spider::unique_ptr<i64> spider::sched::MergeVertex::buildInputParams() const {
    auto params = spider::allocate<i64, StackID::RUNTIME>(Vertex::inputEdgeCount() + 2);
#ifndef NDEBUG
    if (!params) {
        throwNullptrException();
    }
#endif
    params[0] = outputEdge(0)->getAlloc().size_;
    params[1] = static_cast<i64>(Vertex::inputEdgeCount());
    for (const auto *edge : Vertex::inputEdges()) {
        params[2 + edge->sinkPortIx()] = edge->getAlloc().size_;
    }
    return make_unique(params);
}
