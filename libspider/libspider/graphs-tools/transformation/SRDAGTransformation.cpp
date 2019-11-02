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


static Spider::vector<Spider::SRDAG::VertexLinker, StackID::TRANSFO>
buildSourceLinkerVector(const PiSDFEdge *edge,
                        const Spider::SRDAG::Job &job,
                        PiSDFGraph *srdag,
                        Spider::SRDAG::JobStack &nextJobs,
                        Spider::SRDAG::JobStack &dynaJobs) {
    const auto *source = edge->source();
    const auto *delay = edge->delay();
    Spider::vector<Spider::SRDAG::VertexLinker, StackID::TRANSFO> sourceVector;
    if (delay) {
        sourceVector.reserve(delay->setter()->repetitionValue() + source->repetitionValue());
        for (auto i = 0; i < delay->setter()->repetitionValue(); ++i) {
            // todo: push_back
        }
    } else {
        sourceVector.reserve(source->repetitionValue());
    }
    return sourceVector;
}

static Spider::vector<Spider::SRDAG::VertexLinker, StackID::TRANSFO>
buildSinkLinkerVector(const PiSDFEdge *edge,
                      const Spider::SRDAG::Job &job,
                      PiSDFGraph *srdag,
                      Spider::SRDAG::JobStack &nextJobs,
                      Spider::SRDAG::JobStack &dynaJobs) {
    const auto *sink = edge->sink();
    const auto *delay = edge->delay();
    Spider::vector<Spider::SRDAG::VertexLinker, StackID::TRANSFO> sinkVector;
    if (delay) {
        sinkVector.reserve(delay->getter()->repetitionValue() + sink->repetitionValue());
    } else {
        sinkVector.reserve(sink->repetitionValue());
    }
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
    std::vector<bool> vertexStatusVector(job.reference_->vertexCount(), false);
    for (const auto &edge : job.reference_->edges()) {
        //TODO: make copies of source and sink, setter and getter
        staticEdgeSingleRateLinkage(edge, job, srdag, nextJobs, dynaJobs);
        vertexStatusVector[edge->source()->ix()] = true;
        vertexStatusVector[edge->sink()->ix()] = true;
    }

    /* == Check for non-connected vertices == */
    for (const auto &vertex : job.reference_->vertices()) {
        if (!vertexStatusVector[vertex->ix()]) {
            //TODO: copy the vertex into the srdag
        }
    }

    return std::make_pair(std::move(nextJobs), std::move(dynaJobs));
}

void Spider::SRDAG::staticEdgeSingleRateLinkage(PiSDFEdge *edge,
                                                const Spider::SRDAG::Job &job,
                                                PiSDFGraph *srdag,
                                                Spider::SRDAG::JobStack &nextJobs,
                                                Spider::SRDAG::JobStack &dynaJobs) {

    auto sourceVector = buildSourceLinkerVector(edge, job, srdag, nextJobs, dynaJobs);
    auto sinkVector = buildSinkLinkerVector(edge, job, srdag, nextJobs, dynaJobs);

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
