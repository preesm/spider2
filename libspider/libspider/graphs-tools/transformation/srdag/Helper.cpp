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

#include <graphs-tools/transformation/srdag/Helper.h>
#include <graphs-tools/transformation/srdag/Transformation.h>
#include <spider-api/pisdf.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs/pisdf/params/InHeritedParam.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/interfaces/Interface.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/visitors/CloneVertexVisitor.h>
#include <graphs-tools/numerical/PiSDFAnalysis.h>

/* === Visitor(s) === */

struct CopyParamVisitor final : public spider::pisdf::DefaultVisitor {
    explicit CopyParamVisitor(spider::vector<PiSDFParam *> &paramVector,
                              const spider::vector<PiSDFParam *> &parentParamVector) : paramVector_{ paramVector },
                                                                                       parentParamVector_{
                                                                                               parentParamVector } { }

    inline void visit(spider::pisdf::Param *param) override {
        paramVector_.push_back(param);
    }

    inline void visit(spider::pisdf::DynamicParam *param) override {
        auto *p = spider::allocate<PiSDFDynamicParam>(StackID::TRANSFO);
        spider::construct(p, param->name(), nullptr, Expression(param->expression()));
        paramVector_.push_back(p);
    }

    inline void visit(spider::pisdf::InHeritedParam *param) override {
        const auto &inheritedParam = parentParamVector_[param->parent()->ix()];
        auto *p = spider::api::createStaticParam(nullptr, param->name(), inheritedParam->value(), StackID::TRANSFO);
        paramVector_.push_back(p);
    }

    spider::vector<PiSDFParam *> &paramVector_;
    const spider::vector<PiSDFParam *> &parentParamVector_;
};

struct CloneVisitor final : public spider::pisdf::DefaultVisitor {

    explicit CloneVisitor(spider::srdag::TransfoJob &transfoJob) : transfoJob_{ transfoJob } { }

    inline void visit(spider::pisdf::DelayVertex *) override { }

    inline void visit(spider::pisdf::ExecVertex *vertex) override {
        spider::pisdf::CloneVertexVisitor cloneVisitor{ transfoJob_.srdag_, StackID::TRANSFO };
        for (std::uint32_t it = 0; it < vertex->repetitionValue(); ++it) {
            /* == Change the name of the clone == */
            vertex->visit(&cloneVisitor);
            auto *clone = transfoJob_.srdag_->vertices().back();
            clone->setName(buildCloneName(vertex, it, transfoJob_));
        }
        ix_ = (transfoJob_.srdag_->vertexCount() - 1) - (vertex->repetitionValue() - 1);
    }

    inline void visit(spider::pisdf::Graph *graph) override {
        /* == Clone the vertex == */
        ix_ = 0;
        for (std::uint32_t it = 0; it < graph->repetitionValue(); ++it) {
            const auto *clone = spider::api::createVertex(transfoJob_.srdag_,
                                                          buildCloneName(graph, it, transfoJob_),
                                                          graph->inputEdgeCount(),
                                                          graph->outputEdgeCount(),
                                                          StackID::TRANSFO);
            ix_ = clone->ix();
        }
        ix_ = ix_ - (graph->repetitionValue() - 1);

        /* == Push the jobs == */
        const auto &runGraphSubIx = transfoJob_.init2dynamic_[graph->subIx()];
        if (runGraphSubIx != UINT32_MAX) {
            auto *runGraph = graph->containingGraph()->subgraphs()[runGraphSubIx];

            /* == Find the first job corresponding to the init graph == */
            auto it = transfoJob_.dynaJobs_.begin();
            while ((it != transfoJob_.dynaJobs_.end()) && ((*it).reference_ != runGraph)) { it++; }
            if (it == transfoJob_.dynaJobs_.end()) {
                /* == Seems like run counter part of the graph has not been cloned yet == */
                const auto &offset = transfoJob_.dynaJobs_.size();
                CloneVisitor visitor{ transfoJob_ };
                runGraph->visit(&visitor);
                transfoJob_.tracker_[runGraph->ix()] = visitor.ix_;
                it = transfoJob_.dynaJobs_.begin() + offset;
                if (it == transfoJob_.dynaJobs_.end()) {
                    throwSpiderException("Init graph [%s] did not find run counter part [%s].",
                                         graph->name().c_str(),
                                         runGraph->name().c_str());
                }
            }

            /* == Push the jobs == */
            for (auto srdagIx = ix_; srdagIx < ix_ + graph->repetitionValue(); ++srdagIx) {
                transfoJob_.nextJobs_.emplace_back(graph, transfoJob_.srdag_->vertex(srdagIx)->ix(), srdagIx - ix_);
                transfoJob_.nextJobs_.back().params_.reserve(runGraph->paramCount());

                /* == Copy the params pointer == */
                for (auto &param : (*it).params_) {
                    transfoJob_.nextJobs_.back().params_.emplace_back(param);
                }
                it++;
            }
        } else {
            auto &jobStack = graph->dynamic() ? transfoJob_.dynaJobs_ : transfoJob_.nextJobs_;
            for (auto srdagIx = ix_ + graph->repetitionValue() - 1; srdagIx >= ix_; --srdagIx) {
                jobStack.emplace_back(graph, transfoJob_.srdag_->vertex(srdagIx)->ix(), srdagIx - ix_);

                /* == Copy the params == */
                CopyParamVisitor cpyVisitor{ jobStack.back().params_, transfoJob_.job_.params_ };
                for (const auto &param : graph->params()) {
                    param->visit(&cpyVisitor);
                }
            }
        }
    }

