/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
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
/* === Includes === */

#include <graphs-tools/transformation/srdag/Transformation.h>
#include <api/pisdf-api.h>

/* === Static function(s) === */

template<class InterfaceVector, class EdgeReconnector>
static ufast32 reconnectInterface(const InterfaceVector &interfaceVector, const EdgeReconnector &reconnector) {
    ufast32 ix = 0;
    for (auto &interface : interfaceVector) {
        const auto &vertex = interface->opposite();
        if (vertex->subtype() != spider::pisdf::VertexType::CONFIG) {
            auto *edge = reconnector(interface.get(), interface->edge(), ix);
            interface->graph()->addEdge(edge);
            ix++;
        }
    }
    return ix;
}

/* === Methods implementation === */

void spider::srdag::separateRunGraphFromInit(pisdf::Graph *graph) {
    if (!graph->configVertexCount() || !graph->dynamic()) {
        return;
    }

    /* == Compute the input interface count for both graphs == */
    ufast32 cfg2OutputIfCount = 0;
    ufast32 cfg2RunIfCount = 0;
    ufast32 inputIf2CfgCount = 0;
    for (const auto &cfg : graph->configVertices()) {
        for (const auto &edge : cfg->inputEdgeVector()) {
            const auto &source = edge->source();
            if (source->subtype() != pisdf::VertexType::INPUT) {
                throwSpiderException("Config vertex can not have source of type other than interface.");
            }
            inputIf2CfgCount++;
        }
        for (const auto &edge : cfg->outputEdgeVector()) {
            auto isOutputIf = edge->sink()->subtype() == pisdf::VertexType::OUTPUT;
            cfg2OutputIfCount += (isOutputIf);
            cfg2RunIfCount += (!isOutputIf);
        }
    }
    const auto runInputIfCount = graph->inputEdgeCount() + cfg2RunIfCount - inputIf2CfgCount;
    const auto runOutputIfCount = graph->outputEdgeCount() - cfg2OutputIfCount;

    /* == Create the run subgraph == */
    auto *runGraph = api::createGraph("run", graph->vertexCount(), graph->edgeCount(), graph->paramCount(),
                                      runInputIfCount, runOutputIfCount);

    /* == Move the edges == */
    auto itEdge = graph->edges().begin();
    while (graph->edgeCount() != (inputIf2CfgCount + cfg2OutputIfCount)) {
        const auto *source = itEdge->get()->source();
        const auto *sink = itEdge->get()->sink();
        if (sink->subtype() == pisdf::VertexType::CONFIG ||
            (sink->subtype() == pisdf::VertexType::OUTPUT &&
             source->subtype() == pisdf::VertexType::CONFIG)) {
            itEdge++;
        } else {
            graph->moveEdge(itEdge->get(), runGraph);
            itEdge = graph->edges().begin();
        }
    }

    /* == Move the subgraphs == */
    while (!graph->subgraphs().empty()) {
        graph->moveVertex(graph->subgraphs()[0], runGraph);
    }

    /* == Move the vertices == */
    auto itVertex = graph->vertices().begin();
    while (graph->vertexCount() != graph->configVertexCount()) {
        if ((*itVertex)->subtype() == pisdf::VertexType::CONFIG) {
            itVertex++;
        } else {
            graph->moveVertex(itVertex->get(), runGraph);
            itVertex = graph->vertices().begin();
        }
    }

    /* == Add run graph == */
    graph->addVertex(runGraph);

    /* == Reconnect Edges from input interfaces == */
    auto inputRunIx = reconnectInterface(graph->inputInterfaceVector(),
                                         [&runGraph](pisdf::Interface *input, pisdf::Edge *edge, ufast32 ix) {
                                             auto expr = edge->sourceRateExpression();
                                             /* == Change source of original edge to run graph interface == */
                                             edge->setSource(runGraph->inputInterface(ix), 0u, expr);
                                             edge->source()->setName(input->name());
                                             /* == Create an edge with the original interface == */
                                             return make<pisdf::Edge, StackID::PISDF>(input, 0u, expr,
                                                                                      runGraph, ix, std::move(expr));
                                         });

    /* == Reconnect Edges from output interfaces == */
    reconnectInterface(graph->outputInterfaceVector(),
                       [&runGraph](pisdf::Interface *output, pisdf::Edge *edge, ufast32 ix) {
                           auto expr = edge->sinkRateExpression();
                           /* == Change sink of original edge to run graph interface == */
                           edge->setSink(runGraph->outputInterface(ix), 0u, expr);
                           edge->sink()->setName(output->name());
                           /* == Create an edge with the original interface == */
                           return make<pisdf::Edge, StackID::PISDF>(runGraph, ix, expr,
                                                                    output, 0u, std::move(expr));
                       });

    /* == Connect the output edges of config vertices == */
    for (auto &cfg : graph->configVertices()) {
        for (auto edge : cfg->outputEdgeVector()) {
            const auto &sink = edge->sink();
            if (sink->subtype() != pisdf::VertexType::OUTPUT) {
                const auto srcRate = edge->sourceRateValue(); /* = Config actors can not have dynamic rate = */
                const auto srcPortIx = edge->sourcePortIx();
                /* == Connect input interface to vertex in run graph == */
                auto *input = runGraph->inputInterface(inputRunIx);
                edge->setSource(input, 0, edge->sourceRateExpression());
                input->setName(cfg->name() + "::out:" + std::to_string(srcPortIx));
                /* == Connect cfg to run graph == */
                api::createEdge(cfg, edge->sourcePortIx(), srcRate, runGraph, inputRunIx, srcRate);
                inputRunIx += 1;
            }
        }
    }

    /* == Copy the params to the run graph == */
    auto paramsToRemove = factory::vector<std::shared_ptr<pisdf::Param>>(StackID::TRANSFO);
    for (auto &param : graph->params()) {
        api::createInheritedParam(runGraph, param->name(), param);
    }
}

std::pair<spider::srdag::JobStack, spider::srdag::JobStack>
spider::srdag::singleRateTransformation(TransfoJob &job, pisdf::Graph *srdag) {
    if (!srdag) {
        throwSpiderException("nullptr for single rate graph.");
    }
    auto *graph = job.reference_;
    if (!graph) {
        throwSpiderException("nullptr for reference graph.");
    }
    if (graph->configVertexCount() && (graph->subgraphCount() != 1) &&
        (graph->vertexCount() != graph->configVertexCount())) {
        separateRunGraphFromInit(graph);
    }
    SingleRateTransformer transformer{ job, srdag };
    return transformer.execute();
}

