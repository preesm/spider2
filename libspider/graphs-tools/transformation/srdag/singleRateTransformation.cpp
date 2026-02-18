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
#ifndef _NO_BUILD_LEGACY_RT

/* === Includes === */

#include <graphs-tools/transformation/srdag/singleRateTransformation.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs-tools/numerical/dependencies.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/srdag/SRDAGGraph.h>
#include <graphs/srdag/SRDAGEdge.h>
#include <graphs/srdag/SRDAGVertex.h>

/* === Static function(s) === */

namespace {
    size_t getIx(const spider::pisdf::Vertex *vertex, const spider::pisdf::Graph *graph) {
        if (vertex->subtype() == spider::pisdf::VertexType::INPUT) {
            return vertex->ix() + graph->vertexCount();
        } else if (vertex->subtype() == spider::pisdf::VertexType::OUTPUT) {
            return vertex->ix() + graph->vertexCount() + graph->inputEdgeCount();
        }
        return vertex->ix();
    }
}

/* === Methods implementation === */

std::pair<spider::srdag::JobStack, spider::srdag::JobStack>
spider::srdag::singleRateTransformation(TransfoJob &job, srdag::Graph *srdag) {
    auto *reference = job.reference_;
    if (!srdag || !reference || (reference->graph() && !job.srdagInstance_)) {
        throwNullptrException();
    }
    if (reference->configVertexCount() && (reference->subgraphCount() != 1) &&
        (reference->vertexCount() != reference->configVertexCount())) {
        pisdf::separateRunGraphFromInit(reference);
    }
    /* == 0. Set dynamic dependent parameter values == */
    detail::updateParams(job);
    /* == 1. Compute the repetition vector only for first firing of static graph or if graph is dynamic == */
    if (!job.firingValue_) {
        brv::compute(reference, job.params_);
    }
    const auto vertexCount = reference->vertexCount() + reference->inputEdgeCount() + reference->outputEdgeCount();
    auto ref2CloneVector = factory::vector<size_t>(vertexCount, SIZE_MAX, StackID::TRANSFO);
    /* == 2. Clone vertices accordingly to their repetition value == */
    auto delayVertexToRemove = factory::vector<srdag::Vertex *>(StackID::TRANSFO);
    for (const auto &vertex : reference->vertices()) {
        const auto rv = vertex->repetitionValue();
        for (u32 k = 0; k < rv; ++k) {
            detail::cloneVertex(vertex.get(), k, srdag, job);
        }
        ref2CloneVector[getIx(vertex.get(), reference)] = srdag->vertexCount() - rv;
        if (vertex->subtype() == pisdf::VertexType::DELAY) {
            delayVertexToRemove.emplace_back(srdag->vertices().back().get());
        }
    }
    /* == 3. Create the next and dynamic jobs == */
    auto futureJobs = detail::makeFutureJobs(reference, srdag, ref2CloneVector, job.params_);
    /* == 4. Perform single rate linkage for every edge of the reference graph == */
    for (auto &edge : reference->edges()) {
        detail::singleRateLinkage(edge.get(), job, srdag, ref2CloneVector);
    }
    /* == 5. Remove the Graph instance inside the SR-DAG == */
    srdag->removeVertex(job.srdagInstance_);
    job.srdagInstance_ = nullptr;
    /* == 6. Remove the delay vertex added for the transformation == */
    for (const auto &vertex : delayVertexToRemove) {
        srdag->removeVertex(vertex);
    }
    /* == 7. Remove unconnected edges (due to delays) == */
    auto it = std::begin(srdag->edges());
    while (it != std::end(srdag->edges())) {
        auto *edge = it->get();
        if (!edge->source() && !edge->sink()) {
            auto dist = std::distance(std::begin(srdag->edges()), it);
            srdag->removeEdge(edge);
            it = std::next(std::begin(srdag->edges()), dist);
        } else {
            it++;
        }
    }
    return futureJobs;
}

/* === Detail methods implementation === */

