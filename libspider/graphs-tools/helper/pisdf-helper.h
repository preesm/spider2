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
#ifndef SPIDER2_PISDF_HELPER_H
#define SPIDER2_PISDF_HELPER_H

/* === Include(s) === */

#include <containers/array.h>
#include <containers/vector.h>

/* === Function(s) prototype === */

namespace spider {
    namespace pisdf {

        /* === Forward declaration(s) === */

        class Graph;

        class Vertex;

        class Param;

        /* === Function(s) prototype === */

        /**
         * @brief Recursively check if a graph is completly static or not.
         * @param graph Pointer to the graph.
         * @return true if the graph is fully static, false else.
         */
        bool isGraphFullyStatic(const Graph *graph);

        /**
         * @brief Recursively split dynamic graphs to separate init from run sections of the graph.
         * @param graph  Pointer to the top graph.
         */
        void recursiveSplitDynamicGraph(Graph *graph);

        /**
         * @brief Creates an array with parameters needed for the runtime exec of a vertex.
         * @param vertex Pointer to the vertex.
         * @return array of int_least_64_t.
         */
        array<i64> buildVertexRuntimeInputParameters(const pisdf::Vertex *vertex);

        /**
         * @brief Creates an array with parameters needed for the runtime exec of a vertex.
         * @param vertex  Pointer to the vertex.
         * @param params  Parameters to use for the rates evaluation. (should contain the same parameters as the graphs)
         * @return array of int_least_64_t.
         */
        array<i64> buildVertexRuntimeInputParameters(const pisdf::Vertex *vertex,
                                                     const spider::vector<std::shared_ptr<pisdf::Param>> &params);

        /**
         * @brief Get the source of the vertex across interfaces.
         * @example V ->  H -> V2
         *             | -> |
         *          V2 direct source is H, but real indirect source is V.
         * @param vertex  Pointer to the vertex.
         * @param ix      Index of the input edge.
         * @return pointer to the indirect real source of the vertex on given edge.
         * @throws std::out_of_range
         */
        pisdf::Vertex *getIndirectSource(const pisdf::Vertex *vertex, size_t ix);

        /**
         * @brief Get the source of the vertex across interfaces.
         * @example V ->  H -> V2
         *             | -> |
         *          V direct sink is H, but real indirect source is V2.
         * @param vertex  Pointer to the vertex.
         * @param ix      Index of the output edge.
         * @return pointer to the indirect real sink of the vertex on given edge.
         * @throws std::out_of_range
         */
        pisdf::Vertex *getIndirectSink(const pisdf::Vertex *vertex, size_t ix);
    }
}
#endif //SPIDER2_PISDF_HELPER_H
