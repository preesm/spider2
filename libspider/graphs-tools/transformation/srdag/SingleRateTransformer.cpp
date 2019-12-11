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

/* === Include(s) === */

#include <graphs-tools/transformation/srdag/SingleRateTransformer.h>
#include <graphs-tools/transformation/srdag/visitors/CopyVisitor.h>
#include <graphs-tools/transformation/srdag/visitors/CopyParamVisitor.h>
#include <graphs-tools/transformation/srdag/Transformation.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs-tools/numerical/dependencies.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <api/pisdf-api.h>

/* === Static function(s) === */

/**
 * @brief Split every dynamic subgraphs of a given graph into an init and a run subgraph.
 * @param graph  Graph to evaluate.
 * @return @refitem spider::vector of index linking init to run subgraphs.
 */
static spider::vector<size_t> splitSubgraphs(spider::pisdf::Graph *graph) {
    /* == 0. Split the dynamic subgraphs into init and run subgraphs == */
    auto subgraph2RemoveVector = spider::containers::vector<PiSDFGraph *>(StackID::TRANSFO);
    auto subgraph2AddVector = spider::containers::vector<std::pair<PiSDFGraph *, PiSDFGraph *>>(StackID::TRANSFO);
    subgraph2RemoveVector.reserve(graph->subgraphCount());
    subgraph2AddVector.reserve(graph->subgraphCount());
    for (auto *subgraph : graph->subgraphs()) {
        auto &&result = spider::srdag::splitDynamicGraph(subgraph);
        if (result.first) {
            subgraph2RemoveVector.emplace_back(subgraph);
            subgraph2AddVector.emplace_back(std::move(result));
        }
    }

    /* == 1. Remove the subgraphs from the graph == */
    for (auto &subgraph : subgraph2RemoveVector) {
        graph->removeVertex(subgraph);
    }

    /* == 2. Set the link between init to run graphs == */
    auto init2Run = spider::containers::vector<size_t>(graph->subgraphCount(), SIZE_MAX, StackID::TRANSFO);
    for (auto &pair : subgraph2AddVector) {
        auto *initGraph = pair.first;
        auto *runGraph = pair.second;
        init2Run[initGraph->subIx()] = runGraph->subIx();
    }
    return init2Run;
}

/* === Method(s) implementation === */

spider::srdag::SingleRateTransformer::SingleRateTransformer(const spider::srdag::TransfoJob &job,
                                                            pisdf::Graph *srdag) : job_{ job }, srdag_{ srdag } {
    auto *graph = job_.reference_;

    /* == 0. Split subgraphs == */
    init2run_ = splitSubgraphs(graph);
    ref2Clone_ = spider::containers::vector<size_t>(graph->vertexCount() +
                                                    graph->inputEdgeCount() +
                                                    graph->outputEdgeCount(), SIZE_MAX, StackID::TRANSFO);

    /* == 1. Compute the repetition vector == */
    if (graph->dynamic() || (job_.firingValue_ == 0) || (job_.firingValue_ == UINT32_MAX)) {
        spider::brv::compute(graph, job_.params_);
    }
}

std::pair<spider::srdag::JobStack, spider::srdag::JobStack> spider::srdag::SingleRateTransformer::execute() {
    /* == 0. Insert repeat and tail actors for input and output interfaces, respectively == */
    replaceInterfaces();

    /* == 1. Copy the vertex accordingly to their repetition value == */
    for (const auto &vertex : job_.reference_->vertices()) {
        const auto &vertexUniformIx = uniformIx(vertex, job_.reference_);
        CopyVisitor visitor{ job_, srdag_ };
        vertex->visit(&visitor);
        ref2Clone_[vertexUniformIx] = visitor.ix_;
    }

    /* == 1.1 Create the next and dynamic jobs == */
    auto futureJobs = makeFutureJobs();

    /* == 2. Perform single rate linkage for every edge of the reference graph == */
    for (auto *edge : job_.reference_->edges()) {
        singleRateLinkage(edge);
    }

    /* == 3. Remove the Graph instance inside the SR-DAG == */
    if (job_.firingValue_ != UINT32_MAX) {
        auto *srdagInstance = srdag_->vertex(job_.srdagIx_);
        srdag_->removeVertex(srdagInstance);
    }

    return futureJobs;
}

