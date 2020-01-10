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

/* === Includes === */

#include <graphs-tools/transformation/srdag/Transformation.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <api/pisdf-api.h>

/* === Methods implementation === */

bool spider::srdag::splitDynamicGraph(pisdf::Graph *subgraph) {
    if (!subgraph->dynamic() || !subgraph->configVertexCount()) {
        return false;
    }

    /* == Compute the input interface count for both graphs == */
    uint32_t initInputIFCount = 0;
    uint32_t initOutputIFCount = 0;
    uint32_t cfgInputIFCount = 0;
    for (const auto &cfg : subgraph->configVertices()) {
        for (const auto &edge : cfg->inputEdgeVector()) {
            const auto &source = edge->source();
            if (source->subtype() != pisdf::VertexType::INPUT) {
                throwSpiderException("Config vertex can not have source of type other than interface.");
            }
            initInputIFCount += 1;
        }
        for (const auto &edge : cfg->outputEdgeVector()) {
            const auto &sink = edge->sink();
            auto isOutputIF = sink->subtype() == pisdf::VertexType::OUTPUT;
            cfgInputIFCount += (!isOutputIF);
            initOutputIFCount += (isOutputIF);
        }
    }
    auto runInputIFCount = subgraph->inputEdgeCount() + cfgInputIFCount - initInputIFCount;
    const auto &runOutputIFCount = subgraph->outputEdgeCount() - initOutputIFCount;

    /* == Create the init subgraph == */
    auto *initGraph = api::createSubgraph(subgraph->graph(),
                                          "ginit-" + subgraph->name(),
                                          static_cast<uint32_t>(subgraph->configVertexCount()),
                                          initInputIFCount + initOutputIFCount + cfgInputIFCount,
                                          0,
                                          initInputIFCount,
                                          initOutputIFCount + cfgInputIFCount,
                                          static_cast<uint32_t>(subgraph->configVertexCount()));

    /* == Create the run subgraph == */
    auto *runGraph = api::createSubgraph(subgraph->graph(),
                                         "grun-" + subgraph->name(),
                                         static_cast<uint32_t>(subgraph->vertexCount()),
                                         static_cast<uint32_t>(subgraph->edgeCount()),
                                         static_cast<uint32_t>(subgraph->paramCount()),
                                         static_cast<uint32_t>(runInputIFCount),
                                         static_cast<uint32_t>(runOutputIFCount),
                                         0);

    /* == Set repetition values == */
    initGraph->setRepetitionValue(subgraph->repetitionValue());
    runGraph->setRepetitionValue(subgraph->repetitionValue());

    uint32_t inputInitIx = 0;
    uint32_t inputRunIx = 0;
    for (const auto &input : subgraph->inputInterfaceVector()) {
        const auto &sink = input->opposite();
        if (sink->subtype() == pisdf::VertexType::CONFIG) {
            /* == Reconnect and move inner edge in init graph == */
            auto *edge = input->outputEdge();
            edge->setSource(initGraph->inputInterface(inputInitIx), 0, Expression(edge->sourceRateExpression()));
            edge->source()->setName(input->name());
            subgraph->moveEdge(edge, initGraph);

            /* == Reconnect outside edge == */
            auto *inputEdge = input->inputEdge();
            inputEdge->setSink(initGraph, inputInitIx, Expression(inputEdge->sinkRateExpression()));
            inputInitIx += 1;
        } else {
            /* == Reconnect and move inner edge in run graph == */
            auto *edge = input->outputEdge();
            edge->setSource(runGraph->inputInterface(inputRunIx), 0, Expression(edge->sourceRateExpression()));
            edge->source()->setName(input->name());
            subgraph->moveEdge(edge, runGraph);

            /* == Reconnect outside edge == */
            auto *inputEdge = input->inputEdge();
            inputEdge->setSink(runGraph, inputRunIx, Expression(inputEdge->sinkRateExpression()));
            inputRunIx += 1;
        }
    }

    uint32_t outputInitIx = 0;
    uint32_t outputRunIx = 0;
    for (const auto &output : subgraph->outputInterfaceVector()) {
        const auto &source = output->opposite();
        if (source->subtype() == pisdf::VertexType::CONFIG) {
            /* == Reconnect and move inner edge in init graph == */
            auto *edge = output->inputEdge();
            edge->setSink(initGraph->outputInterface(outputInitIx), 0, Expression(edge->sinkRateExpression()));
            edge->sink()->setName(output->name());
            subgraph->moveEdge(edge, initGraph);

            /* == Reconnect outside edge == */
            auto *outputEdge = output->outputEdge();
            outputEdge->setSource(initGraph, outputInitIx, Expression(outputEdge->sourceRateExpression()));
            outputInitIx += 1;
        } else {
            /* == Reconnect and move inner edge in run graph == */
            auto *edge = output->inputEdge();
            edge->setSink(runGraph->outputInterface(outputRunIx), 0, Expression(edge->sinkRateExpression()));
            edge->sink()->setName(output->name());
            subgraph->moveEdge(edge, runGraph);

            /* == Reconnect outside edge == */
            auto *outputEdge = output->outputEdge();
            outputEdge->setSource(runGraph, outputRunIx, Expression(outputEdge->sourceRateExpression()));
            outputRunIx += 1;
        }
    }

    for (auto &cfg : subgraph->configVertices()) {
        subgraph->moveVertex(cfg, initGraph);
        for (auto edge : cfg->outputEdgeVector()) {
            const auto &sink = edge->sink();
            if (sink->subtype() != pisdf::VertexType::OUTPUT) {
                const auto &srcRate = edge->sourceRateExpression().evaluate(subgraph->params());
                const auto &srcPortIx = edge->sourcePortIx();
                const auto &name = cfg->name() + "_out-" + std::to_string(srcPortIx);

                /* == Connect input interface to vertex in run graph == */
                auto *input = runGraph->inputInterface(inputRunIx);
                edge->setSource(input, 0, Expression(edge->sourceRateExpression()));
                subgraph->moveEdge(edge, runGraph);

                /* == Connect cfg to output interface in init graph == */
                auto *output = initGraph->outputInterface(outputInitIx);
                api::createEdge(cfg, srcPortIx, srcRate, output, 0, srcRate);

                /* == Connect init graph to run graph == */
                api::createEdge(initGraph, outputInitIx, srcRate, runGraph, inputRunIx, srcRate);
                input->setName(name);
                output->setName(name);
                outputInitIx += 1;
                inputRunIx += 1;
            }
        }
    }

    /* == Move the params to the run graph (init job will use the one of the dyna) == */
    for (auto &param : subgraph->params()) {
        subgraph->moveParam(param, runGraph);
    }

    /* == Move the edges == */
    for (auto &edge : subgraph->edges()) {
        subgraph->moveEdge(edge, runGraph);
    }

    /* == Move the vertices == */
    for (auto &vertex : subgraph->vertices()) {
        subgraph->moveVertex(vertex, runGraph);
    }

    /* == Set the run reference in the init graph == */
    initGraph->setRunGraphReference(runGraph);

    return true;
}

std::pair<spider::srdag::JobStack, spider::srdag::JobStack>
spider::srdag::singleRateTransformation(const TransfoJob &job, pisdf::Graph *srdag) {
    if (!srdag) {
        throwSpiderException("nullptr for single rate graph.");
    }
    if (!job.reference_) {
        throwSpiderException("nullptr for reference graph.");
    }
    SingleRateTransformer transformer{ job, srdag };
    return transformer.execute();
}