void spider::srdag::detail::updateParams(TransfoJob &job) {
    const auto *graph = job.reference_;
    if (!graph->configVertexCount()) {
        for (auto &param : job.params_) {
            if (param->type() == pisdf::ParamType::INHERITED) {
                const auto *parent = param->parent();
                if (!parent) {
                    throwNullptrException();
                }
                const auto value = parent->value(job.params_);
                const auto ix = param->ix();
                param = spider::make_shared<pisdf::Param, StackID::TRANSFO>(param->name(), value);
                param->setIx(ix);
            } else if (param->type() == pisdf::ParamType::DYNAMIC_DEPENDANT) {
                const auto value = param->value(job.params_);
                param = spider::make_shared<pisdf::Param, StackID::TRANSFO>(param->name(), value);
            }
        }
    }
}

void spider::srdag::detail::cloneVertex(const pisdf::Vertex *vertex, u32 firing, Graph *srdag, const TransfoJob &job) {
    if (vertex->subtype() == pisdf::VertexType::DELAY) {
        /* == This a trick to ensure proper coherence even with recursive delay init == */
        /* == For given scenario:   A -> | delay | -> B
         *                         setter --^ --> getter
         *    This will produce this:
         *                          setter -> | delay | -> getter
         *                               A -> |       | -> B
         *    But in reality the vertex does not make it after the SR-Transformation.
         */
        auto *clone = make<srdag::Vertex, StackID::TRANSFO>(vertex, firing, 2u, 2u);
        clone->setExecutable(false);
        /* == Add clone to the srdag == */
        srdag->addVertex(clone);
    } else {
        auto *clone = make<srdag::Vertex, StackID::TRANSFO>(vertex, firing, vertex->inputEdgeCount(),
                                                            vertex->outputEdgeCount());
        clone->setExecutable(vertex->executable());
        /* == Add clone to the srdag == */
        srdag->addVertex(clone);
        /* == Get the cloned parameters == */
        for (const auto ix : vertex->inputParamIxVector()) {
            clone->addInputParameter(job.params_[ix]);
        }
        for (const auto ix : vertex->refinementParamIxVector()) {
            clone->addRefinementParameter(job.params_[ix]);
        }
    }
}

std::pair<spider::srdag::JobStack, spider::srdag::JobStack>
spider::srdag::detail::makeFutureJobs(const pisdf::Graph *graph,
                                      srdag::Graph *srdag,
                                      const spider::vector<size_t> &ref2CloneVector,
                                      const spider::vector<std::shared_ptr<pisdf::Param>> &jobParams) {
    auto staticJobStack = factory::vector<TransfoJob>(StackID::TRANSFO);
    auto dynaJobStack = factory::vector<TransfoJob>(StackID::TRANSFO);
    /* == Creates future TransfoJob == */
    const auto isDynamic = graph->dynamic();
    for (auto *subgraph : graph->subgraphs()) {
        const auto &params = subgraph->params();
        const auto firstCloneIx = ref2CloneVector[subgraph->ix()];
        auto &stack = isDynamic ? dynaJobStack : staticJobStack;
        for (auto ix = firstCloneIx; ix < firstCloneIx + subgraph->repetitionValue(); ++ix) {
            auto futurJob = TransfoJob{ subgraph, srdag->vertex(ix), static_cast<u32>((ix - firstCloneIx)) };
            /* == Copy the params == */
            futurJob.params_.reserve(params.size());
            for (const auto &param : params) {
                futurJob.params_.emplace_back(detail::copyParameter(param, jobParams));
            }
            stack.emplace_back(std::move(futurJob));
        }
    }
    /* == Update reference of config vertices parameters == */
    for (const auto &cfg : graph->configVertices()) {
        auto *clone = srdag->vertex(ref2CloneVector[cfg->ix()]);
        for (const auto ix : cfg->outputParamIxVector()) {
            clone->addOutputParameter(jobParams[ix]);
        }
    }
    return std::make_pair(std::move(staticJobStack), std::move(dynaJobStack));
}

