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
#include <graphs-tools/transformation/SRDAGTransformation.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs/pisdf/params/InHeritedParam.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/Interface.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs-tools/brv/LCMBRVCompute.h>
#include <graphs-tools/numerical/PiSDFAnalysis.h>
#include <graphs-tools/expression-parser/Expression.h>

/* === Static function(s) === */

static std::string
buildCloneName(const PiSDFAbstractVertex *vertex, std::uint32_t instance, Spider::SRDAG::JobLinker &linker) {
    const auto *graphRef = linker.job_.instanceValue_ == UINT32_MAX ? linker.job_.reference_ :
                           linker.srdag_->vertex(linker.job_.srdagIx_);
    return graphRef->name() + "-" + vertex->name() + "_" + std::to_string(instance);
}

static void cloneParams(Spider::SRDAG::Job &job, const PiSDFGraph *graph, const Spider::SRDAG::Job &parentJob) {
    for (const auto &param : graph->params()) {
        if (param->type() == Spider::PiSDF::ParamType::INHERITED) {
            const auto &inheritedParamIx = dynamic_cast<const PiSDFInHeritedParam *>(param->self())->parent()->ix();
            const auto &inheritedParam = parentJob.params_[inheritedParamIx];
            auto *p = Spider::API::createStaticParam(nullptr,
                                                     param->name(),
                                                     inheritedParam->value(),
                                                     StackID::TRANSFO);
            job.params_.push_back(p);
        } else if (!param->dynamic()) {
            job.params_.push_back(param);
        } else {
            auto *p = Spider::API::createDynamicParam(nullptr, param->name(), "", StackID::TRANSFO);
            job.params_.push_back(p);
        }
    }
}

static inline std::uint32_t cloneVertex(const PiSDFAbstractVertex *vertex, Spider::SRDAG::JobLinker &linker) {
    std::uint32_t ix = 0;
    for (std::uint32_t it = 0; it < vertex->repetitionValue(); ++it) {
        auto *clone = vertex->clone(StackID::TRANSFO, linker.srdag_);
        clone->setName(buildCloneName(vertex, it, linker));
        ix = clone->ix();
    }
    return ix - (vertex->repetitionValue() - 1);
}

static inline std::uint32_t cloneGraph(const PiSDFGraph *graph, Spider::SRDAG::JobLinker &linker) {
    std::uint32_t ix = 0;
    /* == Split the graph == */

    /* == Clone the vertex == */
    for (std::uint32_t it = 0; it < graph->repetitionValue(); ++it) {
        const auto *clone = Spider::API::createVertex(linker.srdag_,
                                                      buildCloneName(graph, it, linker),
                                                      graph->edgesINCount(),
                                                      graph->edgesOUTCount(),
                                                      StackID::TRANSFO);
        ix = clone->ix();
    }
    ix = ix - (graph->repetitionValue() - 1);

    /* == Push the jobs == */
    const auto &runGraphSubIx = linker.dynamic2init_[graph->subIx()];
    if (runGraphSubIx != UINT32_MAX) {
        auto *runGraph = graph->containingGraph()->subgraphs()[runGraphSubIx];

        /* == Find the first job corresponding to the init graph == */
        auto it = linker.dynaJobs_.begin();
        while ((it != linker.dynaJobs_.end()) && ((*it).reference_ != runGraph)) { it++; }
        if (it == linker.dynaJobs_.end()) {
            const auto &offset = linker.dynaJobs_.size();
            /* == Seems like run counter part of the graph has not been cloned yet == */
            cloneGraph(runGraph, linker);
            it = linker.dynaJobs_.begin() + offset;
            if (it == linker.dynaJobs_.end()) {
                throwSpiderException("Init graph [%s] did not find run counter part [%s].",
                                     graph->name().c_str(),
                                     runGraph->name().c_str());
            }
        }

        /* == Push the jobs == */
        for (auto srdagIx = ix; srdagIx < ix + graph->repetitionValue(); ++srdagIx) {
            linker.nextJobs_.emplace_back(graph, linker.srdag_->vertex(srdagIx)->ix(), srdagIx - ix);
            linker.nextJobs_.back().params_.reserve(runGraph->paramCount());

            /* == Copy the params pointer == */
            for (auto &param : (*it).params_) {
                linker.nextJobs_.back().params_.emplace_back(param);
            }
            it++;
        }
    } else {
        auto &jobStack = graph->dynamic() ? linker.dynaJobs_ : linker.nextJobs_;
        for (auto srdagIx = ix; srdagIx < ix + graph->repetitionValue(); ++srdagIx) {
            jobStack.emplace_back(graph, linker.srdag_->vertex(srdagIx)->ix(), srdagIx - ix);

            /* == Copy the params == */
            cloneParams(jobStack.back(), graph, linker.job_);
        }
    }
    return ix;
}