/* === Private method(s) implementation === */

void spider::srdag::SingleRateTransformer::replaceInterfaces() {
    if (!job_.reference_->inputEdgeCount() && !job_.reference_->outputEdgeCount()) {
        return;
    }

    /* == 0. Search for the instance in the SR-DAG == */
    auto *instance = srdag_->vertex(job_.srdagIx_);
    if (!instance) {
        throwSpiderException("could not find matching single rate instance [%zu] of graph [%s]",
                             job_.firingValue_,
                             job_.reference_->name().c_str());
    }

    /* == 1. Replace the input interfaces == */
    for (const auto &interface : job_.reference_->inputInterfaceArray()) {
        auto *edge = instance->inputEdge(interface->ix());
        auto &&name = instance->name() + "_" + interface->name();
        auto *vertex = spider::api::createRepeat(srdag_, std::move(name), StackID::TRANSFO);
        edge->setSink(vertex, 0, Expression(edge->sinkRateExpression()));
        ref2Clone_[uniformIx(interface, job_.reference_)] = vertex->ix();
    }

    /* == 2. Replace the output interfaces == */
    for (const auto &interface : job_.reference_->outputInterfaceArray()) {
        auto *edge = instance->outputEdge(interface->ix());
        auto &&name = instance->name() + "_" + interface->name();
        auto *vertex = spider::api::createTail(srdag_, std::move(name), 1, StackID::TRANSFO);
        edge->setSource(vertex, 0, Expression(edge->sourceRateExpression()));
        ref2Clone_[uniformIx(interface, job_.reference_)] = vertex->ix();
    }
}

std::pair<spider::srdag::JobStack, spider::srdag::JobStack> spider::srdag::SingleRateTransformer::makeFutureJobs() {
    auto nextJobs = spider::containers::vector<TransfoJob>(StackID::TRANSFO);
    auto dynaJobs = spider::containers::vector<TransfoJob>(StackID::TRANSFO);

    /* == 0. Copy Params for static and dynamic jobs == */
    auto initGraphVector = spider::containers::vector<pisdf::Graph *>(StackID::TRANSFO);
    for (auto *subgraph : job_.reference_->subgraphs()) {
        /* == 0.1 Check if subgraph is an init graph or a run (or static) graph  == */
        const auto &runGraphSubIx = init2run_[subgraph->subIx()];
        if (runGraphSubIx != SIZE_MAX) {
            initGraphVector.emplace_back(subgraph);
        } else {
            auto &jobStack = subgraph->dynamic() ? dynaJobs : nextJobs;
            const auto &firstCloneIx = ref2Clone_[subgraph->ix()];
            for (auto ix = firstCloneIx + subgraph->repetitionValue() - 1; ix >= firstCloneIx; --ix) {
                auto *clone = srdag_->vertex(ix);
                jobStack.emplace_back(subgraph, clone->ix(), ix - firstCloneIx);

                /* == Copy the params == */
                CopyParamVisitor cpyVisitor{ job_, jobStack.back().params_ };
                for (const auto &param : subgraph->params()) {
                    param->visit(&cpyVisitor);
                }
            }
        }
    }

    /* == 1. Copy param pointers from run graph jobs to init graph jobs == */
    for (auto *subgraph : initGraphVector) {
        /* == 1.1 Find the first job corresponding to the init graph == */
        const auto &runGraphSubIx = init2run_[subgraph->subIx()];
        auto *runGraph = job_.reference_->subgraphs()[runGraphSubIx];
        auto it = dynaJobs.begin();
        while ((it != dynaJobs.end()) && ((*it).reference_ != runGraph)) { it++; }
        if (it == dynaJobs.end()) {
            throwSpiderException("Init graph [%s] did not find run counter part [%s].",
                                 subgraph->name().c_str(),
                                 runGraph->name().c_str());
        }

        /* == 1.2 Do the actual copy == */
        const auto &firstCloneIx = ref2Clone_[subgraph->ix()];
        for (auto ix = firstCloneIx + runGraph->repetitionValue() - 1; ix >= firstCloneIx; --ix) {
            auto *clone = srdag_->vertex(ix);
            nextJobs.emplace_back(subgraph, clone->ix(), ix - firstCloneIx);

            auto &job = nextJobs.back();
            job.params_.reserve(runGraph->paramCount());
            for (auto &param : (*it).params_) {
                job.params_.emplace_back(param);
            }
            it++;
        }
    }
    return std::make_pair(std::move(nextJobs), std::move(dynaJobs));
}

