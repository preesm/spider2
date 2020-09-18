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

#include <graphs-tools/transformation/srdagless/FiringHandler.h>
#include <graphs-tools/transformation/srdagless/GraphHandler.h>
#include <graphs/pisdf/Graph.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::FiringHandler::FiringHandler(const GraphHandler *parent,
                                     spider::vector<std::shared_ptr<pisdf::Param>> &params) :
        children_{ factory::vector<spider::unique_ptr<GraphHandler>>(StackID::TRANSFO) },
        params_{ factory::vector<std::shared_ptr<pisdf::Param>>(StackID::TRANSFO) },
        parent_{ parent } {
    const auto *graph = parent->graph();
    children_.reserve(graph->subgraphCount());
    params_.reserve(params.size());
}

int64_t spider::FiringHandler::getParamValue(size_t ix) {
#ifndef NDEBUG
    return params_.at(ix)->value(params_);
#else
    return params_[ix]->value(params_);
#endif
}

void spider::FiringHandler::setParamValue(size_t ix, int64_t value) {
#ifndef NDEBUG
    params_.at(ix)->setValue(value);
#else
    params_[ix]->setValue(value);
#endif
}

/* === Private method(s) implementation === */
