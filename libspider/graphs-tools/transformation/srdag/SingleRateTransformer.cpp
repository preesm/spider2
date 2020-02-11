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
#include <graphs-tools/transformation/srdag/visitors/SRDAGCopyVertexVisitor.h>
#include <graphs-tools/transformation/srdag/visitors/SRDAGCopyParamVisitor.h>
#include <graphs-tools/transformation/srdag/Transformation.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs-tools/numerical/dependencies.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>
#include <api/pisdf-api.h>

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::srdag::SingleRateTransformer::SingleRateTransformer(const TransfoJob &job, pisdf::Graph *srdag) :
        ref2Clone_{ sbc::vector < size_t, StackID::TRANSFO > { }}, job_{ job }, srdag_{ srdag } {
    auto *graph = job_.reference_;

    /* == 0. Compute the repetition vector == */
    if (graph->dynamic() || (job_.firingValue_ == 0) || (job_.root_)) {
        brv::compute(graph, job_.params_);
    }

    /* == 1. Split subgraphs (must be done after brv so that init and run have same repetition value) == */
    for (auto *subgraph : graph->subgraphs()) {
        spider::srdag::splitDynamicGraph(subgraph);
    }
    ref2Clone_ = sbc::vector<size_t, StackID::TRANSFO>(graph->vertexCount() +
                                                       graph->inputEdgeCount() +
                                                       graph->outputEdgeCount(), SIZE_MAX);
}

std::pair<spider::srdag::JobStack, spider::srdag::JobStack> spider::srdag::SingleRateTransformer::execute() {
    /* == Set dynamic dependent parameter values == */
    if (job_.reference_->dynamic()) {
        for (auto &param : job_.params_) {
            if (param->dynamic()) {
                param->setValue(param->value(job_.params_));
            }
        }
    }

    /* == 0. Insert repeat and tail actors for input and output interfaces, respectively == */
    replaceInterfaces();

    /* == 1. Copy the vertex accordingly to their repetition value == */
    spider::vector<pisdf::Vertex *> delayVertexToRemove;
    SRDAGCopyVertexVisitor visitor{ job_, srdag_ };
    for (const auto &vertex : job_.reference_->vertices()) {
        const auto &vertexUniformIx = uniformIx(vertex.get(), job_.reference_);
        vertex->visit(&visitor);
        ref2Clone_[vertexUniformIx] = visitor.ix_;
        if (vertex->subtype() == pisdf::VertexType::DELAY) {
            delayVertexToRemove.emplace_back(srdag_->vertices().back().get());
        }
    }

    /* == 1.1 Create the next and dynamic jobs == */
    auto futureJobs = makeFutureJobs();

    /* == 2. Perform single rate linkage for every edge of the reference graph == */
    for (auto &edge : job_.reference_->edges()) {
        singleRateLinkage(edge.get());
    }

    /* == 3. Remove the Graph instance inside the SR-DAG == */
    if (!job_.root_) {
        auto *srdagInstance = srdag_->vertex(*(job_.srdagIx_));
        srdag_->removeVertex(srdagInstance);
    }

    /* == 4. Remove the delay vertex added for the transformation == */
    for (const auto &vertex : delayVertexToRemove) {
        srdag_->removeVertex(vertex);
    }
    return futureJobs;
}

/* === Private method(s) implementation === */

