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
#ifndef SPIDER2_PISDFFORKFORKOPTIMIZER_H
#define SPIDER2_PISDFFORKFORKOPTIMIZER_H

/* === Includes === */

#include <graphs-tools/transformation/optims/PiSDFOptimizer.h>
#include <api/pisdf-api.h>
#include <api/config-api.h>

/* === Class definition === */

/**
 * @brief Optimize Fork -> Fork patterns in a spider::pisdf::Graph.
 * @see: https://tel.archives-ouvertes.fr/tel-01301642
 */
class PiSDFForkForkOptimizer final : public PiSDFOptimizer {
public:
    inline bool operator()(spider::pisdf::Graph *graph) const override;

private:
    inline spider::pisdf::ExecVertex *createNewFork(spider::pisdf::Vertex *firstFork,
                                                    spider::pisdf::Vertex *secondFork) const {
        auto *graph = firstFork->graph();
        const auto &outputCount = static_cast<uint32_t>((firstFork->outputEdgeCount() - 1) +
                                                        secondFork->outputEdgeCount());
        auto *newFork = spider::api::createFork(graph,
                                                "merged-" + firstFork->name() + "-" + secondFork->name(),
                                                outputCount,
                                                StackID::TRANSFO);

        /* == Connect the input of the first Fork to the new Fork == */
        auto *edge = firstFork->inputEdge(0);
        edge->setSink(newFork, 0, spider::Expression(edge->sinkRateExpression()));
        return newFork;
    }
};

bool PiSDFForkForkOptimizer::operator()(spider::pisdf::Graph *graph) const {
    auto verticesToOptimize = spider::containers::vector<std::pair<spider::pisdf::Vertex *, spider::pisdf::Vertex *>>(
            StackID::TRANSFO);

    /* == Search for the pair of fork to optimize == */
    for (const auto &vertex : graph->vertices()) {
        if (vertex->subtype() == spider::pisdf::VertexType::FORK) {
            auto *source = vertex->inputEdge(0)->source();
            if (source->subtype() == spider::pisdf::VertexType::FORK) {
                verticesToOptimize.emplace_back(source, vertex);
            }
        }
    }

    /* == Do the optimization == */
    for (auto it = verticesToOptimize.begin(); it != verticesToOptimize.end(); ++it) {
        auto &pair = (*it);
        auto *firstFork = pair.first;
        auto *secondFork = pair.second;

        /* == Create the new fork == */
        auto *newFork = createNewFork(firstFork, secondFork);

        /* === Link the edges === */

        /* == Connect the output edges of the first Fork into the new Fork == */
        auto secondForkEdgeIx = secondFork->inputEdge(0)->sourcePortIx();
        for (size_t i = 0; i < secondForkEdgeIx; ++i) {
            auto *edge = firstFork->outputEdge(i);
            edge->setSource(newFork, i, spider::Expression(edge->sourceRateExpression()));
        }

        /* == Remove the edge between the two Forks == */
        graph->removeEdge(firstFork->outputEdge(secondForkEdgeIx));

        /* == Connect the output edges of the second fork into the new fork == */
        for (size_t i = 0; i < secondFork->outputEdgeCount(); ++i) {
            auto *edge = secondFork->outputEdge(i);
            const auto &ix = edge->sourcePortIx() + secondForkEdgeIx;
            edge->setSource(newFork, ix, spider::Expression(edge->sourceRateExpression()));
        }

        /* == Connect the remaining output edges of the first Fork into the new Fork == */
        const auto &offset = secondFork->outputEdgeCount() - 1;
        for (size_t i = secondForkEdgeIx + 1; i < firstFork->outputEdgeCount(); ++i) {
            auto *edge = firstFork->outputEdge(i);
            const auto &ix = edge->sourcePortIx() + offset;
            edge->setSource(newFork, ix, spider::Expression(edge->sourceRateExpression()));
        }

        /* == Search for the pair to modify (if any) == */
        for (auto it2 = std::next(it); it2 != std::end(verticesToOptimize); ++it2) {
            auto &secPair = (*it2);
            if (secPair.first == firstFork || secPair.first == secondFork) {
                secPair.first = newFork;
            }
            if (secPair.second == firstFork || secPair.second == secondFork) {
                secPair.second = newFork;
            }
        }

        /* == Remove the vertices == */
        if (spider::api::verbose() && log_enabled<LOG_OPTIMS>()) {
            spider::log::verbose<LOG_OPTIMS>("ForkForkOptimizer: removing [%s] and [%s] fork vertices.\n",
                                             secondFork->name().c_str(), firstFork->name().c_str());
        }
        graph->removeVertex(secondFork);
        graph->removeVertex(firstFork);
    }

    return verticesToOptimize.empty();
}

#endif //SPIDER2_PISDFFORKFORKOPTIMIZER_H