void spider::srdag::SingleRateTransformer::singleRateLinkage(spider::pisdf::Edge *edge) {
    if ((edge->source() == edge->sink())) {
        if (!edge->delay()) {
            throwSpiderException("No delay on edge with self loop.");
        }
    }

    /* == 0. Create vectors of source and sinks for the linkage == */
    auto sourceVector = buildSourceLinkerVector(edge);
    auto sinkVector = buildSinkLinkerVector(edge);

    /* == 1. Compute the different dependencies of the sink vertices over the source vertices == */
    computeDependencies(edge, sourceVector, sinkVector);

    /* == 2. Iterate until there are no sink anymore == */
    while (!sinkVector.empty()) {
        const auto &snkLnk = sinkVector.back();
        const auto &srcLnk = sourceVector.back();
        if (snkLnk.lowerDep_ == snkLnk.upperDep_) {
            if (srcLnk.lowerDep_ == srcLnk.upperDep_) {
                /* == 2.1 Forward link between source and sink == */
                spider::api::createEdge(srcLnk.vertex_, srcLnk.portIx_, srcLnk.rate_,
                                        snkLnk.vertex_, snkLnk.portIx_, snkLnk.rate_, StackID::TRANSFO);
                sourceVector.pop_back();
                sinkVector.pop_back();
            } else {
                /* == 2.2 Source need a fork == */
                addForkVertex(sourceVector, sinkVector);
            }
        } else {
            /* == 2.3 Sink need a join == */
            addJoinVertex(sourceVector, sinkVector);
        }
    }

    /* == 3. Sanity check == */
    if (!sourceVector.empty()) {
        throwSpiderException("remaining sources to link after single rate transformation on edge: [%s].",
                             edge->name().c_str());
    }
}

