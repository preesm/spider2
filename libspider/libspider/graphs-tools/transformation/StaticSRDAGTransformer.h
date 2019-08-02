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
#ifndef SPIDER2_STATICSRDAGTRANSFORMER_H
#define SPIDER2_STATICSRDAGTRANSFORMER_H

/* === Include(s) === */

#include <cstdint>
#include <graphs/pisdf/PiSDFEdge.h>
#include <graphs/pisdf/PiSDFDelay.h>

/* === Forward declaration(s) === */

class PiSDFGraph;

class PiSDFVertex;

/* === Class definition === */

class StaticSRDAGTransformer {
public:
    explicit StaticSRDAGTransformer(const PiSDFGraph *graph);

    StaticSRDAGTransformer(const PiSDFGraph *graph, PiSDFGraph *srdag);

    ~StaticSRDAGTransformer();

    /* === Method(s) === */

    void execute();

    /* === Getter(s) === */

    inline const PiSDFGraph *srdag() const;

    /* === Setter(s) === */

private:
    const PiSDFGraph *piSdfGraph_ = nullptr;
    PiSDFGraph *srdag_ = nullptr;
    bool externSRDAG_ = false;

    /* === Private structure(s) === */

    struct EdgeLinker {
        PiSDFVertex *source = nullptr;
        PiSDFVertex *sink = nullptr;
        std::uint64_t sourceRate = 0;
        std::uint64_t sinkRate = 0;
        std::uint64_t delay = 0;
        std::uint32_t sourceCount = 0;
        std::uint32_t sinkCount = 0;
        std::uint32_t sinkPortIx = 0;
        std::uint32_t sourcePortIx = 0;
        Spider::Array<PiSDFVertex *> *sourceArray = nullptr;
        Spider::Array<PiSDFVertex *> *sinkArray = nullptr;

        EdgeLinker(const PiSDFEdge *edge,
                   Spider::Array<PiSDFVertex *> &sourceArray,
                   Spider::Array<PiSDFVertex *> &sinkArray) : source{edge->source()},
                                                              sink{edge->sink()},
                                                              sourceArray{&sourceArray},
                                                              sinkArray{&sinkArray} {
            sourceRate = edge->sourceRate();
            sinkRate = edge->sinkRate();
            delay = edge->delayValue();
            sourcePortIx = edge->sourcePortIx();
            sinkPortIx = edge->sinkPortIx();
        }

        ~EdgeLinker() = default;
    };

    struct SinkLinker {
        PiSDFVertex *vertex = nullptr;
        std::uint32_t sinkPortIx = 0;
        std::uint64_t sinkRate = 0;
        std::int64_t lowerDep = 0;
        std::int64_t upperDep = 0;

        SinkLinker() = default;

        ~SinkLinker() = default;
    };

    /* === Private method(s) === */

    PiSDFVertex *copyVertex(const PiSDFVertex *vertex, std::uint32_t instance = 0);

    void extractAndLinkActors(const PiSDFGraph *graph);

    void singleRateLinkage(EdgeLinker &edgeLinker);

    void buildSourceLinkArray(EdgeLinker &edgeLinker, Spider::Array<PiSDFVertex *> &sourceLinkArray);

    void buildSinkLinkArray(EdgeLinker &edgeLinker, Spider::Array<SinkLinker> &sinkLinkArray);

    void reconnectSetter(const PiSDFEdge *edge, PiSDFVertex *delayVertex, PiSDFVertex *sink);

    void reconnectGetter(const PiSDFEdge *edge, PiSDFVertex *delayVertex, PiSDFVertex *source);
};

/* === Inline method(s) === */

const PiSDFGraph *StaticSRDAGTransformer::srdag() const {
    return srdag_;
}

#endif //SPIDER2_STATICSRDAGTRANSFORMER_H
