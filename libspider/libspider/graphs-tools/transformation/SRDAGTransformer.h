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

    /* === Private method(s) === */

    PiSDFVertex *copyVertex(const PiSDFVertex *vertex, std::uint32_t instance = 0);

    void extractConfigActors(const PiSDFGraph *graph);

    void extractAndLinkActors(const PiSDFGraph *graph);

    PiSDFVertex *createFork(const PiSDFEdge *edge,
                            std::int64_t sourceRate,
                            std::int64_t nConsumer,
                            Spider::Array<PiSDFVertex *> &sourceArray,
                            std::uint32_t sourceCount);

    PiSDFVertex *createJoin(const PiSDFEdge *edge,
                            std::int64_t sinkRate,
                            std::int64_t nProducer,
                            Spider::Array<PiSDFVertex *> &sinkArray,
                            std::uint32_t sinkCount);

//    void linkInstanceOfActor(const PiSDFVertex *vertex,
//                             PiSDFVertex *copyVertex,
//                             std::uint32_t instance,
//                             Spider::Array<const PiSDFVertex *> &doneVertexArray);
};

/* === Inline method(s) === */

const PiSDFGraph *SRDAGTransformer::srdag() const {
    return srdag_;
}


#endif //SPIDER2_SRDAGTRANSFORMER_H
