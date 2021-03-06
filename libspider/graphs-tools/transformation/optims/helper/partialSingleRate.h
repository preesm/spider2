/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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
#ifndef SPIDER2_PARTIALSINGLERATE_H
#define SPIDER2_PARTIALSINGLERATE_H

#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <common/Types.h>
#include <containers/array.h>

namespace spider {

    /* === Forward declaration(s) === */

    namespace srdag {
        class Vertex;

        class Graph;
    }

    /* === Struct(s) definition === */

    struct EdgeLinker {
        srdag::Vertex *vertex_ = nullptr;
        int64_t rate_ = 0;
        size_t portIx_ = 0;

        EdgeLinker() = default;

        EdgeLinker(srdag::Vertex *vertex, int64_t rate, size_t portIx) : vertex_{ vertex },
                                                                         rate_{ rate },
                                                                         portIx_{ portIx } { };
    };

    /* === Function(s) prototype === */

    /**
     * @brief Perform partial single rate linkage between an array of source actors and an array of sink actors.
     * @param graph         Pointer to the graph.
     * @param sourceArray   Reference to the array of sources (see @refitem EdgeLinker).
     * @param sinkArray     Reference to the array of sinks (see @refitem EdgeLinker).
     */
    void partialSingleRateTransformation(srdag::Graph *graph,
                                         array<EdgeLinker> &sourceArray,
                                         array<EdgeLinker> &sinkArray);
}
#endif
#endif //SPIDER2_PARTIALSINGLERATE_H
