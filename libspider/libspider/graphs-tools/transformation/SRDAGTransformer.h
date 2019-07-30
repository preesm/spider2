/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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
#ifndef SPIDER2_SRDAGTRANSFORMER_H
#define SPIDER2_SRDAGTRANSFORMER_H

/* === Include(s) === */

#include <cstdint>
#include <graphs/pisdf/PiSDFEdge.h>
#include <graphs/pisdf/PiSDFDelay.h>

/* === Forward declaration(s) === */

class PiSDFGraph;

class PiSDFVertex;

/* === Class definition === */

class SRDAGTransformer {
public:
    explicit SRDAGTransformer(const PiSDFGraph *graph);

    ~SRDAGTransformer();

    /* === Method(s) === */

    void execute();

    void resume();

    /* === Getter(s) === */

    inline const PiSDFGraph *srdag() const;

    /* === Setter(s) === */

private:
    PiSDFGraph *srdag_ = nullptr;
    const PiSDFGraph *piSdfGraph_ = nullptr;
    bool stoppedFromConfig_ = false;

    /* === Private struct === */
    struct SRLinker {
        PiSDFVertex *source = nullptr;
        PiSDFVertex *sink = nullptr;
        std::uint64_t sourceRate = 0;
        std::uint64_t sinkRate = 0;
        std::uint64_t delay = 0;
        std::uint32_t sourceCount = 0;
        std::uint32_t sinkCount = 0;
        std::uint32_t sinkPortIx = 0;
        std::uint32_t sourcePortIx = 0;
        PiSDFVertex *init = nullptr;
        Spider::Array<PiSDFVertex *> *sourceArray = nullptr;
        Spider::Array<PiSDFVertex *> *sinkArray = nullptr;

        SRLinker(const PiSDFEdge *edge,
                 Spider::Array<Spider::Array<PiSDFVertex *>> &vertex2Vertex) : source{edge->source()},
                                                                               sink{edge->sink()} {
            sourceRate = edge->sourceRate();
            sinkRate = edge->sinkRate();
            sourceArray = &vertex2Vertex[edge->source()->getIx()];
            sinkArray = &vertex2Vertex[edge->sink()->getIx()];
            if (edge->delay()) {
                delay = edge->delay()->value();
            }
            sourcePortIx = edge->sourcePortIx();
            sinkPortIx = edge->sinkPortIx();
        }

        ~SRLinker() = default;
    };

    /* === Private method(s) === */

    PiSDFVertex *copyVertex(const PiSDFVertex *vertex, std::uint32_t instance = 0);

    void extractConfigActors(const PiSDFGraph *graph);

    void extractAndLinkActors(const PiSDFGraph *graph);

    void nodelayLinkage(SRLinker &linker);

    void forwardLinkage(SRLinker &linker);

    void joinPatternLinkage(SRLinker &linker);

    void forkPatternLinkage(SRLinker &linker);

    void delayLinkage(SRLinker &linker);

    void fullyPipelinedLinkage(SRLinker &linker);

    void delayInitLinkage(SRLinker &linker);

    void delayEndLinkage(SRLinker &linker);

    void delayedJoinPatternLinkage(SRLinker &linker);

    void delayedForkPatternLinkage(SRLinker &linker);

    static inline std::int64_t computeConsLowerDep(std::int64_t sinkRate,
                                                   std::int64_t sourceRate,
                                                   std::uint32_t instance,
                                                   std::int64_t delay);

    static inline std::int64_t computeConsUpperDep(std::int64_t sinkRate,
                                                   std::int64_t sourceRate,
                                                   std::uint32_t instance,
                                                   std::int64_t delay);

    static inline std::int64_t computeProdLowerDep(std::int64_t sinkRate,
                                                   std::int64_t sourceRate,
                                                   std::uint32_t instance,
                                                   std::int64_t delay,
                                                   std::int64_t sinkRepetitionValue);

    static inline std::int64_t computeProdUpperDep(std::int64_t sinkRate,
                                                   std::int64_t sourceRate,
                                                   std::uint32_t instance,
                                                   std::int64_t delay,
                                                   std::int64_t sinkRepetitionValue);
};

/* === Inline method(s) === */

const PiSDFGraph *SRDAGTransformer::srdag() const {
    return srdag_;
}

std::int64_t SRDAGTransformer::computeConsLowerDep(std::int64_t sinkRate,
                                                   std::int64_t sourceRate,
                                                   std::uint32_t instance,
                                                   std::int64_t delay) {
    auto consumed = instance * sinkRate - delay;
    auto lowerDep = Spider::Math::floorDiv(consumed, sourceRate);
    constexpr std::int64_t initBound = -1;
    return std::max(initBound, lowerDep);
}

std::int64_t SRDAGTransformer::computeConsUpperDep(std::int64_t sinkRate,
                                                   std::int64_t sourceRate,
                                                   std::uint32_t instance,
                                                   std::int64_t delay) {
    auto consumed = (instance + 1) * sinkRate - delay - 1;
    auto lowerDep = Spider::Math::floorDiv(consumed, sourceRate);
    constexpr std::int64_t initBound = -1;
    return std::max(initBound, lowerDep);
}

std::int64_t SRDAGTransformer::computeProdLowerDep(std::int64_t sinkRate,
                                                   std::int64_t sourceRate,
                                                   std::uint32_t instance,
                                                   std::int64_t delay,
                                                   std::int64_t sinkRepetitionValue) {
    auto produced = instance * sourceRate + delay;
    auto lowerDep = produced / sinkRate;
    return std::min(sinkRepetitionValue, lowerDep);
}

std::int64_t SRDAGTransformer::computeProdUpperDep(std::int64_t sinkRate,
                                                   std::int64_t sourceRate,
                                                   std::uint32_t instance,
                                                   std::int64_t delay,
                                                   std::int64_t sinkRepetitionValue) {
    auto produced = (instance + 1) * sourceRate + delay - 1;
    auto lowerDep = produced / sinkRate;
    return std::min(sinkRepetitionValue, lowerDep);
}




#endif //SPIDER2_SRDAGTRANSFORMER_H
