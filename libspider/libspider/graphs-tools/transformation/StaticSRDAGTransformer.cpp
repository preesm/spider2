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

#include <memory/Allocator.h>
#include <graphs/pisdf/PiSDFGraph.h>
#include <graphs/pisdf/PiSDFVertex.h>
#include <spider-api/pisdf.h>
#include <graphs-tools/numerical/PiSDFAnalysis.h>
#include <graphs-tools/transformation/StaticSRDAGTransformer.h>
#include <graphs-tools/brv/LCMBRVCompute.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

StaticSRDAGTransformer::StaticSRDAGTransformer(const PiSDFGraph *graph) : piSdfGraph_{graph},
                                                                          externSRDAG_{false} {

}

StaticSRDAGTransformer::StaticSRDAGTransformer(const PiSDFGraph *graph, PiSDFGraph *srdag) : piSdfGraph_{graph},
                                                                                             srdag_{srdag},
                                                                                             externSRDAG_{true} {
}

StaticSRDAGTransformer::~StaticSRDAGTransformer() {
    if (srdag_ && !externSRDAG_) {
        Spider::destroy(srdag_);
        Spider::deallocate(srdag_);
    }
}

void StaticSRDAGTransformer::execute() {
    if (done_) {
        return;
    }
    if (!piSdfGraph_) {
        throwSpiderException("Cannot transform nullptr PiSDFGraph.");
    } else if (!piSdfGraph_->isStatic()) {
        throwSpiderException("Cannot transform non-static graph.");
    }

    if (!srdag_) {
        srdag_ = Spider::allocate<PiSDFGraph>(StackID::PISDF);
        Spider::construct(srdag_,
                          nullptr,
                          "srdag-" + piSdfGraph_->name(),
                          0, /* = nActors = */
                          0, /* = nEdges = */
                          0, /* = nParams = */
                          0, /* = nInputInterfaces = */
                          0, /* = nOutputInterfaces = */
                          0  /* = nConfigActors = */);
    }

    /* == Extract the vertices from the top graph == */
    jobs_.push_back(SRDAGTransfoJob{piSdfGraph_});

    /* == Iterate over the subgraphs == */
    std::uint64_t jobIx = 0;
    while (jobIx != jobs_.size()) {
        /* == Pop the job == */
        auto job = jobs_[jobIx];

        /* == Compute BRV of the graph == */
        if (!job.reference->isStatic() || !job.firingCount) {
//            LCMBRVCompute lcmbrvCompute{job.reference};
            // TODO: use the firing count for the value of the parameter (even though they should be evaluated in order here)
//            lcmbrvCompute.execute();
        }

        /* == Do the job == */
        extractAndLinkActors(job);

        /* == Perform replace of the actor in the srdag and remove it == */
        if (job.srdagIx != UINT32_MAX) {
            srdag_->removeVertex(srdag_->vertices()[job.srdagIx]);
        }
        jobIx += 1;
    }

    /* == Set flag done to true == */
    done_ = true;
}

/* === Private method(s) === */

PiSDFVertex *StaticSRDAGTransformer::copyVertex(const PiSDFVertex *vertex,
                                                std::uint32_t instance,
                                                const std::string &prefix) {
    auto *copyVertex = Spider::allocate<PiSDFVertex>(StackID::TRANSFO);
    Spider::construct(copyVertex,
                      StackID::TRANSFO,
                      srdag_,
                      prefix + std::string(vertex->name()) + "_" + std::to_string(instance),
                      vertex->type(),
                      vertex->nEdgesIN(),
                      vertex->nEdgesOUT(),
                      vertex->nParamsIN(),
                      vertex->nParamsOUT());
    copyVertex->setRepetitionValue(1);
    copyVertex->setReference(vertex->reference());

    /* == Add the job for later process == */
    if (vertex->isHierarchical()) {
        jobs_.push_back(SRDAGTransfoJob{static_cast<const PiSDFGraph *>(vertex),  /* = reference = */
                                        copyVertex->ix(),                            /* = srdagIx = */
                                        instance});                                  /* = firingCount = */
    }

    /* == Copy the input parameter == */
    std::uint32_t ix = 0;
    for (auto *param : vertex->inputParams()) {
        copyVertex->setInputParam(param, ix);
        ix += 1;
    }
    return copyVertex;
}

