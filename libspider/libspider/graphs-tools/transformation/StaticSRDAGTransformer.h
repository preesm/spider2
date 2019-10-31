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
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>

/* === Class definition === */

class StaticSRDAGTransformer {
public:
    explicit StaticSRDAGTransformer(const PiSDFGraph *graph);

    StaticSRDAGTransformer(const PiSDFGraph *graph, PiSDFGraph *srdag);

    ~StaticSRDAGTransformer();

    /* === Method(s) === */

    /**
     * @brief Creates and applies the SR-DAG transformation to the graph.
     * @remark If called multiple times, apply transformation only on first call.
     */
    void execute();

    /* === Getter(s) === */

    /**
     * @brief Get the resulting SR-DAG.
     * @remark if called before @refitem StaticSRDAGTransformer::execute -> return nullptr graph.
     * @return Single rate graph.
     */
    inline PiSDFGraph *srdag() const;

    /* === Setter(s) === */

private:
    const PiSDFGraph *piSdfGraph_ = nullptr;
    PiSDFGraph *srdag_ = nullptr;
    bool externSRDAG_ = false;
    bool done_ = false;

    /* === Private structure(s) === */

    /**
     * @brief Job structure for hierarchical srdag transformation
     */
    struct SRDAGTransfoJob {
        const PiSDFGraph *reference = nullptr;
        std::uint32_t srdagIx = UINT32_MAX;
        std::uint32_t firingCount = 0;

        explicit SRDAGTransfoJob(const PiSDFGraph *reference,
                                 std::uint32_t srdagIx = UINT32_MAX,
                                 std::uint32_t firingCount = 0) : reference{reference},
                                                                  srdagIx{srdagIx},
                                                                  firingCount{firingCount} {

        }
    };

    Spider::vector<SRDAGTransfoJob> jobs_;

    /**
     * @brief Structure used during the SRLinkage containing every information of a given edge.
     */
    struct EdgeLinker {
        PiSDFAbstractVertex  *source = nullptr;
        PiSDFAbstractVertex *sink = nullptr;
        std::int64_t sourceRate = 0;
        std::int64_t sinkRate = 0;
        std::int64_t delay = 0;
        std::uint32_t sourceCount = 0;
        std::uint32_t sinkCount = 0;
        std::uint32_t sinkPortIx = 0;
        std::uint32_t sourcePortIx = 0;
        Spider::Array<PiSDFVertex *> *sourceArray = nullptr;
        Spider::Array<PiSDFVertex *> *sinkArray = nullptr;

        EdgeLinker(const PiSDFEdge *edge,
                   Spider::Array<PiSDFVertex *> &sourceArray,
                   Spider::Array<PiSDFVertex *> &sinkArray,
                   std::int64_t sourceRate,
                   std::int64_t sinkRate) : source{edge->source()},
                                             sink{edge->sink()},
                                             sourceRate{sourceRate},
                                             sinkRate{sinkRate},
                                             sourceArray{&sourceArray},
                                             sinkArray{&sinkArray} {
            delay = edge->delay()->value();
            sourcePortIx = edge->sourcePortIx();
            sinkPortIx = edge->sinkPortIx();
        }

        ~EdgeLinker() = default;
    };

    /**
     * @brief Structure used during the SRLinkage containing information about a sink.
     */
    struct SinkLinker {
        PiSDFVertex *vertex = nullptr;
        std::int32_t sinkPortIx = 0;
        std::int64_t sinkRate = 0;
        std::int64_t lowerDep = 0;
        std::int64_t upperDep = 0;

        SinkLinker() = default;

        ~SinkLinker() = default;
    };

    /* === Private method(s) === */

    /**
     * @brief Copy a @refitem PiSDFVertex and return the copy.
     * @remark Hierarchical vertices are copied as normal vertices.
     * @param vertex    Vertex to copy.
     * @param instance  Instance of the vertex.
     * @return copied @refitem PiSDFVertex.
     */
    PiSDFVertex *copyVertex(const PiSDFVertex *vertex, std::uint32_t instance = 0, const std::string &prefix = "");

    /**
     * @brief Copy all vertices of a graph w.r.t their repetition values and do the single-rate DAG linkage.
     * @param job Current job to transform.
     * @throws @refitem Spider::Exception if interfaces inner / outer rates do not match
     */
    void extractAndLinkActors(SRDAGTransfoJob &job);

    /**
     * @brief Perform Single-Rate linkage for a given edge.
     * @param edgeLinker @refitem EdgeLinker structure reference containing information needed for the linkage.
     */
    void singleRateLinkage(EdgeLinker &edgeLinker);

    /**
     * @brief Creates the array of single rate sources (including init vertex).
     * @param edgeLinker       @refitem EdgeLinker structure reference containing information needed for the linkage.
     * @param sourceLinkArray  Array of single rate sources.
     */
    void buildSourceLinkArray(EdgeLinker &edgeLinker, Spider::Array<PiSDFVertex *> &sourceLinkArray);

    /**
     * @brief Creates the array of single rate sinks (including end vertex).
     * @param edgeLinker     @refitem EdgeLinker structure reference containing information needed for the linkage.
     * @param sinkLinkArray  Array of single rate sources.
     */
    void buildSinkLinkArray(EdgeLinker &edgeLinker, Spider::Array<SinkLinker> &sinkLinkArray);

    /**
     * @brief Remove the init introduced during transformation and connect the edge to the setter actor (if any).
     * @param edge        Original edge with the delay.
     * @param delayVertex Virtual delay vertex.
     * @param sink        Single rate sink connected to the Init vertex.
     */
    void reconnectSetter(const PiSDFEdge *edge, PiSDFVertex *delayVertex, PiSDFVertex *sink);

    /**
     * @brief Remove the end introduced during transformation and connect the edge to the getter actor (if any).
     * @param edge        Original edge with the delay.
     * @param delayVertex Virtual delay vertex.
     * @param source      First single rate source connected to the End vertex.
     */
    void reconnectGetter(const PiSDFEdge *edge, PiSDFVertex *delayVertex, PiSDFVertex *source);
};

/* === Inline method(s) === */

PiSDFGraph *StaticSRDAGTransformer::srdag() const {
    return srdag_;
}

#endif //SPIDER2_STATICSRDAGTRANSFORMER_H
