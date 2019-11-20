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
#ifndef SPIDER2_HELPER_H
#define SPIDER2_HELPER_H

/* === Include(s) === */

#include <graphs/pisdf/specials/Specials.h>
#include <containers/StlContainers.h>
#include <graphs-tools/transformation/srdag/TransfoJob.h>
#include <graphs-tools/transformation/srdag/TransfoData.h>

namespace spider {
    namespace srdag {

        /* === Structure definition(s) === */

        struct TransfoVertex {
            std::int64_t rate_ = -1;
            std::uint32_t portIx_ = UINT32_MAX;
            PiSDFAbstractVertex *vertex_ = nullptr;
            std::uint32_t lowerDep_ = UINT32_MAX;
            std::uint32_t upperDep_ = 0;

            TransfoVertex() = default;

            TransfoVertex(std::int64_t rate, std::uint32_t portIx, PiSDFAbstractVertex *vertex) : rate_{ rate },
                                                                                                  portIx_{ portIx },
                                                                                                  vertex_{ vertex } { }
        };

        /* === Type definition(s) === */

        using TransfoStack = spider::vector<TransfoVertex>;

        /* === Function(s) prototype === */

        /**
         * @brief  Copy a vertex in the srdag as many times as its repetition value.
         * @param vertex      Vertex to copy.
         * @param transfoData  Current transformation job.
         */
        void copyFromRV(PiSDFAbstractVertex *vertex, TransfoData &transfoData);

        void fillLinkerVector(TransfoStack &vector,
                              PiSDFAbstractVertex *reference,
                              std::int64_t rate,
                              std::uint32_t portIx,
                              TransfoData &transfoData);

        /**
         * @brief Add a @refitem ForkVertex into the single-rate graph and connect it.
         * @param srcVector   Vector of @refitem TransfoVertex.
         * @param snkVector   Vector of @refitem TransfoVertex.
         * @param srdag       Single-Rate graph.
         */
        void addForkVertex(TransfoStack &srcVector, TransfoStack &snkVector, PiSDFGraph *srdag);

        /**
         * @brief Add a @refitem JoinVertex into the single-rate graph and connect it.
         * @param srcVector   Vector of @refitem TransfoVertex.
         * @param snkVector   Vector of @refitem TransfoVertex.
         * @param srdag       Single-Rate graph.
         */
        void addJoinVertex(TransfoStack &srcVector, TransfoStack &snkVector, PiSDFGraph *srdag);

        /**
         * @brief Insert @refitem UpSampleVertex for every input interface and @refitem TailVertex for
         *        every output interface of current job.
         * @param transfoData Current @refitem TransfoData.
         */
        void replaceJobInterfaces(TransfoData &transfoData);

        /**
         * @brief Compute all real dependencies of current instances of source / sink of the edge.
         * @param srcVector   Vector of @refitem TransfoVertex corresponding to the sources of the edge of the job.
         * @param snkVector   Vector of @refitem TransfoVertex corresponding to the sinks of the edge of the job.
         * @param transfoData Current @refitem TransfoData.
         */
        void computeEdgeDependencies(TransfoStack &srcVector, TransfoStack &snkVector, TransfoData &transfoData);
    }
}

#endif //SPIDER2_HELPER_H