void StaticSRDAGTransformer::extractAndLinkActors(SRDAGTransfoJob &job) {
    auto *graph = job.reference;

    /* == Array to keep track of who has been done == */
    Spider::Array<Spider::Array<PiSDFVertex *>> vertex2Vertex{graph->nVertices(), StackID::TRANSFO};

    /* == Dummy arrays for interfaces (if any) == */
    Spider::Array<PiSDFVertex *> arrayInputInterface{1, StackID::TRANSFO};
    Spider::Array<PiSDFVertex *> arrayOutputInterface{1, StackID::TRANSFO};

    /* == Copy all vertices == */
    auto prefix = job.srdagIx != UINT32_MAX ? srdag_->vertices()[job.srdagIx]->name() + "-" : "";
    for (const auto *vertex : graph->vertices()) {
        Spider::construct(&vertex2Vertex[vertex->ix()], vertex->repetitionValue(), StackID::TRANSFO);
        for (std::uint32_t i = 0; i < vertex->repetitionValue(); ++i) {
            vertex2Vertex[vertex->ix()][i] = copyVertex(vertex, i, prefix);
        }
    }

    /* == Do the linkage == */
    for (const auto *edge : graph->edges()) {
        auto *sourceArray = &vertex2Vertex[edge->source()->ix()];
        auto *sinkArray = &vertex2Vertex[edge->sink()->ix()];
        auto sourceRate = edge->sourceRate();
        auto sinkRate = edge->sinkRate();

        if (edge->source()->type() == PiSDFVertexType::INTERFACE) {
            sourceArray = &arrayInputInterface;
            arrayInputInterface[0] = Spider::API::createUpsample(srdag_, prefix + edge->source()->name());
            auto *graphVertex = srdag_->vertices()[job.srdagIx];
            auto *edge2Replace = graphVertex->inputEdge(edge->source()->ix());
            auto rate = edge2Replace->sinkRate();
            if (rate != sourceRate) {
                throwSpiderException("Interface should have same rate inside and outside the graph. [%s] -> %"
                                             PRIu64
                                             " != %"
                                             PRIu64
                                             "", edge->source()->name().c_str(), rate, sourceRate);
            }
            edge2Replace->disconnectSink();
            edge2Replace->connectSink(arrayInputInterface[0], 0, rate);
            sourceRate = sinkRate * edge->sink()->repetitionValue();
        }
        if (edge->sink()->type() == PiSDFVertexType::INTERFACE) {
            sinkArray = &arrayOutputInterface;
            arrayOutputInterface[0] = Spider::API::createDownsample(srdag_, prefix + edge->sink()->name());
            auto *graphVertex = srdag_->vertices()[job.srdagIx];
            auto *edge2Replace = graphVertex->outputEdge(edge->sink()->ix());
            auto rate = edge2Replace->sourceRate();
            if (rate != sinkRate) {
                throwSpiderException("Interface should have same rate inside and outside the graph. [%s] -> %"
                                             PRIu64
                                             " != %"
                                             PRIu64
                                             "", edge->sink()->name().c_str(), sinkRate, rate);
            }
            edge2Replace->disconnectSource();
            edge2Replace->connectSource(arrayOutputInterface[0], 0, rate);
            sinkRate = sourceRate * edge->source()->repetitionValue();
        }

        /* == Do the linkage == */
        auto edgeLinker = EdgeLinker{edge, *sourceArray, *sinkArray, sourceRate, sinkRate};
        singleRateLinkage(edgeLinker);

        /* == Check that everything has been linked == */
        if (edgeLinker.sinkCount != edgeLinker.sink->repetitionValue()) {
            throwSpiderException("Remaining %"
                                         PRIu32
                                         " instances of %s to link.",
                                 edgeLinker.sink->repetitionValue() - edgeLinker.sinkCount,
                                 edgeLinker.sink->name().c_str());
        }
        if (edgeLinker.sourceCount != edgeLinker.source->repetitionValue()) {
            throwSpiderException("Remaining %"
                                         PRIu32
                                         " instances of %s to link.",
                                 edgeLinker.source->repetitionValue() - edgeLinker.sourceCount,
                                 edgeLinker.source->name().c_str());
        }
    }

    /* == Reconnect setter / getter (if any) == */
    for (const auto *edge : graph->edges()) {
        if (edge->delay()) {
            /* == Retrieve the virtual vertex (there can be only one) == */
            auto *delayVertex = vertex2Vertex[edge->delay()->virtualVertex()->ix()][0];

            if (edge->delay()->setter()) {
                /* == Disconnect / reconnect setter == */
                auto &sinkArray = vertex2Vertex[edge->sink()->ix()];
                reconnectSetter(edge, delayVertex, sinkArray[0]);
            }

            if (edge->delay()->getter()) {
                /* == Disconnect / reconnect getter == */
                auto &sourceArray = vertex2Vertex[edge->source()->ix()];
                reconnectGetter(edge, delayVertex, sourceArray[edge->source()->repetitionValue() - 1]);
            }

            /* == Remove the delay vertex == */
            srdag_->removeVertex(delayVertex);
        }
    }

    /* == Free memory of the arrays == */
    for (auto &array : vertex2Vertex) {
        Spider::destroy(&array);
    }
}