static inline std::uint32_t uniformIx(const PiSDFAbstractVertex *vertex, const PiSDFGraph *graph) {
    return vertex->ix() +
           ((vertex->type() == PiSDFVertexType::INTERFACE) * graph->vertexCount()) +
           ((vertex->subtype() == PiSDFVertexType::OUTPUT) * graph->edgesINCount());
}

static PiSDFAbstractVertex *fetchOrClone(const PiSDFAbstractVertex *vertex, Spider::SRDAG::JobLinker &linker) {
    if (!vertex) {
        throwSpiderException("Trying to clone nullptr vertex.");
    }
    const auto &vertexUniformIx = uniformIx(vertex, linker.job_.reference_);

    /* == If vertex has already been cloned return the first one == */
    if (linker.tracker_[vertexUniformIx] == UINT32_MAX) {
        if (vertex->subtype() == PiSDFVertexType::GRAPH) {
            /* == Clone the graph N times and create the different jobs == */
            linker.tracker_[vertexUniformIx] = cloneGraph(dynamic_cast<const PiSDFGraph *>(vertex), linker);
        } else {
            /* == Clone the vertex N times and return the first one == */
            linker.tracker_[vertexUniformIx] = cloneVertex(vertex, linker);
        }
    }
    return linker.srdag_->vertex(linker.tracker_[vertexUniformIx]);
}

static void pushReverseVertexLinkerVector(Spider::SRDAG::LinkerVector &vector,
                                          const PiSDFAbstractVertex *reference,
                                          const std::int64_t &rate,
                                          const std::uint32_t portIx,
                                          Spider::SRDAG::JobLinker &linker) {
    const auto &cloneIx = fetchOrClone(reference, linker)->ix();
    for (auto i = (cloneIx + reference->repetitionValue()); i != cloneIx; --i) {
        vector.emplace_back(rate, portIx, linker.srdag_->vertex(i - 1));
    }
}

static Spider::SRDAG::LinkerVector buildSourceLinkerVector(Spider::SRDAG::JobLinker &linker) {
    const auto &edge = linker.edge_;
    const auto &source = edge->source();
    const auto &delay = edge->delay();
    Spider::SRDAG::LinkerVector sourceVector;
    sourceVector.reserve(source->repetitionValue() + (delay ? delay->setter()->repetitionValue() : 0));

    /* == Populate first the source clones in reverse order == */
    const auto &params = linker.job_.params_;
    const auto &rate = source->type() == PiSDFVertexType::INTERFACE ? edge->sinkRateExpression().evaluate(params) *
                                                                      edge->sink()->repetitionValue()
                                                                    : edge->sourceRateExpression().evaluate(params);
    pushReverseVertexLinkerVector(sourceVector, source, rate, edge->sourcePortIx(), linker);

    /* == If delay, populate the setter clones in reverse order == */
    if (delay) {
        const auto &setterEdge = delay->vertex()->inputEdge(0);
        const auto &setter = delay->setter();
        const auto &setterRate = setterEdge->sourceRateExpression().evaluate(params);
        pushReverseVertexLinkerVector(sourceVector, setter, setterRate, setterEdge->sourcePortIx(), linker);
    }
    return sourceVector;
}