std::shared_ptr<spider::pisdf::Param> spider::srdag::detail::copyParameter(const std::shared_ptr<pisdf::Param> &param,
                                                                           const spider::vector<std::shared_ptr<pisdf::Param>> &jobParams) {
    if (param->dynamic()) {
        std::shared_ptr<pisdf::Param> p;
        if (param->type() == pisdf::ParamType::INHERITED) {
            const auto *parent = param->parent();
            if (!parent) {
                throwNullptrException();
            }
            const auto &parentParam = jobParams[parent->ix()];
            p = spider::make_shared<pisdf::Param, StackID::TRANSFO>(param->name(), parentParam);
        } else {
            p = spider::make_shared<pisdf::Param, StackID::TRANSFO>(*param);
        }
        p->setIx(param->ix());
        return p;
    }
    return param;
}

void spider::srdag::detail::singleRateLinkage(const pisdf::Edge *edge,
                                              TransfoJob &job,
                                              srdag::Graph *srdag,
                                              const spider::vector<size_t> &ref2CloneVector) {
    if ((edge->source() == edge->sink()) && !edge->delay()) {
        throwSpiderException("No delay on edge with self loop.");
    }
    /* == Check for null edge == */
    if (detail::checkForNullEdge(edge, job, srdag, ref2CloneVector)) {
        return;
    }
    /* == 0. Create vectors of source and sinks for the linkage == */
    auto sourceVector = detail::buildSourceLinkerVector(edge, job, srdag, ref2CloneVector);
    auto sinkVector = detail::buildSinkLinkerVector(edge, job, srdag, ref2CloneVector);
    /* == 1. Compute the different dependencies of the sink vertices over the source vertices == */
    detail::computeDependencies(edge, sourceVector, sinkVector);
    /* == 2. Iterate until there are no sink anymore == */
    while (!sinkVector.empty()) {
        const auto &snkLnk = sinkVector.back();
        const auto &srcLnk = sourceVector.back();
        if (snkLnk.lowerDep_ == snkLnk.upperDep_) {
            if (srcLnk.lowerDep_ == srcLnk.upperDep_) {
                /* == 2.1 Forward link between source and sink == */
                srdag->createEdge(srcLnk.vertex_, srcLnk.portIx_, snkLnk.vertex_, snkLnk.portIx_, snkLnk.rate_);
                sourceVector.pop_back();
                sinkVector.pop_back();
            } else {
                /* == 2.2 Source need a fork == */
                detail::addForkVertex(sourceVector, sinkVector, srdag);
            }
        } else {
            /* == 2.3 Sink need a join == */
            detail::addJoinVertex(sourceVector, sinkVector, srdag);
        }
    }
    /* == 3. Sanity check == */
    if (!sourceVector.empty()) {
        // LCOV_IGNORE: this a sanity check, if go there, something bad happened that can not be tested directly
        throwSpiderException("remaining sources to link after single rate transformation on edge: [%s].",
                             edge->name().c_str());
    }
}

bool spider::srdag::detail::checkForNullEdge(const pisdf::Edge *edge,
                                             TransfoJob &job,
                                             srdag::Graph *srdag,
                                             const spider::vector<size_t> &ref2CloneVector) {
    if (!(edge->sourceRateExpression().evaluate(job.params_)) && !(edge->sinkRateExpression().evaluate(job.params_))) {
        /* == Add an empty INIT to the sink == */
        const auto *sink = edge->sink();
        if (sink->repetitionValue()) {
            auto start = ref2CloneVector[getIx(sink, job.reference_)];
            for (auto i = start; i < start + sink->repetitionValue(); ++i) {
                auto *clone = srdag->vertex(i);
                auto name = std::string("void::in::").append(clone->name()).append(":").append(
                        std::to_string(edge->sinkPortIx()));
                auto *init = srdag->createVoidVertex(std::move(name), 0, 1);
                srdag->createEdge(init, 0, clone, edge->sinkPortIx(), 0);
            }
        }
        /* == Add an empty END to the source == */
        const auto *source = edge->source();
        if (source->repetitionValue()) {
            auto start = ref2CloneVector[getIx(source, job.reference_)];
            for (auto i = start; i < start + source->repetitionValue(); ++i) {
                auto *clone = srdag->vertex(i);
                auto name = std::string("void::out::").append(clone->name()).append(":").append(
                        std::to_string(edge->sourcePortIx()));
                auto *end = srdag->createVoidVertex(std::move(name), 1, 0);
                srdag->createEdge(clone, edge->sourcePortIx(), end, 0, 0);
            }
        }
        return true;
    }
    return false;
}

