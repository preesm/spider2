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
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs-tools/brv/LCMBRVCompute.h>

/* === Static function(s) === */

static PiSDFAbstractVertex *
fetchOrClone(const PiSDFAbstractVertex *reference, Spider::SRDAG::EdgeLinker &linker) {
    if (!reference) {
        throwSpiderException("Trying to clone nullptr vertex.");
    }
    auto &index = linker.tracker_[reference->ix()];

    // TODO: handle the split of dynamic graphs

    /* == If vertex has already been cloned return the first one == */
    if (index != UINT32_MAX) {
        return linker.srdag_->vertices()[index];
    }

    /* == Clone the vertex N times and return the first one == */
    auto *clone = reference->clone(StackID::TRANSFO, linker.srdag_);
    index = clone->ix();
    for (std::uint32_t it = 1; it < reference->repetitionValue(); ++it) {
        auto *vertex = reference->clone(StackID::TRANSFO, linker.srdag_);
        vertex->setName(vertex->name() + "_" + std::to_string(it));
    }
    return clone;
}

static void pushReverseVertexLinkerVector(Spider::SRDAG::LinkerVector &vector,
                                          const PiSDFAbstractVertex *reference,
                                          const std::int64_t &rate,
                                          const std::uint32_t portIx,
                                          Spider::SRDAG::EdgeLinker &linker) {
    using Spider::SRDAG::VertexLinker;
    const auto &cloneIx = fetchOrClone(reference, linker)->ix();
    for (auto i = (cloneIx + reference->repetitionValue() - 1); i >= cloneIx; --i) {
        vector.push_back(VertexLinker{rate, portIx, linker.srdag_->vertices()[i]});
    }
}

static Spider::SRDAG::LinkerVector
buildSourceLinkerVector(Spider::SRDAG::EdgeLinker &linker) {
    const auto &edge = linker.edge_;
    const auto *source = edge->source();
    const auto *delay = edge->delay();
    Spider::SRDAG::LinkerVector sourceVector;
    sourceVector.reserve(source->repetitionValue() +
                         (delay ? delay->setter()->repetitionValue() : 0));

    /* == Populate first the source clones in reverse order == */
    pushReverseVertexLinkerVector(sourceVector,
                                  source,
                                  edge->sourceRateExpression().evaluate(),
                                  edge->sourcePortIx(),
                                  linker);

    /* == If delay, populate the setter clones in reverse order == */
    if (delay) {
        const auto &setterEdge = delay->vertex()->inputEdge(0);
        pushReverseVertexLinkerVector(sourceVector,
                                      delay->setter(),
                                      setterEdge->sourceRateExpression().evaluate(),
                                      setterEdge->sourcePortIx(),
                                      linker);
    }
    return sourceVector;
}

static Spider::vector<Spider::SRDAG::VertexLinker, StackID::TRANSFO>
buildSinkLinkerVector(Spider::SRDAG::EdgeLinker &linker) {
    const auto &edge = linker.edge_;
    const auto *sink = edge->sink();
    const auto *delay = edge->delay();
    Spider::vector<Spider::SRDAG::VertexLinker, StackID::TRANSFO> sinkVector;
    sinkVector.reserve(sink->repetitionValue() +
                       (delay ? delay->getter()->repetitionValue() : 0));

    /* == First, if delay, populate the getter clones in reverse order == */
    if (delay) {
        const auto &getterEdge = delay->vertex()->outputEdge(0);
        pushReverseVertexLinkerVector(sinkVector,
                                      delay->getter(),
                                      getterEdge->sinkRateExpression().evaluate(),
                                      getterEdge->sinkPortIx(),
                                      linker);
    }

    /* == Populate the sink clones in reverse order == */
    pushReverseVertexLinkerVector(sinkVector,
                                  sink,
                                  edge->sinkRateExpression().evaluate(),
                                  edge->sinkPortIx(),
                                  linker);
    return sinkVector;
}

/* === Methods implementation === */

std::pair<Spider::SRDAG::JobStack, Spider::SRDAG::JobStack>
Spider::SRDAG::staticSingleRateTransformation(const Spider::SRDAG::Job &job, PiSDFGraph *srdag) {
    if (!srdag) {
        throwSpiderException("nullptr for single rate graph.");
    }
    if (!job.reference_) {
        throwSpiderException("nullptr for job.reference graph.");
    }

    auto *srdagInstance = srdag;
    if (job.srdagIx_ >= 0) {
        // TODO: update parameters values of job.reference_ using the one of current instance
        srdagInstance = srdag->subgraphs()[job.srdagIx_];
        if (!srdagInstance || (srdagInstance->reference() != job.reference_)) {
            throwSpiderException("could not find matching single rate instance [%"
                                         PRIu32
                                         "] of graph [%s]", job.instanceValue_, job.reference_->name().c_str());
        }
    }

    /* == Compute the repetition values of the graph (if dynamic and/or first instance) == */
    if (job.reference_->dynamic() || job.srdagIx_ <= 0) {
        LCMBRVCompute brvTask{job.reference_};
        brvTask.execute();
    }

    JobStack nextJobs;
    JobStack dynaJobs;
    /* == Do the linkage for every edges of the graph == */
    TransfoTracker vertexTransfoTracker;
    vertexTransfoTracker.reserve(job.reference_->vertexCount());
    std::fill(vertexTransfoTracker.begin(), vertexTransfoTracker.end(), UINT32_MAX);
    auto linker = EdgeLinker{nullptr, srdag, job, nextJobs, dynaJobs, vertexTransfoTracker};
    for (const auto &edge : job.reference_->edges()) {
        linker.edge_ = edge;
        staticEdgeSingleRateLinkage(linker);
    }

    /* == Check for non-connected vertices == */
    linker.edge_ = nullptr;
    for (const auto &vertex : job.reference_->vertices()) {
        fetchOrClone(vertex, linker);
    }

    return std::make_pair(std::move(nextJobs), std::move(dynaJobs));
}

void Spider::SRDAG::staticEdgeSingleRateLinkage(EdgeLinker &linker) {

    auto sourceVector = buildSourceLinkerVector(linker);
    auto sinkVector = buildSinkLinkerVector(linker);

    /* == Iterate over sinks == */
    while (!sinkVector.empty()) {

    }

    /* == Left overs == */
    while (!sourceVector.empty()) {

    }

    // for (sink : sinks) {
    //      sink.lower==sink.upper ?
    //          source.lower == source.upper?
    //              connect source.ix->sink.ix;
    //          else: label=create_fork
    //              create fork;
    //              connect source.ix->fork.0
    //              sources.pop()
    //              for (out : fork.count - 1) {
    //                  connect fork.out->sink.ix;
    //                  sinks.pop();
    //                  sink = sinks.next
    //              }
    //              source.push(fork, fork.count, remaining)
    //      else: label=create_join
    //          create join;
    //          connect join.0->sink.ix;
    //          sinks.pop()
    //          for (in : join.in - 1) {
    //              connect source.ix->join.in;
    //              sources.pop();
    //              source = sources.next;
    //          }
    //          sinks.push(join, join.count, remaining)
}