static Spider::SRDAG::LinkerVector buildSinkLinkerVector(Spider::SRDAG::JobLinker &linker) {
    const auto &edge = linker.edge_;
    const auto &sink = edge->sink();
    const auto &delay = edge->delay();

    Spider::SRDAG::LinkerVector sinkVector;
    sinkVector.reserve(sink->repetitionValue() + (delay ? delay->getter()->repetitionValue() : 0));

    /* == First, if delay, populate the getter clones in reverse order == */
    const auto &params = linker.job_.params_;
    if (delay) {
        if (delay->value(params) < edge->sinkRateExpression().evaluate(params)) {
            throwSpiderException("Insufficient delay [%"
                                         PRIu32
                                         "] on edge [%s].", delay->value(params), edge->name().c_str());
        }
        const auto &getterEdge = delay->vertex()->outputEdge(0);
        const auto &getter = delay->getter();
        const auto &getterRate = getterEdge->sinkRateExpression().evaluate(params);
        pushReverseVertexLinkerVector(sinkVector, getter, getterRate, getterEdge->sinkPortIx(), linker);
    }

    /* == Populate the sink clones in reverse order == */
    const auto &rate = sink->type() == PiSDFVertexType::INTERFACE ? edge->sourceRateExpression().evaluate(params) *
                                                                    edge->source()->repetitionValue()
                                                                  : edge->sinkRateExpression().evaluate(params);
    pushReverseVertexLinkerVector(sinkVector, sink, rate, edge->sinkPortIx(), linker);
    return sinkVector;
}

static void computeDependencies(Spider::SRDAG::LinkerVector &srcVector,
                                Spider::SRDAG::LinkerVector &snkVector,
                                Spider::SRDAG::JobLinker &linker) {
    const auto &edge = linker.edge_;
    auto &&delay = edge->delay() ? edge->delay()->value(linker.job_.params_) : 0;
    const auto &srcRate = srcVector[0].rate_;     /* = This should be the proper source rate of the edge = */
    const auto &snkRate = snkVector.back().rate_; /* = This should be the proper sink rate of the edge = */
    const auto &setterRate = edge->delay() ? srcVector.back().rate_ : 0;
    const auto &getterRate = edge->delay() ? snkVector[0].rate_ : 0;
    const auto &sinkRepetitionValue = edge->sink()->repetitionValue();
    const auto &setterOffset = edge->delay() ? edge->delay()->setter()->repetitionValue() : 0;

    /* == Compute dependencies for sinks == */
    std::uint32_t firing = 0;
    auto currentSinkRate = snkRate;
    for (auto it = snkVector.rbegin(); it < snkVector.rend(); ++it) {
        if (it == snkVector.rbegin() + sinkRepetitionValue) {
            /* == We've reached the end / getter vertices == */
            delay = delay - snkRate * sinkRepetitionValue;
            currentSinkRate = getterRate;
            firing = 0;
        }
        auto snkLowerDep = Spider::PiSDF::computeConsLowerDep(currentSinkRate, srcRate, firing, delay);
        auto snkUpperDep = Spider::PiSDF::computeConsUpperDep(currentSinkRate, srcRate, firing, delay);
        if (snkLowerDep < 0) {
            /* == Update dependencies for init / setter == */
            snkLowerDep -= Spider::PiSDF::computeConsLowerDep(snkRate, setterRate, firing, 0);
            if (snkUpperDep < 0) {
                snkUpperDep -= Spider::PiSDF::computeConsUpperDep(snkRate, setterRate, firing, 0);
            }
        }

        /* == Adjust the values to match the actual position in the source vector == */
        snkLowerDep += setterOffset;
        snkUpperDep += setterOffset;
        (*it).lowerDep_ = snkLowerDep;
        (*it).upperDep_ = snkUpperDep;
        firing += 1;
    }

    /* == Update source vector with proper dependencies == */
    firing = 0;
    for (auto it = snkVector.rbegin(); it < snkVector.rend(); ++it) {
        const auto &lowerIndex = srcVector.size() - 1 - (*it).lowerDep_;
        const auto &upperIndex = srcVector.size() - 1 - (*it).upperDep_;
        srcVector[lowerIndex].lowerDep_ = std::min(srcVector[lowerIndex].lowerDep_, firing);
        srcVector[lowerIndex].upperDep_ = std::max(srcVector[lowerIndex].upperDep_, firing);
        srcVector[upperIndex].lowerDep_ = std::min(srcVector[upperIndex].lowerDep_, firing);
        srcVector[upperIndex].upperDep_ = std::max(srcVector[upperIndex].upperDep_, firing);
        firing += 1;
    }
}

