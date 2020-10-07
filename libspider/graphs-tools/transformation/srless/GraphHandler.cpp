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

#include <graphs-tools/transformation/srless/GraphHandler.h>
#include <graphs-tools/transformation/srless/GraphFiring.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs/pisdf/Graph.h>
#include <containers/vector.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::srless::GraphHandler::GraphHandler(const spider::pisdf::Graph *graph,
                                           const spider::vector<std::shared_ptr<pisdf::Param>> &params,
                                           u32 repetitionCount,
                                           const srless::GraphFiring *handler) :
        firings_{ factory::vector<GraphFiring *>(StackID::TRANSFO) },
        handler_{ handler },
        graph_{ graph },
        repetitionCount_{ repetitionCount } {
    firings_.reserve(repetitionCount);
    static_ = true;
    for (const auto &param : graph->params()) {
        if (param->dynamic()) {
            static_ &= graph->configVertexCount() > 0;
            break;
        }
    }
    for (u32 k = 0; k < repetitionCount; ++k) {
        firings_.emplace_back(spider::make<GraphFiring>(this, params, k));
        if (static_) {
            firings_[k]->resolveBRV();
        }
    }
}

spider::srless::GraphHandler::~GraphHandler() {
    for (u32 k = 0; k < repetitionCount_; ++k) {
        destroy(firings_[k]);
    }
}

void spider::srless::GraphHandler::clear() {
    for (auto &firing : firings_) {
        if (firing) {
            firing->clear();
        }
    }
}

/* === Private method(s) implementation === */