void spider::srdag::SingleRateTransformer::computeDependencies(spider::pisdf::Edge *edge,
                                                               spider::vector<TransfoVertex> &srcVector,
                                                               spider::vector<TransfoVertex> &snkVector) {
    auto &&delay = edge->delay() ? edge->delay()->value(job_.params_) : 0;
    const auto &srcRate = srcVector[0].rate_;     /* = This should be the proper source rate of the edge = */
    const auto &snkRate = snkVector.back().rate_; /* = This should be the proper sink rate of the edge = */
    const auto &getterRate = edge->delay() ? snkVector[0].rate_ : 0;
    const auto &sinkRepetitionValue = edge->sink()->repetitionValue();
    const auto &setterOffset = edge->delay() ? 1 : 0;

    /* == Compute dependencies for sinks == */
    uint32_t firing = 0;
    auto currentSinkRate = snkRate;
    for (auto it = snkVector.rbegin(); it < snkVector.rend(); ++it) {
        if (it == snkVector.rbegin() + sinkRepetitionValue) {
            /* == We've reached the end / getter vertices == */
            delay = delay - snkRate * sinkRepetitionValue;
            currentSinkRate = getterRate;
            firing = 0;
        }
        auto snkLowerDep = spider::pisdf::computeConsLowerDep(currentSinkRate, srcRate, firing, delay);
        auto snkUpperDep = spider::pisdf::computeConsUpperDep(currentSinkRate, srcRate, firing, delay);

        /* == Adjust the values to match the actual position in the source vector == */
        snkLowerDep += setterOffset;
        snkUpperDep += setterOffset;
        (*it).lowerDep_ = static_cast<uint32_t>(snkLowerDep);
        (*it).upperDep_ = static_cast<uint32_t>(snkUpperDep);
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

void spider::srdag::SingleRateTransformer::addForkVertex(spider::vector<TransfoVertex> &srcVector,
                                                         spider::vector<TransfoVertex> &snkVector) {
    const auto &sourceLinker = srcVector.back();
    auto *fork = spider::api::createFork(srdag_,
                                         "fork-" + sourceLinker.vertex_->name() + "_out-" +
                                         std::to_string(sourceLinker.portIx_),
                                         (sourceLinker.upperDep_ - sourceLinker.lowerDep_) + 1,
                                         StackID::TRANSFO);

    /* == Create an edge between source and fork == */
    spider::api::createEdge(sourceLinker.vertex_,  /* = Vertex that need to explode = */
                            sourceLinker.portIx_,  /* = Source port ix = */
                            sourceLinker.rate_,    /* = Source rate = */
                            fork,                  /* = Added fork = */
                            0,                     /* = Fork has only one input port so 0 is fixed = */
                            sourceLinker.rate_,    /* = Sink rate is the same as the source rate = */
                            StackID::TRANSFO);
    srcVector.pop_back();

    /* == Connect out of fork == */
    auto remaining = sourceLinker.rate_;
    for (size_t i = 0; i < fork->outputEdgeCount() - 1; ++i) {
        const auto &sinkLinker = snkVector.back();
        remaining -= sinkLinker.rate_;
        spider::api::createEdge(fork,               /* = Fork vertex = */
                                i,                  /* = Fork output to connect = */
                                sinkLinker.rate_,   /* = Sink rate = */
                                sinkLinker.vertex_, /* = Sink to connect to fork = */
                                sinkLinker.portIx_, /* = Sink port ix = */
                                sinkLinker.rate_,   /* = Sink rate = */
                                StackID::TRANSFO);
        snkVector.pop_back();
    }
    srcVector.emplace_back(remaining, static_cast<uint32_t>(fork->outputEdgeCount() - 1), fork);
    srcVector.back().lowerDep_ = sourceLinker.upperDep_;
    srcVector.back().upperDep_ = sourceLinker.upperDep_;
}

void spider::srdag::SingleRateTransformer::addJoinVertex(spider::vector<TransfoVertex> &srcVector,
                                                         spider::vector<TransfoVertex> &snkVector) {
    const auto &sinkLinker = snkVector.back();
    auto *join = spider::api::createJoin(srdag_,
                                         "join-" + sinkLinker.vertex_->name() + "_in-" +
                                         std::to_string(sinkLinker.portIx_),
                                         (sinkLinker.upperDep_ - sinkLinker.lowerDep_) + 1,
                                         StackID::TRANSFO);

    /* == Create an edge between source and fork == */
    spider::api::createEdge(join, 0, sinkLinker.rate_,
                            sinkLinker.vertex_, sinkLinker.portIx_, sinkLinker.rate_, StackID::TRANSFO);
    snkVector.pop_back();

    /* == Connect in of join == */
    auto remaining = sinkLinker.rate_;
    for (size_t i = 0; i < join->inputEdgeCount() - 1; ++i) {
        const auto &sourceLinker = srcVector.back();
        remaining -= sourceLinker.rate_;
        spider::api::createEdge(sourceLinker.vertex_, sourceLinker.portIx_, sourceLinker.rate_,
                                join, i, sourceLinker.rate_, StackID::TRANSFO);
        srcVector.pop_back();
    }
    snkVector.emplace_back(remaining, static_cast<uint32_t>(join->inputEdgeCount() - 1), join);
    snkVector.back().lowerDep_ = sinkLinker.upperDep_;
    snkVector.back().upperDep_ = sinkLinker.upperDep_;

}

spider::vector<spider::srdag::SingleRateTransformer::TransfoVertex>
spider::srdag::SingleRateTransformer::buildSinkLinkerVector(spider::pisdf::Edge *edge) {
    /* == 0. Reserve size of the vector == */
    auto sinkVector = spider::containers::vector<TransfoVertex>(StackID::TRANSFO);
    auto *sink = edge->sink();
    auto *delay = edge->delay();
    sinkVector.reserve(sink->repetitionValue() + (delay != nullptr));

    /* == 1. If delay, populate the getter clones in reverse order == */
    if (delay) {
        const auto &params = job_.params_;
        if ((sink == edge->source()) &&
            delay->value(params) < edge->sinkRateExpression().evaluate(params)) {
            throwSpiderException("Insufficient delay [%"
                                         PRIu32
                                         "] on edge [%s].",
                                 delay->value(params),
                                 edge->name().c_str());
        }
        const auto *delayVertex = delay->vertex();
        auto *clone = srdag_->vertex(ref2Clone_[delayVertex->ix()]);
        auto *outputEdge = clone->outputEdge(0);
        if (outputEdge) {
            /* == 1.1 We already connected getter, we'll use it directly == */
            populateFromDelayVertex(sinkVector, outputEdge, clone, true);
        } else {
            /* == 1.2 Connect to the clone == */
            populateTransfoVertexVector(sinkVector, delay->vertex(), delay->value(params), 1);
        }
    }

    /* == 2. Populate the rest of the sinkVector == */
    auto *clone = srdag_->vertex(ref2Clone_[sink->ix()]);
    if (sink->subtype() == pisdf::VertexType::DELAY && clone->outputEdge(1)) {
        /* == 2.1 We already connected sink of original edge containing the delay, we'll use it directly == */
        populateFromDelayVertex(sinkVector, clone->outputEdge(1), clone, true);
    } else {
        /* == 2.2 Populate the sink clones in reverse order == */
        const auto &params = job_.params_;
        const auto &rate =
                sink->subtype() == PiSDFVertexType::OUTPUT ? edge->sourceRateExpression().evaluate(params) *
                                                             edge->source()->repetitionValue()
                                                           : edge->sinkRateExpression().evaluate(params);
        populateTransfoVertexVector(sinkVector, sink, rate, edge->sinkPortIx());
    }
    return sinkVector;
}

spider::vector<spider::srdag::SingleRateTransformer::TransfoVertex>
spider::srdag::SingleRateTransformer::buildSourceLinkerVector(spider::pisdf::Edge *edge) {
    /* == 0. Reserve size of the vector == */
    auto sourceVector = spider::containers::vector<TransfoVertex>(StackID::TRANSFO);
    auto *source = edge->source();
    auto *delay = edge->delay();
    sourceVector.reserve(source->repetitionValue() + (delay != nullptr));

    /* == 1. Populate the sourceVector == */
    auto *clone = srdag_->vertex(ref2Clone_[source->ix()]);
    if (source->subtype() == pisdf::VertexType::DELAY && clone->inputEdge(1)) {
        /* == 1.1 We already connected source of original edge containing the delay, we'll use it directly == */
        populateFromDelayVertex(sourceVector, clone->inputEdge(1), clone, false);
    } else {
        /* == 1.2 Populate the source clones in reverse order == */
        const auto &params = job_.params_;
        const auto &rate = source->subtype() == PiSDFVertexType::INPUT ? edge->sinkRateExpression().evaluate(params) *
                                                                         edge->sink()->repetitionValue()
                                                                       : edge->sourceRateExpression().evaluate(params);
        populateTransfoVertexVector(sourceVector, source, rate, edge->sourcePortIx());
    }

    /* == 2. If delay, populate the setter clones in reverse order == */
    if (delay) {
        const auto &params = job_.params_;
        const auto *delayVertex = delay->vertex();
        auto *delayClone = srdag_->vertex(ref2Clone_[delayVertex->ix()]);
        auto *inputEdge = delayClone->inputEdge(0);
        if (inputEdge) {
            /* == 2.1 We already connected setter, we'll use it directly == */
            populateFromDelayVertex(sourceVector, inputEdge, delayClone, false);
        } else {
            /* == 2.2 Connect to the clone == */
            populateTransfoVertexVector(sourceVector, delay->vertex(), delay->value(params), 1);
        }
    }

    return sourceVector;
}