static void addForkVertex(Spider::SRDAG::LinkerVector &srcVector,
                          Spider::SRDAG::LinkerVector &snkVector,
                          PiSDFGraph *srdag) {
    const auto &sourceLinker = srcVector.back();
    auto *fork = Spider::API::createFork(srdag,
                                         "fork-" + sourceLinker.vertex_->name() + "_out-" +
                                         std::to_string(sourceLinker.portIx_),
                                         (sourceLinker.upperDep_ - sourceLinker.lowerDep_) + 1,
                                         StackID::TRANSFO);

    /* == Create an edge between source and fork == */
    Spider::API::createEdge(sourceLinker.vertex_, sourceLinker.portIx_, sourceLinker.rate_,
                            fork, 0, sourceLinker.rate_, StackID::TRANSFO);
    srcVector.pop_back();

    /* == Connect out of fork == */
    auto remaining = sourceLinker.rate_;
    for (std::uint32_t i = 0; i < fork->edgesOUTCount() - 1; ++i) {
        const auto &sinkLinker = snkVector.back();
        remaining -= sinkLinker.rate_;
        Spider::API::createEdge(fork, i, sinkLinker.rate_,
                                sinkLinker.vertex_, sinkLinker.portIx_, sinkLinker.rate_, StackID::TRANSFO);
        snkVector.pop_back();
    }
    srcVector.emplace_back(remaining, fork->edgesOUTCount() - 1, fork);
    srcVector.back().lowerDep_ = sourceLinker.upperDep_;
    srcVector.back().upperDep_ = sourceLinker.upperDep_;
}

static void addJoinVertex(Spider::SRDAG::LinkerVector &srcVector,
                          Spider::SRDAG::LinkerVector &snkVector,
                          PiSDFGraph *srdag) {
    const auto &sinkLinker = snkVector.back();
    auto *join = Spider::API::createJoin(srdag,
                                         "join-" + sinkLinker.vertex_->name() + "_in-" +
                                         std::to_string(sinkLinker.portIx_),
                                         (sinkLinker.upperDep_ - sinkLinker.lowerDep_) + 1,
                                         StackID::TRANSFO);

    /* == Create an edge between source and fork == */
    Spider::API::createEdge(join, 0, sinkLinker.rate_,
                            sinkLinker.vertex_, sinkLinker.portIx_, sinkLinker.rate_, StackID::TRANSFO);
    snkVector.pop_back();

    /* == Connect in of join == */
    auto remaining = sinkLinker.rate_;
    for (std::uint32_t i = 0; i < join->edgesINCount() - 1; ++i) {
        const auto &sourceLinker = srcVector.back();
        remaining -= sourceLinker.rate_;
        Spider::API::createEdge(sourceLinker.vertex_, sourceLinker.portIx_, sourceLinker.rate_,
                                join, i, sourceLinker.rate_, StackID::TRANSFO);
        srcVector.pop_back();
    }
    snkVector.emplace_back(remaining, join->edgesINCount() - 1, join);
    snkVector.back().lowerDep_ = sinkLinker.upperDep_;
    snkVector.back().upperDep_ = sinkLinker.upperDep_;
}

