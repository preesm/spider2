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
#ifndef SPIDER2_SRDAGTRANSFOHELPER_H
#define SPIDER2_SRDAGTRANSFOHELPER_H

/* === Include(s) === */

#include <graphs/pisdf/Types.h>
#include <containers/StlContainers.h>

namespace Spider {
    namespace SRDAG {

        /* === Forward declaration(s) === */

        struct Job;

        struct VertexLinker;

        struct JobLinker;

        /* === Type definition(s) === */

        using JobStack = Spider::vector<Job, StackID::TRANSFO>;

        using TransfoTracker = Spider::vector<std::uint32_t, StackID::TRANSFO>;

        using LinkerVector = Spider::vector<VertexLinker, StackID::TRANSFO>;

        /* === Function(s) prototype === */

        PiSDFAbstractVertex *fetchOrClone(const PiSDFAbstractVertex *vertex, JobLinker &linker);

        void fillLinkerVector(LinkerVector &vector,
                              const PiSDFAbstractVertex *reference,
                              const std::int64_t &rate,
                              const std::uint32_t &portIx,
                              JobLinker &linker);

        /**
         * @brief Add a @refitem ForkVertex into the single-rate graph and connect it.
         * @param srcVector   Vector of @refitem VertexLinker.
         * @param snkVector   Vector of @refitem VertexLinker.
         * @param srdag       Single-Rate graph.
         */
        void addForkVertex(LinkerVector &srcVector, LinkerVector &snkVector, PiSDFGraph *srdag);

        /**
         * @brief Add a @refitem JoinVertex into the single-rate graph and connect it.
         * @param srcVector   Vector of @refitem VertexLinker.
         * @param snkVector   Vector of @refitem VertexLinker.
         * @param srdag       Single-Rate graph.
         */
        void addJoinVertex(LinkerVector &srcVector, LinkerVector &snkVector, PiSDFGraph *srdag);

        /**
         * @brief Insert @refitem UpSampleVertex for every input interface and @refitem TailVertex for
         *        every output interface of current job.
         * @param linker Current @refitem JobLinker.
         */
        void replaceJobInterfaces(JobLinker &linker);

        /**
         * @brief Compute all real dependencies of current instances of source / sink of the edge.
         * @param srcVector  Vector of @refitem VertexLinker corresponding to the sources of the edge of the job.
         * @param snkVector  Vector of @refitem VertexLinker corresponding to the sinks of the edge of the job.
         * @param linker     Current @refitem JobLinker.
         */
        void computeEdgeDependencies(LinkerVector &srcVector, LinkerVector &snkVector, JobLinker &linker);
    }
}

#endif //SPIDER2_SRDAGTRANSFOHELPER_H
