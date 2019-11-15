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

#include <graphs-tools/transformation/srdag/SRDAGTransfoHelper.h>
#include <graphs-tools/transformation/srdag/SRDAGTransformation.h>
#include <spider-api/pisdf.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs/pisdf/params/InHeritedParam.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/interfaces/Interface.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/visitors/CloneVertexVisitor.h>
#include <graphs-tools/numerical/PiSDFAnalysis.h>

/* == Static function(s) === */

static std::string
buildCloneName(const PiSDFAbstractVertex *vertex, std::uint32_t instance, Spider::SRDAG::TransfoJob &transfoJob) {
    const auto *graphRef = transfoJob.job_.instanceValue_ == UINT32_MAX ?
                           transfoJob.job_.reference_ : transfoJob.srdag_->vertex(transfoJob.job_.srdagIx_);
    return graphRef->name() + "-" + vertex->name() + "_" + std::to_string(instance);
}

static inline std::uint32_t uniformIx(const PiSDFAbstractVertex *vertex, const PiSDFGraph *graph) {
    return vertex->ix() +
           ((vertex->type() == PiSDFVertexType::INTERFACE) * graph->vertexCount()) +
           ((vertex->subtype() == PiSDFVertexType::OUTPUT) * graph->edgesINCount());
}

static void cloneParams(Spider::SRDAG::Job &job, const PiSDFGraph *graph, const Spider::SRDAG::Job &parentJob) {
    for (const auto &param : graph->params()) {
        if (param->type() == Spider::PiSDF::ParamType::INHERITED) {
            const auto &inheritedParamIx = dynamic_cast<const PiSDFInHeritedParam *>(param->self())->parent()->ix();
            const auto &inheritedParam = parentJob.params_[inheritedParamIx];
            auto *p = Spider::API::createStaticParam(nullptr, param->name(), inheritedParam->value(), StackID::TRANSFO);
            job.params_.push_back(p);
        } else if (!param->dynamic()) {
            job.params_.push_back(param);
        } else {
            auto *p = Spider::API::createDynamicParam(nullptr, param->name(), "", StackID::TRANSFO);
            job.params_.push_back(p);
        }
    }
}

static std::uint32_t cloneVertex(PiSDFAbstractVertex *vertex, Spider::SRDAG::TransfoJob &transfoJob) {
    Spider::PiSDF::CloneVertexVisitor cloneVisitor{ transfoJob.srdag_, StackID::TRANSFO };
    for (std::uint32_t it = 0; it < vertex->repetitionValue(); ++it) {
        vertex->visit(&cloneVisitor);

        /* == Change the name of the clone == */
        auto *clone = transfoJob.srdag_->vertices().back();
        clone->setName(buildCloneName(vertex, it, transfoJob));
    }
    return (transfoJob.srdag_->vertexCount() - 1) - (vertex->repetitionValue() - 1);
}

