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

/* === Include(s) === */

#include <graphs-tools/transformation/srdag/SingleRateTransformer.h>
#include <graphs-tools/transformation/srdag/visitors/SRDAGCopyVertexVisitor.h>
#include <graphs-tools/numerical/brv.h>
#include <graphs-tools/numerical/dependencies.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/srdag/SRDAGGraph.h>
#include <graphs/srdag/SRDAGEdge.h>
#include <graphs/srdag/SRDAGVertex.h>
#include <api/pisdf-api.h>
#include <api/runtime-api.h>

/* === Static function(s) === */

size_t spider::srdag::SingleRateTransformer::getIx(const pisdf::Vertex *vertex, const pisdf::Graph *graph) {
    if (vertex->subtype() == pisdf::VertexType::INPUT) {
        return vertex->ix() + graph->vertexCount();
    } else if (vertex->subtype() == pisdf::VertexType::OUTPUT) {
        return vertex->ix() + graph->vertexCount() + graph->inputEdgeCount();
    }
    return vertex->ix();
}

void spider::srdag::SingleRateTransformer::updateParams(TransfoJob &job) {
    const auto *graph = job.reference_;
    if (!graph->configVertexCount()) {
        for (auto &param : job.params_) {
            if (param->type() == pisdf::ParamType::INHERITED) {
                const auto value = param->parent()->value(job.params_);
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

bool
spider::srdag::SingleRateTransformer::isInterfaceTransparent(const TransfoJob &job, const pisdf::Interface *interface) {
    const auto *edge = interface->edge();
    const auto *vertex = interface->opposite();
    const auto sourceRate = edge->sourceRateExpression().evaluate(job.params_);
    const auto sinkRate = edge->sinkRateExpression().evaluate(job.params_);
    if (interface->subtype() == pisdf::VertexType::INPUT) {
        return (vertex->repetitionValue() * sinkRate) == sourceRate;
    }
    return (vertex->repetitionValue() * sourceRate) == sinkRate;
}

/* === Method(s) implementation === */

spider::srdag::SingleRateTransformer::SingleRateTransformer(TransfoJob &job, srdag::Graph *srdag) :
        ref2Clone_{ factory::vector<size_t>(StackID::TRANSFO) },
        job_{ job },
        srdag_{ srdag } {

    /* == 0. Set dynamic dependent parameter values == */
    updateParams(job_);

    /* == 1. Compute the repetition vector == */
    const auto *graph = job.reference_;
    if (job_.firingValue_ == 0) {
        brv::compute(graph, job_.params_);
    }
    const auto vertexCount = graph->vertexCount() + graph->inputEdgeCount() + graph->outputEdgeCount();
    ref2Clone_ = factory::vector<size_t>(vertexCount, SIZE_MAX, StackID::TRANSFO);
}

std::pair<spider::srdag::JobStack, spider::srdag::JobStack> spider::srdag::SingleRateTransformer::execute() {
    /* == 1. Copy the vertex accordingly to their repetition value == */
    spider::vector<srdag::Vertex *> delayVertexToRemove;
    SRDAGCopyVertexVisitor visitor{ job_, srdag_ };
    for (const auto &vertex : job_.reference_->vertices()) {
        vertex->visit(&visitor);
        ref2Clone_[getIx(vertex.get(), job_.reference_)] = visitor.ix_;
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
    srdag_->removeVertex(job_.srdagInstance_);
    job_.srdagInstance_ = nullptr;

    /* == 4. Remove the delay vertex added for the transformation == */
    for (const auto &vertex : delayVertexToRemove) {
        srdag_->removeVertex(vertex);
    }
    return futureJobs;
}

/* === Private method(s) implementation === */

std::pair<spider::srdag::JobStack, spider::srdag::JobStack> spider::srdag::SingleRateTransformer::makeFutureJobs() {
    auto staticJobStack = factory::vector<TransfoJob>(StackID::TRANSFO);
    auto dynaJobStack = factory::vector<TransfoJob>(StackID::TRANSFO);

    /* == Creates future TransfoJob == */
    const auto isDynamic = job_.reference_->dynamic();
    for (auto *subgraph : job_.reference_->subgraphs()) {
        const auto &params = subgraph->params();
        const auto firstCloneIx = ref2Clone_[subgraph->ix()];
        auto &stack = isDynamic ? dynaJobStack : staticJobStack;
        for (auto ix = firstCloneIx; ix < firstCloneIx + subgraph->repetitionValue(); ++ix) {
            auto job = TransfoJob{ subgraph, srdag_->vertex(ix), static_cast<u32>((ix - firstCloneIx)) };
            /* == Copy the params == */
            job.params_.reserve(params.size());
            for (const auto &param : params) {
                job.params_.emplace_back(copyParameter(param));
            }
            stack.emplace_back(std::move(job));
        }
    }

    /* == Update reference of config vertices parameters == */
    for (const auto &cfg : job_.reference_->configVertices()) {
        auto *clone = srdag_->vertex(ref2Clone_[cfg->ix()]);
        for (const auto ix : cfg->outputParamIxVector()) {
            clone->addOutputParameter(job_.params_[ix]);
        }
    }

    return std::make_pair(std::move(staticJobStack), std::move(dynaJobStack));
}

std::shared_ptr<spider::pisdf::Param>
spider::srdag::SingleRateTransformer::copyParameter(const std::shared_ptr<pisdf::Param> &param) const {
    if (param->dynamic()) {
        std::shared_ptr<pisdf::Param> p;
        if (param->type() == pisdf::ParamType::INHERITED) {
            const auto &parentParam = job_.params_[param->parent()->ix()];
            p = spider::make_shared<pisdf::Param, StackID::TRANSFO>(param->name(), parentParam);
        } else {
            p = spider::make_shared<pisdf::Param, StackID::TRANSFO>(*param);
        }
        p->setIx(param->ix());
        return p;
    }
    return param;
}

bool spider::srdag::SingleRateTransformer::checkForNullEdge(const pisdf::Edge *edge) {
    bool isNullEdge = !(edge->sourceRateExpression().evaluate(job_.params_)) &&
                      !(edge->sinkRateExpression().evaluate(job_.params_));
    if (isNullEdge) {
        /* == Add an empty INIT to the sink == */
        const auto *sink = edge->sink();
        if (sink->repetitionValue()) {
            auto start = ref2Clone_[getIx(sink, job_.reference_)];
            for (auto i = start; i < start + sink->repetitionValue(); ++i) {
                auto *clone = srdag_->vertex(i);
                auto name = std::string("void::in::").append(clone->name()).append(":").append(
                        std::to_string(edge->sinkPortIx()));
                auto *init = srdag_->createVoidVertex(std::move(name), 0, 1);
                srdag_->createEdge(init, 0, 0, clone, edge->sinkPortIx(), 0);
            }
        }
        /* == Add an empty END to the source == */
        const auto *source = edge->source();
        if (source->repetitionValue()) {
            auto start = ref2Clone_[getIx(source, job_.reference_)];
            for (auto i = start; i < start + source->repetitionValue(); ++i) {
                auto *clone = srdag_->vertex(i);
                auto name = std::string("void::out::").append(clone->name()).append(":").append(
                        std::to_string(edge->sourcePortIx()));
                auto *end = srdag_->createVoidVertex(std::move(name), 1, 0);
                srdag_->createEdge(clone, edge->sourcePortIx(), 0, end, 0, 0);
            }
        }
        return true;
    }
    return false;
}

void spider::srdag::SingleRateTransformer::singleRateLinkage(pisdf::Edge *edge) {
    if ((edge->source() == edge->sink()) && !edge->delay()) {
        throwSpiderException("No delay on edge with self loop.");
    }
    /* == Check for null edge == */
    if (checkForNullEdge(edge)) {
        return;
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
                srdag_->createEdge(srcLnk.vertex_, srcLnk.portIx_, srcLnk.rate_,
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

void spider::srdag::SingleRateTransformer::computeDependencies(const pisdf::Edge *edge,
                                                               vector<TransfoVertex> &srcVector,
                                                               vector<TransfoVertex> &snkVector) {
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
void spider::srdag::SingleRateTransformer::connectForkOrJoin(srdag::Vertex *vertex,
                                                             vector<TransfoVertex> &workingVector,
                                                             vector<TransfoVertex> &oppositeVector,
                                                             const ConnectEdge &edgeConnector) const {
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

void spider::srdag::SingleRateTransformer::addForkVertex(vector<TransfoVertex> &srcVector,
                                                         vector<TransfoVertex> &snkVector) {
    const auto &sourceLinker = srcVector.back();
    auto name = std::string("fork::").append(sourceLinker.vertex_->name()).append("::out::").append(
            std::to_string(sourceLinker.portIx_));
    auto *fork = srdag_->createForkVertex(std::move(name), (sourceLinker.upperDep_ - sourceLinker.lowerDep_) + 1);

    /* == Create an edge between source and fork == */
    srdag_->createEdge(sourceLinker.vertex_,  /* = Vertex that need to explode = */
                       sourceLinker.portIx_,  /* = Source port ix = */
                       sourceLinker.rate_,    /* = Source rate = */
                       fork,                  /* = Added fork = */
                       0,                     /* = Fork has only one input port so 0 is fixed = */
                       sourceLinker.rate_    /* = Sink rate is the same as the source rate = */);

    /* == Connect the output edges of the fork == */
    connectForkOrJoin(fork, snkVector, srcVector,
                      [](srdag::Vertex *vertex, size_t portIx, const TransfoVertex &transfoVertex) {
                          vertex->graph()->createEdge(vertex,                /* = Fork vertex = */
                                                      portIx,                /* = Fork output to connect = */
                                                      transfoVertex.rate_,   /* = Sink rate = */
                                                      transfoVertex.vertex_, /* = Sink to connect to fork = */
                                                      transfoVertex.portIx_, /* = Sink port ix = */
                                                      transfoVertex.rate_    /* = Sink rate = */);
                      });
}

void spider::srdag::SingleRateTransformer::addJoinVertex(vector<TransfoVertex> &srcVector,
                                                         vector<TransfoVertex> &snkVector) {
    const auto &sinkLinker = snkVector.back();
    auto name = std::string("join::").append(sinkLinker.vertex_->name()).append("::in::").append(
            std::to_string(sinkLinker.portIx_));
    auto *join = srdag_->createJoinVertex(std::move(name), (sinkLinker.upperDep_ - sinkLinker.lowerDep_) + 1);

    /* == Create an edge between join and sink == */
    srdag_->createEdge(join,                /* = Added join = */
                       0,              /* = Join has only one output port so 0 is fixed = */
                       sinkLinker.rate_,    /* = Source rate is the same as the sink rate = */
                       sinkLinker.vertex_,  /* = Vertex that need to implode = */
                       sinkLinker.portIx_,  /* = Sink port ix = */
                       sinkLinker.rate_     /* = Sink rate = */);

    /* == Connect the input edges of the join == */
    connectForkOrJoin(join, srcVector, snkVector,
                      [](srdag::Vertex *vertex, size_t portIx, const TransfoVertex &transfoVertex) {
                          transfoVertex.vertex_->graph()->createEdge(
                                  transfoVertex.vertex_, /* = Source to connect to join = */
                                  transfoVertex.portIx_, /* = Source port ix = */
                                  transfoVertex.rate_,   /* = Source rate = */
                                  vertex,                /* = Join = */
                                  portIx,                /* = Join input to connect = */
                                  transfoVertex.rate_    /* = Source rate = */);

                      });
}


void spider::srdag::SingleRateTransformer::populateTransfoVertexVector(vector<TransfoVertex> &vector,
                                                                       const pisdf::Vertex *reference,
                                                                       int64_t rate,
                                                                       size_t portIx) const {
    const auto *clone = srdag_->vertex(ref2Clone_[getIx(reference, job_.reference_)]);
    const auto &cloneIx = clone->ix();
    for (auto ix = (cloneIx + reference->repetitionValue()); ix != cloneIx; --ix) {
        vector.emplace_back(rate, static_cast<uint32_t>(portIx), srdag_->vertex(ix - 1));
    }
}

spider::srdag::SingleRateTransformer::TransfoVertexVector
spider::srdag::SingleRateTransformer::buildSinkLinkerVector(const pisdf::Edge *edge) {
    /* == 0. Reserve size of the vector == */
    auto sinkVector = factory::vector<TransfoVertex>(StackID::TRANSFO);
    const auto *sink = edge->sink();
    const auto *delay = edge->delay();
    sinkVector.reserve(sink->repetitionValue() + (delay != nullptr));

    /* == 1. If delay, populate the getter clones in reverse order == */
    if (delay) {
        const auto &params = job_.params_;
        if ((sink == edge->source()) &&
            delay->value() < edge->sinkRateExpression().evaluate(params)) {
            throwSpiderException("Insufficient delay [%"
                                         PRIu32
                                         "] on edge [%s].",
                                 delay->value(),
                                 edge->name().c_str());
        }
        const auto *delayVertex = delay->vertex();
        const auto *clone = srdag_->vertex(ref2Clone_[delayVertex->ix()]);
        auto *outputEdge = clone->outputEdge(0);
        if (outputEdge) {
            /* == 1.1 We already connected getter, we'll use it directly == */
            populateFromDelayVertex(sinkVector, outputEdge, true);
        } else {
            /* == 1.2 Connect to the clone == */
            populateTransfoVertexVector(sinkVector, delay->vertex(), delay->value(), 1);
        }
    }

    /* == 2. Populate the rest of the sinkVector == */
    if (sink->subtype() == pisdf::VertexType::OUTPUT) {
        /* == 2.0 Check if we are in the trivial case of no interface == */
        auto *output = sink->convertTo<pisdf::Interface>();
        auto *srEdge = job_.srdagInstance_->outputEdge(sink->ix());
        if (isInterfaceTransparent(job_, output) && srEdge->sink()->scheduleTaskIx() == SIZE_MAX) {
            sinkVector.emplace_back(srEdge->sinkRateValue(), srEdge->sinkPortIx(), srEdge->sink());
            srdag_->removeEdge(srEdge);
        } else {
            auto *tail = srdag_->createTailVertex(job_.srdagInstance_->name() + "::" + job_.srdagInstance_->name(), 1);
            srEdge->setSource(tail, 0, edge->sourceRateValue());
            const auto rate = edge->sourceRateExpression().evaluate(job_.params_) * edge->source()->repetitionValue();
            sinkVector.emplace_back(rate, 0, tail);
        }
    } else if (sink->subtype() == pisdf::VertexType::DELAY) {
        const auto *clone = srdag_->vertex(ref2Clone_[getIx(sink, job_.reference_)]);
        if (clone->outputEdge(1)) {
            /* == 2.1 We already connected sink of original edge containing the delay, we'll use it directly == */
            populateFromDelayVertex(sinkVector, clone->outputEdge(1), true);
        } else {
            const auto *delayEdge = sink->convertTo<pisdf::DelayVertex>()->delay()->edge();
            const auto isNullEdge = !(delayEdge->sourceRateExpression().evaluate(job_.params_)) &&
                                    !(delayEdge->sinkRateExpression().evaluate(job_.params_));
            if (isNullEdge && clone->outputEdge(0)) {
                /* == let set the setter as our source == */
                populateFromDelayVertex(sinkVector, clone->outputEdge(0), true);
            } else {
                const auto rate = edge->sinkRateExpression().evaluate(job_.params_);
                populateTransfoVertexVector(sinkVector, sink, rate, edge->sinkPortIx());
            }
        }
    } else {
        /* == 2.2 Normal case == */
        const auto rate = edge->sinkRateExpression().evaluate(job_.params_);
        populateTransfoVertexVector(sinkVector, sink, rate, edge->sinkPortIx());
    }
    return sinkVector;
}

spider::srdag::SingleRateTransformer::TransfoVertexVector
spider::srdag::SingleRateTransformer::buildSourceLinkerVector(const pisdf::Edge *edge) {
    /* == 0. Reserve size of the vector == */
    auto sourceVector = factory::vector<TransfoVertex>(StackID::TRANSFO);
    const auto *source = edge->source();
    const auto *delay = edge->delay();
    sourceVector.reserve(source->repetitionValue() + (delay != nullptr));

    /* == 1. Populate the sourceVector == */
    if (source->subtype() == pisdf::VertexType::INPUT) {
        /* == 1.0 Check if we are in the trivial case of no interface == */
        auto *input = source->convertTo<pisdf::Interface>();
        auto *srEdge = job_.srdagInstance_->inputEdge(source->ix());
        if (isInterfaceTransparent(job_, input) && srEdge->source()->scheduleTaskIx() == SIZE_MAX) {
            sourceVector.emplace_back(srEdge->sourceRateValue(), srEdge->sourcePortIx(), srEdge->source());
            srdag_->removeEdge(srEdge);
        } else {
            auto *repeat = srdag_->createRepeatVertex(job_.srdagInstance_->name() + "::" + job_.srdagInstance_->name());
            srEdge->setSink(repeat, 0, srEdge->sinkRateValue());
            const auto rate = edge->sinkRateExpression().evaluate(job_.params_) * edge->sink()->repetitionValue();
            sourceVector.emplace_back(rate, 0, repeat);
        }
    } else if (source->subtype() == pisdf::VertexType::DELAY) {
        const auto *clone = srdag_->vertex(ref2Clone_[getIx(source, job_.reference_)]);
        if (clone->inputEdge(1)) {
            /* == 1.1 We already connected source of original edge containing the delay, we'll use it directly == */
            populateFromDelayVertex(sourceVector, clone->inputEdge(1), false);
        } else {
            const auto *delayEdge = source->convertTo<pisdf::DelayVertex>()->delay()->edge();
            const auto isNullEdge = !(delayEdge->sourceRateExpression().evaluate(job_.params_)) &&
                                    !(delayEdge->sinkRateExpression().evaluate(job_.params_));
            if (isNullEdge && clone->inputEdge(0)) {
                /* == let set the setter as our source == */
                populateFromDelayVertex(sourceVector, clone->inputEdge(0), false);
            } else {
                const auto rate = edge->sourceRateExpression().evaluate(job_.params_);
                populateTransfoVertexVector(sourceVector, source, rate, edge->sourcePortIx());
            }
        }
    } else {
        /* == 1.2 Normal case == */
        const auto rate = edge->sourceRateExpression().evaluate(job_.params_);
        populateTransfoVertexVector(sourceVector, source, rate, edge->sourcePortIx());
    }

    /* == 2. If delay, populate the setter clones in reverse order == */
    if (delay) {
        const auto *delayVertex = delay->vertex();
        const auto *delayClone = srdag_->vertex(ref2Clone_[delayVertex->ix()]);
        auto *inputEdge = delayClone->inputEdge(0);
        if (inputEdge) {
            /* == 2.1 We already connected setter, we'll use it directly == */
            populateFromDelayVertex(sourceVector, inputEdge, false);
        } else {
            /* == 2.2 Connect to the clone == */
            populateTransfoVertexVector(sourceVector, delay->vertex(), delay->value(), 1);
        }
    }

    return sourceVector;
}

void spider::srdag::SingleRateTransformer::populateFromDelayVertex(vector<TransfoVertex> &vector,
                                                                   srdag::Edge *edge,
                                                                   bool isSink) {
    srdag::Vertex *vertex = nullptr;
    int64_t rate;
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
    /* == Remove the Edge == */
    srdag_->removeEdge(edge);
}

#endif