void spider::srdag::detail::computeDependencies(const pisdf::Edge *edge,
                                                spider::vector<TransfoVertex> &srcVector,
                                                spider::vector<TransfoVertex> &snkVector) {
    const auto srcRate = srcVector[0].rate_;     /* = This should be the proper source rate of the edge = */
    const auto snkRate = snkVector.back().rate_; /* = This should be the proper sink rate of the edge = */
    const auto sinkRepetitionValue = edge->sink()->repetitionValue();
    const auto setterOffset = edge->delay() ? 1 : 0;
    auto computeWorker = [&snkVector, &srcRate, &setterOffset](i64 start, i64 end, i64 sinkRate, i64 delay) {
        u32 firing = 0;
        for (auto it = (snkVector.rbegin() + start); it < (snkVector.rbegin() + end); ++it) {
            /* == Adjust the values to match the actual position in the source vector == */
            auto snkLowerDep = setterOffset + pisdf::computeConsLowerDep(sinkRate, srcRate, firing, delay);
            auto snkUpperDep = setterOffset + pisdf::computeConsUpperDep(sinkRate, srcRate, firing, delay);
            (*it).lowerDep_ = static_cast<uint32_t>(snkLowerDep);
            (*it).upperDep_ = static_cast<uint32_t>(snkUpperDep);
            firing += 1;
        }
    };
    /* == Compute dependencies for sinks == */
    auto delay = edge->delay() ? edge->delay()->value() : 0;
    computeWorker(0, sinkRepetitionValue, snkRate, delay);
    /* == Compute dependencies for end / getter vertices == */
    const auto getterRate = edge->delay() ? snkVector[0].rate_ : 0;
    delay = delay - snkRate * sinkRepetitionValue;
    computeWorker(sinkRepetitionValue, static_cast<i64>(snkVector.size()), getterRate, delay);
    /* == Update source vector with proper dependencies == */
    u32 firing = 0;
    for (auto it = snkVector.rbegin(); it < snkVector.rend(); ++it) {
        const auto lowerIndex = srcVector.size() - 1 - (*it).lowerDep_;
        const auto upperIndex = srcVector.size() - 1 - (*it).upperDep_;
        srcVector[lowerIndex].lowerDep_ = std::min(srcVector[lowerIndex].lowerDep_, firing);
        srcVector[lowerIndex].upperDep_ = std::max(srcVector[lowerIndex].upperDep_, firing);
        srcVector[upperIndex].lowerDep_ = std::min(srcVector[upperIndex].lowerDep_, firing);
        srcVector[upperIndex].upperDep_ = std::max(srcVector[upperIndex].upperDep_, firing);
        firing += 1;
    }
}

template<class ConnectEdge>
void spider::srdag::detail::connectForkOrJoin(srdag::Vertex *vertex,
                                              spider::vector<TransfoVertex> &workingVector,
                                              spider::vector<TransfoVertex> &oppositeVector,
                                              const ConnectEdge &edgeConnector) {
    /* == Get current last element == */
    const auto &last = oppositeVector.back();
    oppositeVector.pop_back();
    /* == Number of edges == */
    const auto count = std::max(vertex->inputEdgeCount(), vertex->outputEdgeCount());
    /* == Total number of tokens == */
    auto rate = last.rate_;
    for (size_t i = 0; i < count - 1; ++i) {
        const auto &transfoVertex = workingVector.back();
        rate -= transfoVertex.rate_;
        edgeConnector(vertex, i, transfoVertex);
        workingVector.pop_back();
    }
    oppositeVector.emplace_back(rate, static_cast<uint32_t>(count - 1), vertex);
    oppositeVector.back().lowerDep_ = last.upperDep_;
    oppositeVector.back().upperDep_ = last.upperDep_;
}

