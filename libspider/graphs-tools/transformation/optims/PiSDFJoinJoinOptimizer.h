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
#ifndef SPIDER2_PISDFJOINJOINOPTIMIZER_H
#define SPIDER2_PISDFJOINJOINOPTIMIZER_H

/* === Includes === */

#include <graphs-tools/transformation/optims/PiSDFOptimizer.h>
#include <api/pisdf-api.h>

/* === Class definition === */

/**
 * @brief Optimize Join -> Join patterns in a spider::pisdf::Graph.
 * @see: https://tel.archives-ouvertes.fr/tel-01301642
 */
class PiSDFJoinJoinOptimizer final : public PiSDFOptimizer {
public:
    inline bool operator()(spider::pisdf::Graph *graph) const override;

private:
    inline spider::pisdf::ExecVertex *createNewJoin(spider::pisdf::Vertex *firstJoin,
                                                    spider::pisdf::Vertex *secondJoin) const {
        auto *graph = firstJoin->graph();
        const auto &inputCount = static_cast<uint32_t>(firstJoin->inputEdgeCount() +
                                                       (secondJoin->inputEdgeCount() - 1));
        auto *newJoin = spider::api::createJoin(graph,
                                                "merged-" + firstJoin->name() + "-" + secondJoin->name(),
                                                inputCount);

        /* == Connect the output of the second Join to the new Join == */
        auto *edge = secondJoin->outputEdge(0);
        edge->setSource(newJoin, 0, spider::Expression(edge->sourceRateExpression()));
        return newJoin;
    }
};

bool PiSDFJoinJoinOptimizer::operator()(spider::pisdf::Graph *graph) const {
    auto verticesToOptimize = spider::containers::vector<std::pair<spider::pisdf::Vertex *, spider::pisdf::Vertex *>>(
            StackID::TRANSFO);

    /* == Search for the pair of fork to optimize == */
    for (auto &vertex : graph->vertices()) {
        if (vertex->subtype() == spider::pisdf::VertexType::JOIN) {
            auto *sink = vertex->outputEdge(0)->sink();
            if (sink->subtype() == spider::pisdf::VertexType::JOIN) {
                verticesToOptimize.emplace_back(vertex, sink);
            }
        }
    }

    /* == Do the optimization == */
    for (auto it = verticesToOptimize.begin(); it != verticesToOptimize.end(); ++it) {
        auto &pair = (*it);
        auto *firstJoin = pair.first;
        auto *secondJoin = pair.second;

        /* == Create the new join == */
        auto *newJoin = createNewJoin(firstJoin, secondJoin);

        /* === Link the edges === */

        /* == Connect the output edges of the first Join into the new Join == */
        auto firstJoinEdgeIx = firstJoin->outputEdge(0)->sinkPortIx();
        for (size_t i = 0; i < firstJoinEdgeIx; ++i) {
            auto *edge = secondJoin->inputEdge(i);
            edge->setSink(newJoin, i, spider::Expression(edge->sinkRateExpression()));
        }

        /* == Remove the edge between the two Joins == */
        graph->removeEdge(secondJoin->inputEdge(firstJoinEdgeIx));

        /* == Connect the input edges of the first join into the new join == */
        for (size_t i = 0; i < firstJoin->inputEdgeCount(); ++i) {
            auto *edge = firstJoin->inputEdge(i);
            const auto &ix = edge->sinkPortIx() + firstJoinEdgeIx;
            edge->setSink(newJoin, ix, spider::Expression(edge->sinkRateExpression()));
        }

        /* == Connect the remaining output edges of the first Fork into the new Fork == */
        const auto &offset = firstJoin->inputEdgeCount() - 1;
        for (size_t i = firstJoinEdgeIx + 1; i < secondJoin->inputEdgeCount(); ++i) {
            auto *edge = secondJoin->inputEdge(i);
            const auto &ix = edge->sinkPortIx() + offset;
            edge->setSink(newJoin, ix, spider::Expression(edge->sinkRateExpression()));
        }

        /* == Search for the pair to modify (if any) == */
        for (auto it2 = std::next(it); it2 != std::end(verticesToOptimize); ++it2) {
            auto &secPair = (*it2);
            if (secPair.first == firstJoin || secPair.first == secondJoin) {
                secPair.first = newJoin;
            }
            if (secPair.second == firstJoin || secPair.second == secondJoin) {
                secPair.second = newJoin;
            }
        }

        /* == Remove the vertices == */
        if (spider::api::verbose() && spider::log::enabled<spider::log::Type::OPTIMS>()) {
            spider::log::verbose<spider::log::Type::OPTIMS>("JoinJoinOptimizer: removing [%s] and [%s] join vertices.\n",
                                             firstJoin->name().c_str(), secondJoin->name().c_str());
        }
        graph->removeVertex(firstJoin);
        graph->removeVertex(secondJoin);
    }
    return verticesToOptimize.empty();
}

#endif //SPIDER2_PISDFJOINJOINOPTIMIZER_H