void spider::srdag::SingleRateTransformer::replaceInterfaces() {
    if (!job_.reference_->inputEdgeCount() && !job_.reference_->outputEdgeCount()) {
        return;
    }

    /* == 0. Search for the instance in the SR-DAG == */
    auto *instance = srdag_->vertex(*(job_.srdagIx_));
    if (!instance || instance->name().find(job_.reference_->name()) == std::string::npos) {
        throwSpiderException("could not find matching single rate instance [%zu] of graph [%s]",
                             job_.firingValue_,
                             job_.reference_->name().c_str());
    }

    /* == 1. Replace the input interfaces == */
    {
        auto isInterfaceTransparent = [](const TransfoJob &job,
                                         const spider::pisdf::Interface *interface) -> bool {
            auto *edge = interface->outputEdge();
            auto *sink = edge->sink();
            const auto &sourceRate = edge->sourceRateValue();
            const auto &sinkRate = edge->sinkRateExpression().evaluate(job.params_);
            return (sink->repetitionValue() * sinkRate) == sourceRate;
        };
        for (const auto &interface : job_.reference_->inputInterfaceVector()) {
            auto *edge = instance->inputEdge(interface->ix());
            if (isInterfaceTransparent(job_, interface.get())) {
                ref2Clone_[uniformIx(interface.get(), job_.reference_)] = edge->source()->ix();
            } else {
                auto &&name = instance->name() + "_" + interface->name();
                auto *vertex = api::createRepeat(srdag_, std::move(name));
                edge->setSink(vertex, 0, Expression(edge->sinkRateExpression()));
                ref2Clone_[uniformIx(interface.get(), job_.reference_)] = vertex->ix();
            }
        }
    }

    /* == 2. Replace the output interfaces == */
    {
        auto isInterfaceTransparent = [](const TransfoJob &job,
                                         const spider::pisdf::Interface *interface) -> bool {
            auto *edge = interface->inputEdge();
            auto *source = edge->source();
            const auto &sourceRate = edge->sourceRateExpression().evaluate(job.params_);
            const auto &sinkRate = edge->sinkRateValue();
            return (source->repetitionValue() * sourceRate) == sinkRate;
        };
        for (const auto &interface : job_.reference_->outputInterfaceVector()) {
            auto *edge = instance->outputEdge(interface->ix());
            if (isInterfaceTransparent(job_, interface.get())) {
                ref2Clone_[uniformIx(interface.get(), job_.reference_)] = edge->sink()->ix();
            } else {
                auto &&name = instance->name() + "_" + interface->name();
                auto *vertex = api::createTail(srdag_, std::move(name), 1);
                edge->setSource(vertex, 0, Expression(edge->sourceRateExpression()));
                ref2Clone_[uniformIx(interface.get(), job_.reference_)] = vertex->ix();
            }
        }
    }
}

std::pair<spider::srdag::JobStack, spider::srdag::JobStack> spider::srdag::SingleRateTransformer::makeFutureJobs() {
    auto staticJobStack = factory::vector<TransfoJob>(StackID::TRANSFO);
    auto dynaJobStack = factory::vector<TransfoJob>(StackID::TRANSFO);

    /* == 0. Copy Params for static and dynamic jobs == */
    for (auto *subgraph : job_.reference_->subgraphs()) {
        /* == 1 Check if subgraph is an init graph or a run (or static) graph  == */
        if (subgraph->runReferenceGraph()) {
            /* == 2 Find the first job corresponding to the run graph == */
            const auto &runGraphSubIx = subgraph->runReferenceGraph()->subIx();
            auto *runGraph = job_.reference_->subgraphs()[runGraphSubIx];
            if (runGraph->repetitionValue() != subgraph->repetitionValue()) {
                // LCOV_IGNORE: this is a sanity check, it should never happen and it is not testable from the outside.
                throwSpiderException("Init graph [%s] does not have the same repetition value as run graph [%s] (%"
                                             PRIu32
                                             " != %"
                                             PRIu32
                                             ").",
                                     subgraph->name().c_str(), runGraph->name().c_str(),
                                     subgraph->repetitionValue(), runGraph->repetitionValue());
            }
            /* == 2.0 Creates dynamic job == */
            {
                auto *clone = srdag_->vertex(ref2Clone_[runGraph->ix()]);
                /* = same value but if clone->ix changes, value in transfojob will change also = */
                dynaJobStack.emplace_back(runGraph, clone->ix(), clone->instanceValue());
                auto &job = dynaJobStack.back();
                CopyParamVisitor cpyVisitor{ job_, job.params_ };
                for (const auto &param : runGraph->params()) {
                    param->visit(&cpyVisitor);
                }
            }

            /* == 2.1 Creates init job == */
            /* = same value but if clone->ix changes, value in transfojob will change also = */
            auto *initClone = srdag_->vertex(ref2Clone_[subgraph->ix()]);
            staticJobStack.emplace_back(subgraph, initClone->ix(), 0);
            auto &initJob = staticJobStack.back();

            /* == Copy the params == */
            auto &runJob = dynaJobStack.back();
            for (auto &param : runJob.params_) {
                initJob.params_.emplace_back(param);
            }
        } else if (!subgraph->dynamic()) {
            /* == 3. Add static jobs == */
            const auto &firstCloneIx = ref2Clone_[subgraph->ix()];
            for (auto ix = firstCloneIx; ix < firstCloneIx + subgraph->repetitionValue(); ++ix) {
                auto *clone = srdag_->vertex(ix);
                /* = same value but if clone->ix changes, value in transfojob will change also = */
                staticJobStack.emplace_back(subgraph, clone->ix(), clone->instanceValue());
                auto &job = staticJobStack.back();
                CopyParamVisitor cpyVisitor{ job_, job.params_ };
                for (const auto &param : subgraph->params()) {
                    param->visit(&cpyVisitor);
                }
            }
        }
    }
    return std::make_pair(std::move(staticJobStack), std::move(dynaJobStack));
}