void spider::srdag::detail::addForkVertex(spider::vector<TransfoVertex> &srcVector,
                                          spider::vector<TransfoVertex> &snkVector,
                                          srdag::Graph *srdag) {
    const auto &sourceLinker = srcVector.back();
    auto name = std::string("fork::").append(sourceLinker.vertex_->name()).append("::out::").append(
            std::to_string(sourceLinker.portIx_));
    auto *fork = srdag->createForkVertex(std::move(name), (sourceLinker.upperDep_ - sourceLinker.lowerDep_) + 1);

    /* == Create an edge between source and fork == */
    srdag->createEdge(sourceLinker.vertex_,  /* = Vertex that need to explode = */
                      sourceLinker.portIx_,  /* = Source port ix = */
                      fork,                  /* = Added fork = */
                      0,                     /* = Fork has only one input port so 0 is fixed = */
                      sourceLinker.rate_    /* = Sink rate is the same as the source rate = */);

    /* == Connect the output edges of the fork == */
    connectForkOrJoin(fork, snkVector, srcVector,
                      [](srdag::Vertex *vertex, size_t portIx, const TransfoVertex &transfoVertex) {
                          vertex->graph()->createEdge(vertex,                /* = Fork vertex = */
                                                      portIx,                /* = Fork output to connect = */
                                                      transfoVertex.vertex_, /* = Sink to connect to fork = */
                                                      transfoVertex.portIx_, /* = Sink port ix = */
                                                      transfoVertex.rate_    /* = Sink rate = */);
                      });
}

void spider::srdag::detail::addJoinVertex(spider::vector<TransfoVertex> &srcVector,
                                          spider::vector<TransfoVertex> &snkVector,
                                          srdag::Graph *srdag) {
    const auto &sinkLinker = snkVector.back();
    auto name = std::string("join::").append(sinkLinker.vertex_->name()).append("::in::").append(
            std::to_string(sinkLinker.portIx_));
    auto *join = srdag->createJoinVertex(std::move(name), (sinkLinker.upperDep_ - sinkLinker.lowerDep_) + 1);

    /* == Create an edge between join and sink == */
    srdag->createEdge(join,                /* = Added join = */
                      0,              /* = Join has only one output port so 0 is fixed = */
                      sinkLinker.vertex_,  /* = Vertex that need to implode = */
                      sinkLinker.portIx_,  /* = Sink port ix = */
                      sinkLinker.rate_     /* = Sink rate = */);

    /* == Connect the input edges of the join == */
    connectForkOrJoin(join, srcVector, snkVector,
                      [](srdag::Vertex *vertex, size_t portIx, const TransfoVertex &transfoVertex) {
                          transfoVertex.vertex_->graph()->createEdge(
                                  transfoVertex.vertex_, /* = Source to connect to join = */
                                  transfoVertex.portIx_, /* = Source port ix = */
                                  vertex,                /* = Join = */
                                  portIx,                /* = Join input to connect = */
                                  transfoVertex.rate_    /* = Source rate = */);

                      });
}

