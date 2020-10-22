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

#include <graphs-tools/transformation/pisdf/GraphHandler.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs/pisdf/Graph.h>
#include <containers/vector.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::pisdf::GraphHandler::GraphHandler(const spider::pisdf::Graph *graph,
                                           const spider::vector<std::shared_ptr<pisdf::Param>> &params,
                                           u32 repetitionCount,
                                           const pisdf::GraphFiring *handler) :
        handler_{ handler },
        graph_{ graph },
        repetitionCount_{ repetitionCount } {
    static_ = true;
    firings_ = spider::make_unique(spider::make_n<GraphFiring *, StackID::TRANSFO>(repetitionCount, nullptr));
    for (const auto &param : graph->params()) {
        if (param->dynamic()) {
            static_ &= graph->configVertexCount() > 0;
            break;
        }
    }
    const auto *parentGraph = graph->graph();
    for (u32 k = 0; k < repetitionCount; ++k) {
        firings_.get()[k] = spider::make<GraphFiring>(this, params, k);
    }
    if (static_ || (parentGraph && !parentGraph->configVertexCount())) {
        resolveFirings();
    }
}

spider::pisdf::GraphHandler::~GraphHandler() {
    for (u32 k = 0; k < repetitionCount_; ++k) {
        destroy(firings_.get()[k]);
    }
}

void spider::pisdf::GraphHandler::clear() {
    for (auto &firing : firings()) {
        if (firing) {
            firing->clear();
        }
    }
}

void spider::pisdf::GraphHandler::resolveFirings() {
    if (repetitionCount_ && !graph_->configVertexCount()) {
        auto *firstFiring = firings_.get()[0u];
        firstFiring->resolveBRV();
        for (u32 k = 1; k < repetitionCount_; ++k) {
            firings_.get()[k]->apply(firstFiring);
        }
    } else {
        for (u32 k = 0; k < repetitionCount_; ++k) {
            firings_.get()[k]->resolveBRV();
        }
    }
}