static std::uint32_t cloneGraph(const PiSDFGraph *graph, Spider::SRDAG::TransfoJob &linker) {
    /* == Clone the vertex == */
    std::uint32_t ix = 0;
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
    const auto &runGraphSubIx = linker.init2dynamic_[graph->subIx()];
    if (runGraphSubIx != UINT32_MAX) {
        auto *runGraph = graph->containingGraph()->subgraphs()[runGraphSubIx];

        /* == Find the first job corresponding to the init graph == */
        auto it = linker.dynaJobs_.begin();
        while ((it != linker.dynaJobs_.end()) && ((*it).reference_ != runGraph)) { it++; }
        if (it == linker.dynaJobs_.end()) {
            /* == Seems like run counter part of the graph has not been cloned yet == */
            const auto &offset = linker.dynaJobs_.size();
            linker.tracker_[runGraph->ix()] = cloneGraph(runGraph, linker);
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
        for (auto srdagIx = ix + graph->repetitionValue() - 1; srdagIx >= ix; --srdagIx) {
            jobStack.emplace_back(graph, linker.srdag_->vertex(srdagIx)->ix(), srdagIx - ix);

            /* == Copy the params == */
            cloneParams(jobStack.back(), graph, linker.job_);
        }
    }
    return ix;
}

/* === Function(s) definition === */

PiSDFAbstractVertex *Spider::SRDAG::fetchOrClone(PiSDFAbstractVertex *vertex, TransfoJob &transfoJob) {
    if (!vertex) {
        throwSpiderException("Trying to clone nullptr vertex.");
    }
    const auto &vertexUniformIx = uniformIx(vertex, transfoJob.job_.reference_);

    /* == If vertex has already been cloned return the first one == */
    if (transfoJob.tracker_[vertexUniformIx] == UINT32_MAX) {
        if (vertex->subtype() == PiSDFVertexType::GRAPH) {
            /* == Clone the graph N times and create the different jobs == */
            transfoJob.tracker_[vertexUniformIx] = cloneGraph(dynamic_cast<const PiSDFGraph *>(vertex), transfoJob);
        } else {
            /* == Clone the vertex N times and return the first one == */
            transfoJob.tracker_[vertexUniformIx] = cloneVertex(vertex, transfoJob);
        }
    }
    return transfoJob.srdag_->vertex(transfoJob.tracker_[vertexUniformIx]);
}

void Spider::SRDAG::fillLinkerVector(TransfoStack &vector,
                                     PiSDFAbstractVertex *reference,
                                     std::int64_t rate,
                                     std::uint32_t portIx,
                                     TransfoJob &transfoJob) {
    const auto &clone = fetchOrClone(reference, transfoJob);
    const auto &cloneIx = clone->ix();
    for (auto i = (cloneIx + reference->repetitionValue()); i != cloneIx; --i) {
        vector.emplace_back(rate, portIx, transfoJob.srdag_->vertex(i - 1));
    }
}

void Spider::SRDAG::addForkVertex(TransfoStack &srcVector, TransfoStack &snkVector, PiSDFGraph *srdag) {
    const auto &sourceLinker = srcVector.back();
    auto *fork = Spider::API::createFork(srdag,
                                         "fork-" + sourceLinker.vertex_->name() + "_out-" +
                                         std::to_string(sourceLinker.portIx_),
                                         (sourceLinker.upperDep_ - sourceLinker.lowerDep_) + 1,
                                         StackID::TRANSFO);

    /* == Create an edge between source and fork == */
    Spider::API::createEdge(sourceLinker.vertex_,  /* = Vertex that need to explode = */
                            sourceLinker.portIx_,  /* = Source port ix = */
                            sourceLinker.rate_,    /* = Source rate = */
                            fork,                  /* = Added fork = */
                            0,                     /* = Fork has only one input port so 0 is fixed = */
                            sourceLinker.rate_,    /* = Sink rate is the same as the source rate = */
                            StackID::TRANSFO);
    srcVector.pop_back();

    /* == Connect out of fork == */
    auto remaining = sourceLinker.rate_;
    for (std::uint32_t i = 0; i < fork->edgesOUTCount() - 1; ++i) {
        const auto &sinkLinker = snkVector.back();
        remaining -= sinkLinker.rate_;
        Spider::API::createEdge(fork,               /* = Fork vertex = */
                                i,                  /* = Fork output to connect = */
                                sinkLinker.rate_,   /* = Sink rate = */
                                sinkLinker.vertex_, /* = Sink to connect to fork = */
                                sinkLinker.portIx_, /* = Sink port ix = */
                                sinkLinker.rate_,   /* = Sink rate = */
                                StackID::TRANSFO);
        snkVector.pop_back();
    }
    srcVector.emplace_back(remaining, fork->edgesOUTCount() - 1, fork);
    srcVector.back().lowerDep_ = sourceLinker.upperDep_;
    srcVector.back().upperDep_ = sourceLinker.upperDep_;
}

void Spider::SRDAG::addJoinVertex(TransfoStack &srcVector, TransfoStack &snkVector, PiSDFGraph *srdag) {
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

void Spider::SRDAG::replaceJobInterfaces(TransfoJob &transfoJob) {
    if (!transfoJob.job_.reference_->edgesINCount() &&
        !transfoJob.job_.reference_->edgesOUTCount()) {
        return;
    }
    auto *srdagInstance = transfoJob.srdag_->vertex(transfoJob.job_.srdagIx_);
    if (!srdagInstance) {
        throwSpiderException("could not find matching single rate instance [%"
                                     PRIu32
                                     "] of graph [%s]", transfoJob.job_.instanceValue_,
                             transfoJob.job_.reference_->name().c_str());
    }

    /* == Replace the input interfaces == */
    for (const auto &interface : transfoJob.job_.reference_->inputInterfaceArray()) {
        auto *edge = srdagInstance->inputEdge(interface->ix());
        auto *vertex = Spider::API::createRepeat(transfoJob.srdag_,
                                                 srdagInstance->name() + "_" + interface->name(),
                                                 StackID::TRANSFO);
        edge->setSink(vertex, 0, Expression(edge->sinkRateExpression()));
        transfoJob.tracker_[uniformIx(interface, transfoJob.job_.reference_)] = vertex->ix();
    }

    /* == Replace the output interfaces == */
    for (const auto &interface : transfoJob.job_.reference_->outputInterfaceArray()) {
        auto *edge = srdagInstance->outputEdge(interface->ix());
        auto *vertex = Spider::API::createTail(transfoJob.srdag_,
                                               srdagInstance->name() + "_" + interface->name(),
                                               1,
                                               StackID::TRANSFO);
        edge->setSource(vertex, 0, Expression(edge->sourceRateExpression()));
        transfoJob.tracker_[uniformIx(interface, transfoJob.job_.reference_)] = vertex->ix();
    }
}

void Spider::SRDAG::computeEdgeDependencies(TransfoStack &srcVector, TransfoStack &snkVector, TransfoJob &transfoJob) {
    const auto &edge = transfoJob.edge_;
    auto &&delay = edge->delay() ? edge->delay()->value(transfoJob.job_.params_) : 0;
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