void spider::srdag::SingleRateTransformer::singleRateLinkage(pisdf::Edge *edge) {
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
                api::createEdge(srcLnk.vertex_, srcLnk.portIx_, srcLnk.rate_,
                                snkLnk.vertex_, snkLnk.portIx_, snkLnk.rate_);
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
        // LCOV_IGNORE: this a sanity check, if go there, something bad happened that can not be tested directly
        throwSpiderException("remaining sources to link after single rate transformation on edge: [%s].",
                             edge->name().c_str());
    }
}

void spider::srdag::SingleRateTransformer::computeDependencies(pisdf::Edge *edge,
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
        auto snkLowerDep = pisdf::computeConsLowerDep(currentSinkRate, srcRate, firing, delay);
        auto snkUpperDep = pisdf::computeConsUpperDep(currentSinkRate, srcRate, firing, delay);

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
    auto *fork = api::createFork(srdag_,
                                 "fork-" + sourceLinker.vertex_->name() + "_out-" +
                                 std::to_string(sourceLinker.portIx_),
                                 (sourceLinker.upperDep_ - sourceLinker.lowerDep_) + 1);

    /* == Create an edge between source and fork == */
    api::createEdge(sourceLinker.vertex_,  /* = Vertex that need to explode = */
                    sourceLinker.portIx_,  /* = Source port ix = */
                    sourceLinker.rate_,    /* = Source rate = */
                    fork,                  /* = Added fork = */
                    0,                     /* = Fork has only one input port so 0 is fixed = */
                    sourceLinker.rate_    /* = Sink rate is the same as the source rate = */);
    srcVector.pop_back();

    /* == Connect out of fork == */
    auto remaining = sourceLinker.rate_;
    for (size_t i = 0; i < fork->outputEdgeCount() - 1; ++i) {
        const auto &sinkLinker = snkVector.back();
        remaining -= sinkLinker.rate_;
        api::createEdge(fork,               /* = Fork vertex = */
                        i,                  /* = Fork output to connect = */
                        sinkLinker.rate_,   /* = Sink rate = */
                        sinkLinker.vertex_, /* = Sink to connect to fork = */
                        sinkLinker.portIx_, /* = Sink port ix = */
                        sinkLinker.rate_    /* = Sink rate = */);
        snkVector.pop_back();
    }
    srcVector.emplace_back(remaining, static_cast<uint32_t>(fork->outputEdgeCount() - 1), fork);
    srcVector.back().lowerDep_ = sourceLinker.upperDep_;
    srcVector.back().upperDep_ = sourceLinker.upperDep_;
}

void spider::srdag::SingleRateTransformer::addJoinVertex(spider::vector<TransfoVertex> &srcVector,
                                                         spider::vector<TransfoVertex> &snkVector) {
    const auto &sinkLinker = snkVector.back();
    auto *join = api::createJoin(srdag_,
                                 "join-" + sinkLinker.vertex_->name() + "_in-" +
                                 std::to_string(sinkLinker.portIx_),
                                 (sinkLinker.upperDep_ - sinkLinker.lowerDep_) + 1);

    /* == Create an edge between source and fork == */
    api::createEdge(join, 0, sinkLinker.rate_, sinkLinker.vertex_, sinkLinker.portIx_, sinkLinker.rate_);
    snkVector.pop_back();

    /* == Connect in of join == */
    auto remaining = sinkLinker.rate_;
    for (size_t i = 0; i < join->inputEdgeCount() - 1; ++i) {
        const auto &sourceLinker = srcVector.back();
        remaining -= sourceLinker.rate_;
        api::createEdge(sourceLinker.vertex_, sourceLinker.portIx_, sourceLinker.rate_, join, i,
                        sourceLinker.rate_);
        srcVector.pop_back();
    }
    snkVector.emplace_back(remaining, static_cast<uint32_t>(join->inputEdgeCount() - 1), join);
    snkVector.back().lowerDep_ = sinkLinker.upperDep_;
    snkVector.back().upperDep_ = sinkLinker.upperDep_;

}