void StaticSRDAGTransformer::singleRateLinkage(StaticSRDAGTransformer::EdgeLinker &edgeLinker) {
    auto hasDelay = edgeLinker.delay != 0;
    Spider::Array<PiSDFVertex *> sourceLinkArray{edgeLinker.source->repetitionValue() + hasDelay, StackID::TRANSFO};
    /* == Build the sourceLinkArray == */
    buildSourceLinkArray(edgeLinker, sourceLinkArray);

    /* == Build the sinkLinkArray == */
    Spider::Array<SinkLinker> sinkLinkArray{edgeLinker.sink->repetitionValue() + hasDelay, StackID::TRANSFO};
    buildSinkLinkArray(edgeLinker, sinkLinkArray);

    /* == Do the actual linkage == */
    std::uint32_t forkPortIx = 0;
    std::uint64_t forkConsumption = 0;
    for (std::uint64_t sinkIx = 0; sinkIx < sinkLinkArray.size(); ++sinkIx) {
        auto &sinkLinker = sinkLinkArray[sinkIx];
        auto *src = sourceLinkArray[sinkLinker.lowerDep + hasDelay];
        auto *snk = sinkLinker.vertex;
        if (sinkLinker.lowerDep == sinkLinker.upperDep) {
            /* == Sink can be connected directly == */
            if (src->type() == PiSDFVertexType::INIT) {
                /* == Case delay == sourceRate == */
                Spider::API::createEdge(srdag_, src, 0, edgeLinker.sinkRate,
                                        snk, sinkLinker.sinkPortIx, edgeLinker.sinkRate, StackID::TRANSFO);
            } else if (edgeLinker.sourceRate == sinkLinker.sinkRate) {
                /* == Case sinkRate == sourceRate == */
                Spider::API::createEdge(srdag_, src, edgeLinker.sourcePortIx, edgeLinker.sourceRate,
                                        snk, sinkLinker.sinkPortIx, sinkLinker.sinkRate, StackID::TRANSFO);
            } else {
                /* == Case sinkRate < sourceRate == */
                Spider::API::createEdge(srdag_, src, forkPortIx, sinkLinker.sinkRate,
                                        snk, sinkLinker.sinkPortIx, sinkLinker.sinkRate, StackID::TRANSFO);
                forkPortIx = (forkPortIx + 1) % src->nEdgesOUT();
                forkConsumption += sinkLinker.sinkRate;
            }
        } else {
            /* == Sink needs a join == */
            auto nInput = (sinkLinker.upperDep - sinkLinker.lowerDep) + 1;
            auto *join = Spider::API::createJoin(srdag_,
                                                 "join-" + snk->name() + "-in" + std::to_string(edgeLinker.sinkPortIx),
                                                 nInput, 0, StackID::TRANSFO);
            Spider::API::createEdge(srdag_, join, 0, sinkLinker.sinkRate,
                                    snk, sinkLinker.sinkPortIx, sinkLinker.sinkRate, StackID::TRANSFO);
            std::uint64_t firstEdgeConsumption = 0;
            switch (src->type()) {
                case PiSDFVertexType::FORK:
                    /* == Case sinkRate < sourceRate == */
                    firstEdgeConsumption = src->inputEdge(0)->sinkRate() - forkConsumption;
                    Spider::API::createEdge(srdag_, src, (src->nEdgesOUT() - 1), firstEdgeConsumption,
                                            join, 0, firstEdgeConsumption, StackID::TRANSFO);
                    forkPortIx = (forkPortIx + 1) % src->nEdgesOUT();
                    break;
                case PiSDFVertexType::INIT:
                    /* == Case delay < sinkRate == */
                    firstEdgeConsumption = edgeLinker.delay;
                    Spider::API::createEdge(srdag_, src, 0, firstEdgeConsumption,
                                            join, 0, firstEdgeConsumption, StackID::TRANSFO);
                    break;
                default:
                    /* == Case sinkRate > sourceRate == */
                    firstEdgeConsumption = edgeLinker.sourceRate;
                    Spider::API::createEdge(srdag_, src, edgeLinker.sourcePortIx, firstEdgeConsumption,
                                            join, 0, firstEdgeConsumption, StackID::TRANSFO);
                    break;
            }

            /* == Connect everything else to the join == */
            /* == case of pattern:  F -> J -> B == */
            /* ==                 A_i ->        == */
            /* ==                [..] ->        == */
            /* ==                 A_j ->        == */
            /* ==                   F ->        == */
            std::uint32_t joinPortIx = 1;
            auto joinProduction = sinkLinker.sinkRate - firstEdgeConsumption;
            for (auto i = sinkLinker.lowerDep + hasDelay + 1; i < sinkLinker.upperDep + hasDelay; ++i) {
                src = sourceLinkArray[i];
                Spider::API::createEdge(srdag_, src, edgeLinker.sourcePortIx, edgeLinker.sourceRate,
                                        join, joinPortIx, edgeLinker.sourceRate, StackID::TRANSFO);
                joinProduction -= edgeLinker.sourceRate;
                joinPortIx += 1;
            }

            /* == Replace sink with join == */
            /* == last source can be either: A_j -> J == */
            /* ==                        or:   F -> J == */
            sinkLinker.vertex = join;
            sinkLinker.sinkRate = joinProduction;
            sinkLinker.lowerDep = sinkLinker.upperDep;
            sinkLinker.sinkPortIx = joinPortIx;
            sinkIx -= 1;
            forkConsumption = 0;
        }
    }
}

