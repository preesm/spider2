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

#include <cinttypes>
#include <graphs-tools/transformation/srdag/Transformation.h>
#include <graphs-tools/transformation/srdag/Helper.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/interfaces/Interface.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs-tools/brv/LCMBRVCompute.h>
#include <graphs-tools/numerical/PiSDFAnalysis.h>
#include <graphs-tools/expression-parser/Expression.h>

/* === Static function(s) === */

static spider::srdag::TransfoStack buildSourceLinkerVector(spider::srdag::TransfoJob &linker) {
    const auto &edge = linker.edge_;
    const auto &source = edge->source();
    const auto &delay = edge->delay();
    spider::srdag::TransfoStack sourceVector;
    sourceVector.reserve(source->repetitionValue() + (delay ? delay->setter()->repetitionValue() : 0));

    /* == Populate first the source clones in reverse order == */
    const auto &params = linker.job_.params_;
    const auto &rate = source->subtype() == PiSDFVertexType::INPUT ? edge->sinkRateExpression().evaluate(params) *
                                                                     edge->sink()->repetitionValue()
                                                                   : edge->sourceRateExpression().evaluate(params);
    fillLinkerVector(sourceVector, source, rate, edge->sourcePortIx(), linker);

    /* == If delay, populate the setter clones in reverse order == */
    if (delay) {
        const auto &setterEdge = delay->vertex()->inputEdge(0);
        const auto &setter = delay->setter();
        const auto &setterRate = setterEdge->sourceRateExpression().evaluate(params);
        fillLinkerVector(sourceVector, setter, setterRate, setterEdge->sourcePortIx(), linker);
    }
    return sourceVector;
}

static spider::srdag::TransfoStack buildSinkLinkerVector(spider::srdag::TransfoJob &linker) {
    const auto &edge = linker.edge_;
    const auto &sink = edge->sink();
    const auto &delay = edge->delay();

    spider::srdag::TransfoStack sinkVector;
    sinkVector.reserve(sink->repetitionValue() + (delay ? delay->getter()->repetitionValue() : 0));

    /* == First, if delay, populate the getter clones in reverse order == */
    const auto &params = linker.job_.params_;
    if (delay) {
        if ((sink == edge->source()) && delay->value(params) < edge->sinkRateExpression().evaluate(params)) {
            throwSpiderException("Insufficient delay [%"
                                         PRIu32
                                         "] on edge [%s].", delay->value(params), edge->name().c_str());
        }
        const auto &getterEdge = delay->vertex()->outputEdge(0);
        const auto &getter = delay->getter();
        const auto &getterRate = getterEdge->sinkRateExpression().evaluate(params);
        fillLinkerVector(sinkVector, getter, getterRate, getterEdge->sinkPortIx(), linker);
    }

    /* == Populate the sink clones in reverse order == */
    const auto &rate = sink->subtype() == PiSDFVertexType::OUTPUT ? edge->sourceRateExpression().evaluate(params) *
                                                                    edge->source()->repetitionValue()
                                                                  : edge->sinkRateExpression().evaluate(params);
    fillLinkerVector(sinkVector, sink, rate, edge->sinkPortIx(), linker);
    return sinkVector;
}

/* === Methods implementation === */