spider::vector<spider::srdag::SingleRateTransformer::TransfoVertex>
spider::srdag::SingleRateTransformer::buildSinkLinkerVector(pisdf::Edge *edge) {
    /* == 0. Reserve size of the vector == */
    auto sinkVector = factory::vector<TransfoVertex>(StackID::TRANSFO);
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
            populateFromDelayVertex(sinkVector, outputEdge, true);
        } else {
            /* == 1.2 Connect to the clone == */
            populateTransfoVertexVector(sinkVector, delay->vertex(), delay->value(params), 1);
        }
    }

    /* == 2. Populate the rest of the sinkVector == */
    auto *clone = srdag_->vertex(ref2Clone_[uniformIx(sink, job_.reference_)]);
    if (sink->subtype() == pisdf::VertexType::OUTPUT) {
        /* == 2.0 Check if we are in the trivial case of no interface == */
        if (clone->subtype() != pisdf::VertexType::TAIL) {
            auto *graphInstance = srdag_->vertex(*(job_.srdagIx_));
            auto *outputEdge = graphInstance->outputEdge(sink->ix());
            populateTransfoVertexVector(sinkVector, sink, outputEdge->sinkRateValue(), outputEdge->sinkPortIx());
            srdag_->removeEdge(outputEdge);
        } else {
            const auto &rate = edge->sourceRateExpression().evaluate(job_.params_) * edge->source()->repetitionValue();
            populateTransfoVertexVector(sinkVector, sink, rate, edge->sinkPortIx());
        }
    } else if (sink->subtype() == pisdf::VertexType::DELAY && clone->outputEdge(1)) {
        /* == 2.1 We already connected sink of original edge containing the delay, we'll use it directly == */
        populateFromDelayVertex(sinkVector, clone->outputEdge(1), true);
    } else {
        /* == 2.2 Normal case == */
        const auto &rate = edge->sinkRateExpression().evaluate(job_.params_);
        populateTransfoVertexVector(sinkVector, sink, rate, edge->sinkPortIx());
    }
    return sinkVector;
}

spider::vector<spider::srdag::SingleRateTransformer::TransfoVertex>
spider::srdag::SingleRateTransformer::buildSourceLinkerVector(pisdf::Edge *edge) {
    /* == 0. Reserve size of the vector == */
    auto sourceVector = factory::vector<TransfoVertex>(StackID::TRANSFO);
    auto *source = edge->source();
    auto *delay = edge->delay();
    sourceVector.reserve(source->repetitionValue() + (delay != nullptr));

    /* == 1. Populate the sourceVector == */
    auto *clone = srdag_->vertex(ref2Clone_[uniformIx(source, job_.reference_)]);
    if (source->subtype() == pisdf::VertexType::INPUT) {
        /* == 1.0 Check if we are in the trivial case of no interface == */
        if (clone->subtype() != pisdf::VertexType::REPEAT) {
            auto *graphInstance = srdag_->vertex(*(job_.srdagIx_));
            auto *inputEdge = graphInstance->inputEdge(source->ix());
            populateTransfoVertexVector(sourceVector, source, inputEdge->sourceRateValue(), inputEdge->sourcePortIx());
            srdag_->removeEdge(inputEdge);
        } else {
            const auto &rate = edge->sinkRateExpression().evaluate(job_.params_) * edge->sink()->repetitionValue();
            populateTransfoVertexVector(sourceVector, source, rate, edge->sourcePortIx());
        }
    } else if (source->subtype() == pisdf::VertexType::DELAY && clone->inputEdge(1)) {
        /* == 1.1 We already connected source of original edge containing the delay, we'll use it directly == */
        populateFromDelayVertex(sourceVector, clone->inputEdge(1), false);
    } else {
        /* == 1.2 Normal case == */
        const auto &rate = edge->sourceRateExpression().evaluate(job_.params_);
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
            populateFromDelayVertex(sourceVector, inputEdge, false);
        } else {
            /* == 2.2 Connect to the clone == */
            populateTransfoVertexVector(sourceVector, delay->vertex(), delay->value(params), 1);
        }
    }

    return sourceVector;
}