void StaticSRDAGTransformer::buildSourceLinkArray(StaticSRDAGTransformer::EdgeLinker &edgeLinker,
                                                  Spider::Array<PiSDFVertex *> &sourceLinkArray) {
    auto &sourceArray = *(edgeLinker.sourceArray);
    auto hasDelay = edgeLinker.delay != 0;

    /* == If delay, then first sink will be init (or fork-init) == */
    if (edgeLinker.delay) {
        auto *init = Spider::API::createInit(srdag_, "init-" + edgeLinker.sink->name(), 0, StackID::TRANSFO);
        if (edgeLinker.delay > edgeLinker.sinkRate) {
            std::uint32_t nConsumer = Spider::Math::ceilDiv(edgeLinker.delay, edgeLinker.sinkRate);
            nConsumer = std::min(nConsumer, edgeLinker.sink->repetitionValue() + 1);
            auto *fork = Spider::API::createFork(srdag_, "fork-" + init->name(), nConsumer, 0, StackID::TRANSFO);
            sourceLinkArray[0] = fork;
            Spider::API::createEdge(srdag_, init, 0, edgeLinker.delay,
                                    fork, 0, edgeLinker.delay, StackID::TRANSFO);
        } else {
            sourceLinkArray[0] = init;
        }

    }

    /* == Set the sources or sources-fork == */
    for (auto *src : sourceArray) {
        auto lowerDep = Spider::PiSDF::computeProdLowerDep(edgeLinker.sinkRate,
                                                           edgeLinker.sourceRate,
                                                           edgeLinker.sourceCount,
                                                           edgeLinker.delay,
                                                           edgeLinker.sink->repetitionValue());
        auto upperDep = Spider::PiSDF::computeProdUpperDep(edgeLinker.sinkRate,
                                                           edgeLinker.sourceRate,
                                                           edgeLinker.sourceCount,
                                                           edgeLinker.delay,
                                                           edgeLinker.sink->repetitionValue());
        if (lowerDep == upperDep) {
            sourceLinkArray[edgeLinker.sourceCount + hasDelay] = src;
        } else {
            auto nConsumer = (upperDep - lowerDep) + 1;
            auto *fork = Spider::API::createFork(srdag_, "fork-" + src->name() + "-out" +
                                                         std::to_string(edgeLinker.sourcePortIx),
                                                 nConsumer, 0, StackID::TRANSFO);
            sourceLinkArray[edgeLinker.sourceCount + hasDelay] = fork;
            Spider::API::createEdge(srdag_, src, edgeLinker.sourcePortIx, edgeLinker.sourceRate,
                                    fork, 0, edgeLinker.sourceRate, StackID::TRANSFO);
        }
        edgeLinker.sourceCount += 1;
    }
}