static void replaceJobInterfaces(Spider::SRDAG::JobLinker &linker) {
    if (linker.job_.instanceValue_ == UINT32_MAX) {
        return;
    }
    auto *srdagInstance = linker.srdag_->vertex(linker.job_.srdagIx_);
    if (!srdagInstance) {
        throwSpiderException("could not find matching single rate instance [%"
                                     PRIu32
                                     "] of graph [%s]", linker.job_.instanceValue_,
                             linker.job_.reference_->name().c_str());
    }

    /* == Replace the input interfaces == */
    for (const auto &interface : linker.job_.reference_->inputInterfaceArray()) {
        auto *edge = srdagInstance->inputEdge(interface->ix());
        auto *vertex = Spider::API::createUpsample(linker.srdag_,
                                                   srdagInstance->name() + "_" + interface->name(),
                                                   StackID::TRANSFO);
        edge->setSink(vertex, 0, Expression(edge->sinkRateExpression()));
        linker.tracker_[linker.job_.reference_->vertexCount() + interface->ix()] = vertex->ix();
    }

    /* == Replace the output interfaces == */
    for (const auto &interface : linker.job_.reference_->outputInterfaceArray()) {
        auto *edge = srdagInstance->outputEdge(interface->ix());
        auto *vertex = Spider::API::createTail(linker.srdag_,
                                               srdagInstance->name() + "_" + interface->name(),
                                               1,
                                               StackID::TRANSFO);
        edge->setSource(vertex, 0, Expression(edge->sourceRateExpression()));
        linker.tracker_[linker.job_.reference_->vertexCount() +
                        linker.job_.reference_->edgesINCount() +
                        interface->ix()] = vertex->ix();
    }
}

/* === Methods implementation === */