    spider::srdag::TransfoJob &transfoJob_;
    std::uint32_t ix_ = UINT32_MAX;

private:
    std::string buildCloneName(const PiSDFAbstractVertex *vertex,
                               std::uint32_t instance,
                               spider::srdag::TransfoJob &transfoJob) {
        const auto *graphRef = transfoJob.job_.instanceValue_ == UINT32_MAX ?
                               transfoJob.job_.reference_ : transfoJob.srdag_->vertex(transfoJob.job_.srdagIx_);
        return graphRef->name() + "-" + vertex->name() + "_" + std::to_string(instance);
    }
};

/* == Static function(s) === */

static inline std::uint32_t uniformIx(const PiSDFAbstractVertex *vertex, const PiSDFGraph *graph) {
    if (vertex->subtype() == PiSDFVertexType::INPUT) {
        return vertex->ix() + graph->vertexCount();
    } else if (vertex->subtype() == PiSDFVertexType::OUTPUT) {
        return vertex->ix() + graph->vertexCount() + graph->inputEdgeCount();
    }
    return vertex->ix();
}

/* === Function(s) definition === */

PiSDFAbstractVertex *spider::srdag::fetchOrClone(PiSDFAbstractVertex *vertex, TransfoJob &transfoJob) {
    if (!vertex) {
        throwSpiderException("Trying to clone nullptr vertex.");
    }
    const auto &vertexUniformIx = uniformIx(vertex, transfoJob.job_.reference_);

    /* == If vertex has already been cloned return the first one == */
    auto &indexRef = transfoJob.tracker_[vertexUniformIx];
    if (indexRef == UINT32_MAX) {
        CloneVisitor visitor{ transfoJob };
        vertex->visit(&visitor);
        indexRef = visitor.ix_;
    }
    return indexRef == UINT32_MAX ? nullptr : transfoJob.srdag_->vertex(indexRef);
}

void spider::srdag::fillLinkerVector(TransfoStack &vector,
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

void spider::srdag::addForkVertex(TransfoStack &srcVector, TransfoStack &snkVector, PiSDFGraph *srdag) {
    const auto &sourceLinker = srcVector.back();
    auto *fork = spider::api::createFork(srdag,
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
    for (std::uint32_t i = 0; i < fork->outputEdgeCount() - 1; ++i) {
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
    srcVector.emplace_back(remaining, fork->outputEdgeCount() - 1, fork);
    srcVector.back().lowerDep_ = sourceLinker.upperDep_;
    srcVector.back().upperDep_ = sourceLinker.upperDep_;
}

void spider::srdag::addJoinVertex(TransfoStack &srcVector, TransfoStack &snkVector, PiSDFGraph *srdag) {
    const auto &sinkLinker = snkVector.back();
    auto *join = spider::api::createJoin(srdag,
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
    for (std::uint32_t i = 0; i < join->inputEdgeCount() - 1; ++i) {
        const auto &sourceLinker = srcVector.back();
        remaining -= sourceLinker.rate_;
        spider::api::createEdge(sourceLinker.vertex_, sourceLinker.portIx_, sourceLinker.rate_,
                                join, i, sourceLinker.rate_, StackID::TRANSFO);
        srcVector.pop_back();
    }
    snkVector.emplace_back(remaining, join->inputEdgeCount() - 1, join);
    snkVector.back().lowerDep_ = sinkLinker.upperDep_;
    snkVector.back().upperDep_ = sinkLinker.upperDep_;

}

void spider::srdag::replaceJobInterfaces(TransfoJob &transfoJob) {
    if (!transfoJob.job_.reference_->inputEdgeCount() &&
        !transfoJob.job_.reference_->outputEdgeCount()) {
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
        auto *vertex = spider::api::createRepeat(transfoJob.srdag_,
                                                 srdagInstance->name() + "_" + interface->name(),
                                                 StackID::TRANSFO);
        edge->setSink(vertex, 0, Expression(edge->sinkRateExpression()));
        transfoJob.tracker_[uniformIx(interface, transfoJob.job_.reference_)] = vertex->ix();
    }

    /* == Replace the output interfaces == */
    for (const auto &interface : transfoJob.job_.reference_->outputInterfaceArray()) {
        auto *edge = srdagInstance->outputEdge(interface->ix());
        auto *vertex = spider::api::createTail(transfoJob.srdag_,
                                               srdagInstance->name() + "_" + interface->name(),
                                               1,
                                               StackID::TRANSFO);
        edge->setSource(vertex, 0, Expression(edge->sourceRateExpression()));
        transfoJob.tracker_[uniformIx(interface, transfoJob.job_.reference_)] = vertex->ix();
    }
}

void spider::srdag::computeEdgeDependencies(TransfoStack &srcVector, TransfoStack &snkVector, TransfoJob &transfoJob) {
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
        auto snkLowerDep = spider::pisdf::computeConsLowerDep(currentSinkRate, srcRate, firing, delay);
        auto snkUpperDep = spider::pisdf::computeConsUpperDep(currentSinkRate, srcRate, firing, delay);
        if (snkLowerDep < 0) {
            /* == Update dependencies for init / setter == */
            snkLowerDep -= spider::pisdf::computeConsLowerDep(snkRate, setterRate, firing, 0);
            if (snkUpperDep < 0) {
                snkUpperDep -= spider::pisdf::computeConsUpperDep(snkRate, setterRate, firing, 0);
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