void StaticSRDAGTransformer::buildSinkLinkArray(StaticSRDAGTransformer::EdgeLinker &edgeLinker,
                                                Spider::Array<SinkLinker> &sinkLinkArray) {
    auto &sinkArray = *(edgeLinker.sinkArray);

    /* == Create the end vertex if needed and put it at the end == */
    if (edgeLinker.delay) {
        auto *end = Spider::API::createEnd(srdag_, "end-" + edgeLinker.source->name(), 0, StackID::TRANSFO);
        auto &lastSinkLinker = sinkLinkArray[sinkLinkArray.size() - 1];
        lastSinkLinker.vertex = end;
        lastSinkLinker.sinkRate = edgeLinker.delay;
        lastSinkLinker.sinkPortIx = 0;
        lastSinkLinker.lowerDep = Spider::PiSDF::computeConsLowerDep(edgeLinker.sinkRate,
                                                                     edgeLinker.sourceRate,
                                                                     edgeLinker.sink->repetitionValue(),
                                                                     edgeLinker.delay);
        lastSinkLinker.upperDep = edgeLinker.source->repetitionValue() - 1;
    }

    /* == Add the sinks == */
    for (auto *snk : sinkArray) {
        auto &sinkLinker = sinkLinkArray[edgeLinker.sinkCount];
        sinkLinker.vertex = snk;
        sinkLinker.sinkRate = edgeLinker.sinkRate;
        sinkLinker.sinkPortIx = edgeLinker.sinkPortIx;
        sinkLinker.lowerDep = Spider::PiSDF::computeConsLowerDep(edgeLinker.sinkRate,
                                                                 edgeLinker.sourceRate,
                                                                 edgeLinker.sinkCount,
                                                                 edgeLinker.delay);
        sinkLinker.upperDep = Spider::PiSDF::computeConsUpperDep(edgeLinker.sinkRate,
                                                                 edgeLinker.sourceRate,
                                                                 edgeLinker.sinkCount,
                                                                 edgeLinker.delay);
        edgeLinker.sinkCount += 1;
    }
}

void StaticSRDAGTransformer::reconnectSetter(const PiSDFEdge *edge, PiSDFVertex *delayVertex, PiSDFVertex *sink) {
    auto *setterEdge = delayVertex->inputEdge(0);
    setterEdge->disconnectSink();
    auto *inputEdge = sink->inputEdge(edge->sinkPortIx());
    auto *firstSrc2Sink = inputEdge->source();
    PiSDFVertex *init = nullptr;
    if (firstSrc2Sink->type() == PiSDFVertexType::INIT) {
        inputEdge->disconnectSink();
        setterEdge->connectSink(sink, edge->sinkPortIx(), edge->delayValue());
    } else if (firstSrc2Sink->type() == PiSDFVertexType::FORK ||
               firstSrc2Sink->type() == PiSDFVertexType::JOIN) {
        inputEdge = firstSrc2Sink->inputEdge(0);
        inputEdge->disconnectSink();
        setterEdge->connectSink(firstSrc2Sink, 0, edge->delayValue());
    }

    /* == Remove the edge and the init vertex == */
    init = inputEdge->source();
    inputEdge->disconnectSource();
    srdag_->removeEdge(inputEdge);
    srdag_->removeVertex(init);
}

void StaticSRDAGTransformer::reconnectGetter(const PiSDFEdge *edge, PiSDFVertex *delayVertex, PiSDFVertex *source) {
    auto *getterEdge = delayVertex->outputEdge(0);
    getterEdge->disconnectSource();
    auto *outputEdge = source->outputEdge(edge->sourcePortIx());
    auto *lastSnk2Source = outputEdge->sink();
    PiSDFVertex *end = nullptr;
    if (lastSnk2Source->type() == PiSDFVertexType::END) {
        outputEdge->disconnectSource();
        getterEdge->connectSource(source, edge->sourcePortIx(), edge->delayValue());
    } else if (lastSnk2Source->type() == PiSDFVertexType::FORK) {
        outputEdge = lastSnk2Source->outputEdge(lastSnk2Source->nEdgesOUT() - 1);
        outputEdge->disconnectSource();
        getterEdge->connectSource(lastSnk2Source, lastSnk2Source->nEdgesOUT() - 1, edge->delayValue());
    } else if (lastSnk2Source->type() == PiSDFVertexType::JOIN) {
        outputEdge = lastSnk2Source->outputEdge(0);
        outputEdge->disconnectSource();
        getterEdge->connectSource(lastSnk2Source, 0, edge->delayValue());
    }

    /* == Remove the output edge and the end vertex == */
    end = outputEdge->sink();
    outputEdge->disconnectSink();
    srdag_->removeEdge(outputEdge);
    srdag_->removeVertex(end);
}