spider::srdag::detail::TransfoVertexVector
spider::srdag::detail::buildSinkLinkerVector(const pisdf::Edge *edge,
                                             const TransfoJob &job,
                                             srdag::Graph *srdag,
                                             const spider::vector<size_t> &ref2CloneVector) {
    /* == 0. Reserve size of the vector == */
    auto sinkVector = factory::vector<TransfoVertex>(StackID::TRANSFO);
    const auto *sink = edge->sink();
    const auto *delay = edge->delay();
    sinkVector.reserve(sink->repetitionValue() + (delay != nullptr));
    /* == 1. If delay, populate the getter clones in reverse order == */
    if (delay) {
        const auto &params = job.params_;
        if ((sink == edge->source()) &&
            delay->value() < edge->sinkRateExpression().evaluate(params)) {
            throwSpiderException("Insufficient delay [%"
                                         PRIu32
                                         "] on edge [%s].",
                                 delay->value(),
                                 edge->name().c_str());
        }
        const auto *delayVertex = delay->vertex();
        const auto *clone = srdag->vertex(ref2CloneVector[delayVertex->ix()]);
        auto *outputEdge = clone->outputEdge(0);
        if (outputEdge) {
            /* == 1.1 We already connected getter, we'll use it directly == */
            populateFromDelayVertex(sinkVector, outputEdge, true);
        } else {
            /* == 1.2 Connect to the clone == */
            populateTransfoVertexVector(sinkVector, delay->vertex(), delay->value(), 1, job, srdag, ref2CloneVector);
        }
    }
    /* == 2. Populate the rest of the sinkVector == */
    if (sink->subtype() == pisdf::VertexType::OUTPUT) {
        /* == 2.0 Check if we are in the trivial case of no interface == */
        auto *output = sink->convertTo<pisdf::Interface>();
        auto *srEdge = job.srdagInstance_->outputEdge(sink->ix());
        if (isInterfaceTransparent(job, output)) {
            sinkVector.emplace_back(srEdge->sinkRateValue(), srEdge->sinkPortIx(), srEdge->sink());
        } else {
            auto *tail = srdag->createTailVertex(job.srdagInstance_->name() + "::" + output->name(), 1);
            srEdge->setSource(tail, 0);
            const auto rate = edge->sourceRateExpression().evaluate(job.params_) * edge->source()->repetitionValue();
            sinkVector.emplace_back(rate, 0, tail);
        }
    } else if (sink->subtype() == pisdf::VertexType::DELAY) {
        const auto *clone = srdag->vertex(ref2CloneVector[getIx(sink, job.reference_)]);
        if (clone->outputEdge(1)) {
            /* == 2.1 We already connected sink of original edge containing the delay, we'll use it directly == */
            populateFromDelayVertex(sinkVector, clone->outputEdge(1), true);
        } else {
            const auto *delayEdge = sink->convertTo<pisdf::DelayVertex>()->delay()->edge();
            const auto isNullEdge = !(delayEdge->sourceRateExpression().evaluate(job.params_)) &&
                                    !(delayEdge->sinkRateExpression().evaluate(job.params_));
            if (isNullEdge && clone->outputEdge(0)) {
                /* == let set the setter as our source == */
                populateFromDelayVertex(sinkVector, clone->outputEdge(0), true);
            } else {
                const auto rate = edge->sinkRateExpression().evaluate(job.params_);
                populateTransfoVertexVector(sinkVector, sink, rate, edge->sinkPortIx(), job, srdag, ref2CloneVector);
            }
        }
    } else {
        /* == 2.2 Normal case == */
        const auto rate = edge->sinkRateExpression().evaluate(job.params_);
        populateTransfoVertexVector(sinkVector, sink, rate, edge->sinkPortIx(), job, srdag, ref2CloneVector);
    }
    return sinkVector;
}

