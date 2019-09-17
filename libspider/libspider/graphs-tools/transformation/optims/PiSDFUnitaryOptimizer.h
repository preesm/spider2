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
#ifndef SPIDER2_PISDFUNITARYOPTIMIZER_H
#define SPIDER2_PISDFUNITARYOPTIMIZER_H

/* === Includes === */

#include <graphs-tools/transformation/optims/PiSDFOptimizer.h>

/* === Class definition === */

/**
 * @brief Optimize a PiSDFGraph by removing useless special actors.
 *        detail:    --> Fork --> : removes fork with 1 output edge
 *                   --> Join --> : removes join with 1 input edge
 *                   --> Tail --> : removes tail with 1 input edge if rate_in == rate_out
 *                   --> Head --> : removes head with 1 input edge if rate_in == rate_out
 *                   --> Upsample -->   : removes upsample  if rate_in == rate_out
 *                   --> Downsample --> : removes downsample if rate_in == rate_out
 *                   --> Duplicate -->  : removes duplicate with 1 input edge if rate_in == rate_out
 */
class PiSDFUnitaryOptimizer : public PiSDFOptimizer {
public:
    inline bool operator()(PiSDFGraph *graph) const override;
};

bool PiSDFUnitaryOptimizer::operator()(PiSDFGraph *graph) const {
    Spider::vector<PiSDFVertex *> verticesToOptimize;

    for (auto *vertex : graph->vertices()) {
        switch (vertex->type()) {
            case PiSDFVertexType::FORK:
                if (vertex->nEdgesOUT() == 1) {
                    auto *inputEdge = vertex->inputEdge(0);
                    auto *outputEdge = vertex->outputEdge(0);
                    if (inputEdge->sinkRate() != outputEdge->sourceRate()) {
                        throwSpiderException("Fork [%s] with 1 output edge should have the same input/output rates.",
                                             vertex->name().c_str());
                    }
                    inputEdge->connectSink(outputEdge);
                    graph->removeEdge(outputEdge);
                    verticesToOptimize.push_back(vertex);
                }
                break;
            case PiSDFVertexType::JOIN:
                if (vertex->nEdgesIN() == 1) {
                    auto *inputEdge = vertex->inputEdge(0);
                    auto *outputEdge = vertex->outputEdge(0);
                    if (inputEdge->sinkRate() != outputEdge->sourceRate()) {
                        throwSpiderException("Join [%s] with 1 input edge should have the same input/output rates.",
                                             vertex->name().c_str());
                    }
                    outputEdge->connectSource(inputEdge);
                    graph->removeEdge(inputEdge);
                    verticesToOptimize.push_back(vertex);
                }
                break;
            case PiSDFVertexType::HEAD:
            case PiSDFVertexType::TAIL:
                if (vertex->nEdgesIN() == 1) {
                    auto *inputEdge = vertex->inputEdge(0);
                    auto *outputEdge = vertex->outputEdge(0);
                    if (inputEdge->sinkRate() == outputEdge->sourceRate()) {
                        outputEdge->connectSource(inputEdge);
                        graph->removeEdge(inputEdge);
                        verticesToOptimize.push_back(vertex);
                    }
                }
                break;
            case PiSDFVertexType::DUPLICATE:
                if (vertex->nEdgesOUT() == 1) {
                    auto *inputEdge = vertex->inputEdge(0);
                    auto *outputEdge = vertex->outputEdge(0);
                    if (inputEdge->sinkRate() == outputEdge->sourceRate()) {
                        inputEdge->connectSink(outputEdge);
                        graph->removeEdge(outputEdge);
                        verticesToOptimize.push_back(vertex);
                    }
                }
                break;
            case PiSDFVertexType::UPSAMPLE:
            case PiSDFVertexType::DOWNSAMPLE: {
                auto *inputEdge = vertex->inputEdge(0);
                auto *outputEdge = vertex->outputEdge(0);
                if (inputEdge->sinkRate() == outputEdge->sourceRate()) {
                    inputEdge->connectSink(outputEdge);
                    graph->removeEdge(outputEdge);
                    verticesToOptimize.push_back(vertex);
                }
            }
                break;
            default:
                break;
        }
    }

    /* == Remove the vertices from the graph == */
    for (auto &vertex : verticesToOptimize) {
        graph->removeVertex(vertex);
    }

    return verticesToOptimize.empty();
}

#endif //SPIDER2_PISDFUNITARYOPTIMIZER_H