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
#ifndef SPIDER2_BRV_H
#define SPIDER2_BRV_H

/* === Include(s) === */

#include <containers/array.h>
#include <containers/vector.h>
#include <common/Rational.h>

namespace spider {

    /* === Forward declaration(s) === */

    namespace pisdf {

        class Graph;

        class Param;

        class Vertex;

        class Edge;
    }

    namespace brv {

        /* === Structure(s) definition === */

        struct ConnectedComponent {
            size_t edgeCount_ = 0;
            size_t count_ = 0;
            size_t offset_ = 0;
            bool hasInterfaces_ = false;
            bool hasConfig_ = false;
        };

        struct BRVHandler {
            explicit BRVHandler(size_t vertexCount, size_t edgeCount) :
                    vertexVector_{ factory::vector<pisdf::Vertex *>(StackID::TRANSFO) },
                    rationalVector_{ factory::vector<Rational>(vertexCount, StackID::TRANSFO) },
                    visitedVertices_{ factory::vector<bool>(vertexCount, false, StackID::TRANSFO) },
                    visitedEdges_{ factory::vector<bool>(edgeCount, false, StackID::TRANSFO) } {
                vertexVector_.reserve(vertexCount);
            }

            BRVHandler(const BRVHandler &) = default;

            BRVHandler(BRVHandler &&) = default;

            ~BRVHandler() = default;

            vector<pisdf::Vertex *> vertexVector_; /* = Vector used to handle multiple connected components = */
            vector<Rational> rationalVector_;      /* = Vector used to store Rationals of every vertices = */
            vector<bool> visitedVertices_;         /* = Vector used to keep track of already visited vertices = */
            vector<bool> visitedEdges_;            /* = Vector used to keep track of visited edges = */
        };


        /* === Function(s) prototype === */

        /**
         * @brief Compute the repetition vector of a graph using specified parameters values.
         * @remark This function uses the LCM based method to compute the repetition vector.
         * @param graph   Graph to evaluate.
         * @param params  Parameters to use for the rates evaluation. (should contain the same parameters as the graphs)
         */
        void compute(const pisdf::Graph *graph, const spider::vector<std::shared_ptr<pisdf::Param>> &params);

        /**
         * @brief Compute the repetition vector of a graph using its parameters values as default.
         * @remark this function calls @refitem compute as compute(graph, graph->params());
         * @param graph Graph to evaluate.
         */
        void compute(const pisdf::Graph *graph);

        /**
         * @brief Print the repetition vector of a given graph (only if verbose is enabled)
         * @param graph  Graph to print.
         */
        void print(const pisdf::Graph *graph);
    }
}

#endif //SPIDER2_BRV_H