spider::srdag::detail::TransfoVertexVector
spider::srdag::detail::buildSourceLinkerVector(const pisdf::Edge *edge,
                                               const TransfoJob &job,
                                               srdag::Graph *srdag,
                                               const spider::vector<size_t> &ref2CloneVector) {
    /* == 0. Reserve size of the vector == */
    auto srcVector = factory::vector<TransfoVertex>(StackID::TRANSFO);
    const auto *source = edge->source();
    const auto *delay = edge->delay();
    srcVector.reserve(source->repetitionValue() + (delay != nullptr));
    /* == 1. Populate the sourceVector == */
    if (source->subtype() == pisdf::VertexType::INPUT) {
        /* == 1.0 Check if we are in the trivial case of no interface == */
        auto *input = source->convertTo<pisdf::Interface>();
        auto *srEdge = job.srdagInstance_->inputEdge(source->ix());
        if (isInterfaceTransparent(job, input)) {
            srcVector.emplace_back(srEdge->sourceRateValue(), srEdge->sourcePortIx(), srEdge->source());
        } else {
            auto *repeat = srdag->createRepeatVertex(job.srdagInstance_->name() + "::" + input->name());
            srEdge->setSink(repeat, 0);
            const auto rate = edge->sinkRateExpression().evaluate(job.params_) * edge->sink()->repetitionValue();
            srcVector.emplace_back(rate, 0, repeat);
        }
    } else if (source->subtype() == pisdf::VertexType::DELAY) {
        const auto *clone = srdag->vertex(ref2CloneVector[getIx(source, job.reference_)]);
        if (clone->inputEdge(1)) {
            /* == 1.1 We already connected source of original edge containing the delay, we'll use it directly == */
            populateFromDelayVertex(srcVector, clone->inputEdge(1), false);
        } else {
            const auto *delayEdge = source->convertTo<pisdf::DelayVertex>()->delay()->edge();
            const auto isNullEdge = !(delayEdge->sourceRateExpression().evaluate(job.params_)) &&
                                    !(delayEdge->sinkRateExpression().evaluate(job.params_));
            if (isNullEdge && clone->inputEdge(0)) {
                /* == let set the setter as our source == */
                populateFromDelayVertex(srcVector, clone->inputEdge(0), false);
            } else {
                const auto rate = edge->sourceRateExpression().evaluate(job.params_);
                populateTransfoVertexVector(srcVector, source, rate, edge->sourcePortIx(), job, srdag, ref2CloneVector);
            }
        }
    } else {
        /* == 1.2 Normal case == */
        const auto rate = edge->sourceRateExpression().evaluate(job.params_);
        populateTransfoVertexVector(srcVector, source, rate, edge->sourcePortIx(), job, srdag, ref2CloneVector);
    }
    /* == 2. If delay, populate the setter clones in reverse order == */
    if (delay) {
        const auto *delayVertex = delay->vertex();
        const auto *delayClone = srdag->vertex(ref2CloneVector[delayVertex->ix()]);
        auto *inputEdge = delayClone->inputEdge(0);
        if (inputEdge) {
            /* == 2.1 We already connected setter, we'll use it directly == */
            populateFromDelayVertex(srcVector, inputEdge, false);
        } else {
            /* == 2.2 Connect to the clone == */
            populateTransfoVertexVector(srcVector, delay->vertex(), delay->value(), 1, job, srdag, ref2CloneVector);
        }
    }
    return srcVector;
}

bool spider::srdag::detail::isInterfaceTransparent(const TransfoJob &job, const pisdf::Interface *interface) {
    const auto *edge = interface->edge();
    const auto *vertex = interface->opposite();
    const auto sourceRate = edge->sourceRateExpression().evaluate(job.params_);
    const auto sinkRate = edge->sinkRateExpression().evaluate(job.params_);
    if (interface->subtype() == pisdf::VertexType::INPUT) {
        return (vertex->repetitionValue() * sinkRate) == sourceRate;
    }
    return (vertex->repetitionValue() * sourceRate) == sinkRate;
}

void spider::srdag::detail::populateTransfoVertexVector(spider::vector<TransfoVertex> &vector,
                                                        const pisdf::Vertex *reference,
                                                        i64 rate,
                                                        size_t portIx,
                                                        const TransfoJob &job,
                                                        srdag::Graph *srdag,
                                                        const spider::vector<size_t> &ref2CloneVector) {
    const auto *clone = srdag->vertex(ref2CloneVector[getIx(reference, job.reference_)]);
    const auto &cloneIx = clone->ix();
    for (auto ix = (cloneIx + reference->repetitionValue()); ix != cloneIx; --ix) {
        vector.emplace_back(rate, static_cast<uint32_t>(portIx), srdag->vertex(ix - 1));
    }
}

void spider::srdag::detail::populateFromDelayVertex(spider::vector<TransfoVertex> &vector,
                                                    srdag::Edge *edge,
                                                    bool isSink) {
    srdag::Vertex *vertex = nullptr;
    i64 rate;
    size_t portIx;
    if (isSink) {
        vertex = edge->sink();
        rate = edge->sourceRateValue();
        portIx = edge->sinkPortIx();
    } else {
        vertex = edge->source();
        rate = edge->sinkRateValue();
        portIx = edge->sourcePortIx();
    }
    vector.emplace_back(rate, static_cast<uint32_t>(portIx), vertex);
}

#endif
