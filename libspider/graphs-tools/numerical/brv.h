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

#include <containers/containers.h>
#include <containers/array.h>
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
            ConnectedComponent() = default;

            ConnectedComponent(const ConnectedComponent &) = default;

            ConnectedComponent(ConnectedComponent &&) = default;

            ~ConnectedComponent() = default;

            /* === Member(s) === */

            stack_vector(vertexVector_, pisdf::Vertex*, StackID::TRANSFO);
            size_t edgeCount_ = 0;
        };


        /* === Function(s) prototype === */

        /**
         * @brief Compute the repetition vector of a graph using specified parameters values.
         * @remark This function uses the LCM based method to compute the repetition vector.
         * @param graph   Graph to evaluate.
         * @param params  Parameters to use for the rates evaluation. (should contain the same parameters as the graphs)
         */
        void compute(const pisdf::Graph *graph, const spider::vector<pisdf::Param *> &params);

        /**
         * @brief Compute the repetition vector of a graph using its parameters values as default.
         * @remark this function calls @refitem compute as compute(graph, graph->params());
         * @param graph Graph to evaluate.
         */
        void compute(const pisdf::Graph *graph);

        /**
         * @brief Extract the different connected components of a graph.
         * @param graph Graph to evaluate.
         * @return vector of @refitem ConnectedComponents.
         * @throws @refitem spider::Exception.
         */
        spider::vector<ConnectedComponent> extractConnectedComponents(const pisdf::Graph *graph);

        /**
         * @brief Extract the edges from a connected component.
         * @param component  Connected component to evaluate.
         * @return array of edge.
         */
        spider::array<const pisdf::Edge *> extractEdgesFromComponent(const ConnectedComponent &component);

        /**
         * @brief Extract Rationals of rates of a connected component using its edges.
         * @param rationalArray Array of @refitem Rational to be set.
         * @param edgeArray     Edges of the connected component we are evaluating.
         * @param params        Parameters used for rate evaluation.
         * @throws @refitem spider::Exception
         */
        void extractRationalsFromEdges(spider::array<spider::Rational> &rationalArray,
                                       const spider::array<const pisdf::Edge *> &edgeArray,
                                       const spider::vector<pisdf::Param *> &params);

        /**
         * @brief Update repetition values of a given connected component.
         * @param component Connected component to evaluate.
         * @param params    Parameters value to use.
         */
        void updateBRV(const ConnectedComponent &component, const spider::vector<pisdf::Param *> &params);

        /**
         * @brief Check the consistency (see consistency property of SDF graph) of a connected component.
         * @param edgeArray  Edges of the connected component.
         * @param params     Parameters value to use.
         * @throws @refitem spider::Exception.
         */
        void checkConsistency(const spider::array<const pisdf::Edge *> &edgeArray,
                              const spider::vector<pisdf::Param *> &params);

        /**
         * @brief Print the repetition vector of a given graph (only if verbose is enabled)
         * @param graph  Graph to print.
         */
        void print(const pisdf::Graph *graph);
    }
}

#endif //SPIDER2_BRV_H