std::pair<PiSDFGraph *, PiSDFGraph *> spider::srdag::splitDynamicGraph(PiSDFGraph *subgraph) {
    if (!subgraph->dynamic()) {
        return std::make_pair(nullptr, nullptr);
    }

    /* == Compute the input interface count for both graphs == */
    std::uint32_t initInputIFCount = 0;
    std::uint32_t initOutputIFCount = 0;
    std::uint32_t cfgInputIFCount = 0;
    for (const auto &cfg : subgraph->configVertices()) {
        for (const auto &edge : cfg->inputEdgeArray()) {
            const auto &source = edge->source();
            if (source->subtype() != pisdf::VertexType::INPUT) {
                throwSpiderException("Config vertex can not have source of type other than interface.");
            }
            initInputIFCount += 1;
        }
        for (const auto &edge : cfg->outputEdgeArray()) {
            const auto &sink = edge->sink();
            cfgInputIFCount += (sink->subtype() != pisdf::VertexType::OUTPUT);
            initOutputIFCount += (sink->subtype() == pisdf::VertexType::OUTPUT);
        }
    }
    const auto &runInputIFCount = subgraph->inputEdgeCount() + cfgInputIFCount - initInputIFCount;
    const auto &runOutputIFCount = subgraph->outputEdgeCount() - initOutputIFCount;

    /* == Create the init subgraph == */
    auto *initGraph = spider::api::createSubraph(subgraph->containingGraph(),
                                                 "ginit-" + subgraph->name(),
                                                 subgraph->configVertexCount(),
                                                 initInputIFCount + initOutputIFCount + cfgInputIFCount,
                                                 0,
                                                 initInputIFCount,
                                                 initOutputIFCount + cfgInputIFCount,
                                                 subgraph->configVertexCount(), StackID::PISDF);

    /* == Create the run subgraph == */
    auto *runGraph = spider::api::createSubraph(subgraph->containingGraph(),
                                                "grun-" + subgraph->name(),
                                                subgraph->vertexCount(),
                                                subgraph->edgeCount(),
                                                subgraph->paramCount(),
                                                runInputIFCount,
                                                runOutputIFCount,
                                                0, StackID::PISDF);

    std::uint32_t inputInitIx = 0;
    std::uint32_t inputRunIx = 0;
    for (const auto &input : subgraph->inputInterfaceArray()) {
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

    std::uint32_t outputInitIx = 0;
    std::uint32_t outputRunIx = 0;
    for (const auto &output : subgraph->outputInterfaceArray()) {
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
        for (auto edge : cfg->outputEdgeArray()) {
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
                spider::api::createEdge(cfg, srcPortIx, srcRate, output, 0, srcRate,
                                        StackID::PISDF);

                /* == Connect init graph to run graph == */
                spider::api::createEdge(initGraph, outputInitIx, srcRate, runGraph, inputRunIx, srcRate,
                                        StackID::PISDF);
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

    /* == Destroy the subgraph == */
    subgraph->containingGraph()->removeVertex(subgraph);

    return std::make_pair(initGraph, runGraph);
}

std::pair<spider::srdag::JobStack, spider::srdag::JobStack>
spider::srdag::staticSingleRateTransformation(const spider::srdag::Job &job, PiSDFGraph *srdag) {
    if (!srdag) {
        throwSpiderException("nullptr for single rate graph.");
    }
    if (!job.reference_) {
        throwSpiderException("nullptr for job.reference graph.");
    }

    /* == Split subgraphs if needed == */
    TransfoTracker init2dynamic_;
    const auto &subgraphCount = job.reference_->subgraphCount();
    init2dynamic_.resize(subgraphCount, UINT32_MAX);
    auto it = job.reference_->subgraphs().begin();
    std::uint64_t i = 0;
    while (i < subgraphCount) {
        auto &subgraph = (*it);
        auto &&result = splitDynamicGraph(subgraph);
        if (result.first) {
            init2dynamic_[result.first->subIx()] = result.second->subIx();
        } else {
            it++;
        }
        i++;
    }

    /* == Compute the repetition values of the graph (if dynamic and/or first instance) == */
    if (job.reference_->dynamic() || job.instanceValue_ == 0 || job.instanceValue_ == UINT32_MAX) {
        LCMBRVCompute brvTask{ job.reference_, job.params_ };
        brvTask.execute();
    }

    TransfoTracker vertexTransfoTracker;
    vertexTransfoTracker.resize(job.reference_->vertexCount() +
                                        job.reference_->inputEdgeCount() +
                                        job.reference_->outputEdgeCount(), UINT32_MAX);
    JobStack nextJobs;
    JobStack dynaJobs;

    /* == Replace the interfaces of the graph and remove the vertex == */
    auto &&linker = TransfoJob{ job, nullptr, srdag, nextJobs, dynaJobs, vertexTransfoTracker, init2dynamic_ };
    replaceJobInterfaces(linker);

    /* == Clone the vertices == */
    linker.edge_ = nullptr;
    for (const auto &vertex : job.reference_->vertices()) {
        spider::srdag::fetchOrClone(vertex, linker);
    }

    /* == Do the linkage for every edges of the graph == */
    for (const auto &edge : job.reference_->edges()) {
        linker.edge_ = edge;
        staticEdgeSingleRateLinkage(linker);
    }

    /* == Remove the vertex from the srdag == */
    if (job.instanceValue_ != UINT32_MAX) {
        auto *srdagInstance = linker.srdag_->vertex(linker.job_.srdagIx_);
        linker.srdag_->removeVertex(srdagInstance);
    }
    return std::make_pair(std::move(nextJobs), std::move(dynaJobs));
}

void spider::srdag::staticEdgeSingleRateLinkage(TransfoJob &transfoJob) {
    const auto &edge = transfoJob.edge_;
    if ((edge->source()->subtype() == PiSDFVertexType::DELAY) ||
        (edge->sink()->subtype() == PiSDFVertexType::DELAY)) {
        return;
    }
    if ((edge->source() == edge->sink())) {
        if (!edge->delay()) {
            throwSpiderException("No delay on edge with self loop.");
        }
    }

    auto sourceVector = buildSourceLinkerVector(transfoJob);
    auto sinkVector = buildSinkLinkerVector(transfoJob);

    /* == Compute the different dependencies of sinks over sources == */
    spider::srdag::computeEdgeDependencies(sourceVector, sinkVector, transfoJob);

    /* == Iterate over sinks == */
    while (!sinkVector.empty()) {
        const auto &snkLnk = sinkVector.back();
        const auto &srcLnk = sourceVector.back();
        if (snkLnk.lowerDep_ == snkLnk.upperDep_) {
            if (srcLnk.lowerDep_ == srcLnk.upperDep_) {
                /* == Forward link between source and sink == */
                spider::api::createEdge(srcLnk.vertex_, srcLnk.portIx_, srcLnk.rate_,
                                        snkLnk.vertex_, snkLnk.portIx_, snkLnk.rate_, StackID::TRANSFO);
                sourceVector.pop_back();
                sinkVector.pop_back();
            } else {
                /* == Source need a fork == */
                spider::srdag::addForkVertex(sourceVector, sinkVector, transfoJob.srdag_);
            }
        } else {
            /* == Sink need a join == */
            spider::srdag::addJoinVertex(sourceVector, sinkVector, transfoJob.srdag_);
        }
    }

    /* == Sanity check == */
    if (!sourceVector.empty()) {
        throwSpiderException("remaining sources to link after single rate transformation on edge: [%s].",
                             edge->name().c_str());
    }
}
