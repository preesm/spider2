/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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

#include <graphs-tools/transformation/optims/helper/partialSingleRate.h>
#include <api/pisdf-api.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Edge.h>

/* === Static function(s) === */

static size_t computeNEdge(int64_t rate, spider::array<spider::EdgeLinker>::iterator it) {
    size_t edgeCount = 0;
    int64_t totalRate = 0;
    while (rate > totalRate) {
        totalRate += (*(it++)).rate_;
        edgeCount += 1;
    }
    return edgeCount;
}

/* === Function(s) definition === */

void spider::partialSingleRateTransformation(pisdf::Graph *graph,
                                             array<spider::EdgeLinker> &sourceArray,
                                             array<spider::EdgeLinker> &sinkArray) {
    auto sourceIt = std::begin(sourceArray);
    auto sinkIt = std::begin(sinkArray);
    while (sinkIt != std::end(sinkArray)) {
        if (sinkIt->rate_ == sourceIt->rate_) {
            api::createEdge(sourceIt->vertex_, sourceIt->portIx_, sourceIt->rate_, sinkIt->vertex_, sinkIt->portIx_,
                            sinkIt->rate_);
            sourceIt++;
            sinkIt++;
        } else if (sourceIt->rate_ > sinkIt->rate_) {
            /* == Need for a Fork == */
            auto name = std::string("fork::").append(sourceIt->vertex_->name()).append("::out::").append(
                    std::to_string(sourceIt->portIx_));
            const auto nForkEdge = computeNEdge(sourceIt->rate_, sinkIt);
            auto *addedFork = spider::api::createFork(graph, std::move(name), nForkEdge);
            api::createEdge(sourceIt->vertex_, sourceIt->portIx_, sourceIt->rate_, addedFork, 0, sourceIt->rate_);
            for (size_t forkPortIx = 0; forkPortIx < (nForkEdge - 1); ++forkPortIx) {
                api::createEdge(addedFork, forkPortIx, sinkIt->rate_,
                                sinkIt->vertex_, sinkIt->portIx_, sinkIt->rate_);
                sourceIt->rate_ -= sinkIt->rate_;
                sinkIt++;
            }
            sourceIt->vertex_ = addedFork;
            sourceIt->portIx_ = (nForkEdge - 1);
        } else {
            /* == Need for a Join == */
            auto name = std::string("join::").append(sinkIt->vertex_->name()).append("::in::").append(
                    std::to_string(sinkIt->portIx_));
            const auto nJoinEdge = computeNEdge(sinkIt->rate_, sourceIt);
            auto *addedJoin = api::createJoin(graph, std::move(name), nJoinEdge);
            api::createEdge(addedJoin, 0, sinkIt->rate_, sinkIt->vertex_, sinkIt->portIx_, sinkIt->rate_);
            for (size_t joinPortIx = 0; joinPortIx < (nJoinEdge - 1); ++joinPortIx) {
                api::createEdge(sourceIt->vertex_, sourceIt->portIx_, sourceIt->rate_,
                                addedJoin, joinPortIx, sourceIt->rate_);
                sinkIt->rate_ -= sourceIt->rate_;
                sourceIt++;
            }
            sinkIt->vertex_ = addedJoin;
            sinkIt->portIx_ = (nJoinEdge - 1);
        }
    }
}
#endif