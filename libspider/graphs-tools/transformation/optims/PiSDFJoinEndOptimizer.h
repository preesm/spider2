/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
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
#ifndef SPIDER2_PISDFJOINENDOPTIMIZER_H
#define SPIDER2_PISDFJOINENDOPTIMIZER_H

/* === Includes === */

#include <graphs-tools/transformation/optims/PiSDFOptimizer.h>
#include <api/pisdf-api.h>

/* === Class definition === */

class PiSDFJoinEndOptimizer final : public PiSDFOptimizer {
public:
    inline bool operator()(PiSDFGraph *graph) const override;
};

bool PiSDFJoinEndOptimizer::operator()(PiSDFGraph *graph) const {
    auto verticesToOptimize = spider::containers::vector<PiSDFAbstractVertex *>(StackID::TRANSFO);

    /* == Retrieve the vertices to remove == */
    for (auto *vertex : graph->vertices()) {
        if (vertex->subtype() == PiSDFVertexType::JOIN) {
            auto *sink = vertex->outputEdge(0)->sink();
            if (sink->subtype() == PiSDFVertexType::END) {
                verticesToOptimize.push_back(vertex);
            }
        }
    }

    /* == Remove useless init / end connections == */
    const auto &params = graph->params();
    for (auto *join : verticesToOptimize) {
        auto *edge = join->outputEdge(0);
        auto *end = edge->sink();
        graph->removeEdge(edge);
        // TODO: see how to deal with persistent delay memory allocation
        for (auto *inputEdge : join->inputEdgeArray()) {
            auto rate = inputEdge->sinkRateExpression().evaluate(params);
            auto *newEnd = spider::api::createEnd(graph, "end-" + inputEdge->source()->name(), StackID::TRANSFO);
            inputEdge->setSink(newEnd, 0, spider::Expression(rate));
        }

        if (spider::api::verbose() && log_enabled<LOG_OPTIMS>()) {
            spider::log::verbose<LOG_OPTIMS>("JoinEndOptimizer: removing join [%s] and end [%s] vertices.\n",
                                             join->name().c_str(), end->name().c_str());
        }
        graph->removeVertex(join);
        graph->removeVertex(end);
    }
    return verticesToOptimize.empty();
}

#endif //SPIDER2_PISDFJOINENDOPTIMIZER_H