std::pair<PiSDFGraph *, PiSDFGraph *> Spider::SRDAG::splitDynamicGraph(PiSDFGraph *subgraph) {
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
            if (source->type() != PiSDF::VertexType::INTERFACE) {
                throwSpiderException("Config vertex can not have source of type other than interface.");
            }
            initInputIFCount += 1;
        }
        for (const auto &edge : cfg->outputEdgeArray()) {
            const auto &sink = edge->sink();
            cfgInputIFCount += (sink->type() != PiSDF::VertexType::INTERFACE);
            initOutputIFCount += (sink->type() == PiSDF::VertexType::INTERFACE);
        }
    }
    const auto &runInputIFCount = subgraph->edgesINCount() + cfgInputIFCount - initInputIFCount;
    const auto &runOutputIFCount = subgraph->edgesOUTCount() - initOutputIFCount;

    /* == Create the init subgraph == */
    auto *initGraph = Spider::API::createSubraph(subgraph->containingGraph(),
                                                 "ginit-" + subgraph->name(),
                                                 subgraph->configVertexCount(),
                                                 initInputIFCount + initOutputIFCount + cfgInputIFCount,
                                                 0,
                                                 initInputIFCount,
                                                 initOutputIFCount + cfgInputIFCount,
                                                 subgraph->configVertexCount(), StackID::PISDF);

    /* == Create the run subgraph == */
    auto *runGraph = Spider::API::createSubraph(subgraph->containingGraph(),
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
        if (sink->type() == PiSDF::VertexType::CONFIG) {
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
        if (source->type() == PiSDF::VertexType::CONFIG) {
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
            if (sink->type() != PiSDF::VertexType::INTERFACE) {
                const auto &srcRate = edge->sourceRateExpression().evaluate(subgraph->params());
                const auto &srcPortIx = edge->sourcePortIx();
                const auto &name = cfg->name() + "_out-" + std::to_string(srcPortIx);

                /* == Connect input interface to vertex in run graph == */
                auto *input = runGraph->inputInterface(inputRunIx);
                edge->setSource(input, 0, Expression(edge->sourceRateExpression()));
                subgraph->moveEdge(edge, runGraph);

                /* == Connect cfg to output interface in init graph == */
                auto *output = initGraph->outputInterface(outputInitIx);
                Spider::API::createEdge(cfg, srcPortIx, srcRate, output, 0, srcRate,
                                        StackID::PISDF);

                /* == Connect init graph to run graph == */
                Spider::API::createEdge(initGraph, outputInitIx, srcRate, runGraph, inputRunIx, srcRate,
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
    subgraph->containingGraph()->removeSubgraph(subgraph);

    return std::make_pair(initGraph, runGraph);
}

std::pair<Spider::SRDAG::JobStack, Spider::SRDAG::JobStack>
Spider::SRDAG::staticSingleRateTransformation(const Spider::SRDAG::Job &job, PiSDFGraph *srdag) {
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
    vertexTransfoTracker.reserve(job.reference_->vertexCount() +
                                 job.reference_->edgesINCount() +
                                 job.reference_->edgesOUTCount());
    for (size_t i = 0; i < vertexTransfoTracker.capacity(); ++i) {
        vertexTransfoTracker.push_back(UINT32_MAX);
    }
    JobStack nextJobs;
    JobStack dynaJobs;
    auto &&linker = JobLinker{ job, nullptr, srdag, nextJobs, dynaJobs, vertexTransfoTracker, init2dynamic_ };

    /* == Replace the interfaces of the graph and remove the vertex == */
    replaceJobInterfaces(linker);

    /* == Clone the vertices == */
    linker.edge_ = nullptr;
    for (const auto &vertex : job.reference_->vertices()) {
        if (vertex->type() != PiSDFVertexType::DELAY) {
            fetchOrClone(vertex, linker);
        }
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

void Spider::SRDAG::staticEdgeSingleRateLinkage(JobLinker &linker) {
    const auto &edge = linker.edge_;
    if ((edge->source()->type() == PiSDFVertexType::DELAY) ||
        (edge->sink()->type() == PiSDFVertexType::DELAY)) {
        return;
    }
    if ((edge->source() == edge->sink())) {
        if (!edge->delay()) {
            throwSpiderException("No delay on edge with self loop.");
        }
    }

    auto sourceVector = buildSourceLinkerVector(linker);
    auto sinkVector = buildSinkLinkerVector(linker);

    /* == Compute the different dependencies of sinks over sources == */
    computeDependencies(sourceVector, sinkVector, linker);

    /* == Iterate over sinks == */
    while (!sinkVector.empty()) {
        const auto &snkLnk = sinkVector.back();
        const auto &srcLnk = sourceVector.back();
        if (snkLnk.lowerDep_ == snkLnk.upperDep_) {
            if (srcLnk.lowerDep_ == srcLnk.upperDep_) {
                /* == Forward link between source and sink == */
                Spider::API::createEdge(srcLnk.vertex_, srcLnk.portIx_, srcLnk.rate_,
                                        snkLnk.vertex_, snkLnk.portIx_, snkLnk.rate_, StackID::TRANSFO);
                sourceVector.pop_back();
                sinkVector.pop_back();
            } else {
                /* == Source need a fork == */
                addForkVertex(sourceVector, sinkVector, linker.srdag_);
            }
        } else {
            /* == Sink need a join == */
            addJoinVertex(sourceVector, sinkVector, linker.srdag_);
        }
    }

    /* == Sanity check == */
    if (!sourceVector.empty()) {
        throwSpiderException("remaining sources to link after single rate transformation on edge: [%s].",
                             edge->name().c_str());
    }
}
