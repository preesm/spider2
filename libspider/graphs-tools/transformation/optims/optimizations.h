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
#ifndef SPIDER2_OPTIMIZATIONS_H
#define SPIDER2_OPTIMIZATIONS_H

/* === Include(s) === */

namespace spider {

    namespace pisdf {
        class Graph;
    }

    namespace optims {

        /* === Function(s) prototype === */

        /**
         * @brief Optimize a given @refitem pisdf::Graph with all available optimizations.
         * @remark if graph is nullptr, nothing happen.
         * @param graph Pointer to the graph to be optimized.
         */
        void optimize(pisdf::Graph *graph);

        /**
         * @brief Optimize Repeat -> Fork patterns in a @refitem pisdf::Graph.
         * @param graph Pointer to the graph to be optimized.
         * @return true if optimization was applied, false else.
         */
        bool reduceRepeatFork(pisdf::Graph *graph);

        /**
         * @brief Optimize Fork -> Fork patterns in a @refitem pisdf::Graph.
         * @see: https://tel.archives-ouvertes.fr/tel-01301642
         * @param graph Pointer to the graph to be optimized.
         * @return true if optimization was applied, false else.
         */
        bool reduceForkFork(pisdf::Graph *graph);

        /**
         * @brief Optimize Join -> Fork patterns in a @refitem pisdf::Graph.
         * @see: https://tel.archives-ouvertes.fr/tel-01301642
         * @param graph Pointer to the graph to be optimized.
         * @return true if optimization was applied, false else.
         */
        bool reduceJoinFork(pisdf::Graph *graph);

        /**
         * @brief Optimize Join -> Join patterns in a @refitem pisdf::Graph.
         * @see: https://tel.archives-ouvertes.fr/tel-01301642
         * @param graph Pointer to the graph to be optimized.
         * @return true if optimization was applied, false else.
         */
        bool reduceJoinJoin(pisdf::Graph *graph);

        /**
         * @brief Optimize Join -> End patterns in a @refitem pisdf::Graph.
         * @see: https://tel.archives-ouvertes.fr/tel-01301642
         * @param graph Pointer to the graph to be optimized.
         * @return true if optimization was applied, false else.
         */
        bool reduceJoinEnd(pisdf::Graph *graph);

        /**
         * @brief Optimize Init -> End patterns in a @refitem pisdf::Graph.
         * @see: https://tel.archives-ouvertes.fr/tel-01301642
         * @param graph Pointer to the graph to be optimized.
         * @return true if optimization was applied, false else.
         */
        bool reduceInitEnd(pisdf::Graph *graph);

        /**
         * @brief Optimize a @refitem pisdf::Graph by removing useless special actors.
         *        detail:    --> Fork      --> : removes fork with 1 output edge
         *                   --> Duplicate --> : removes duplicate with 1 input edge if rate_in == rate_out
         *                   --> Join      --> : removes join with 1 input edge
         *                   --> Tail      --> : removes tail with 1 input edge if rate_in == rate_out
         *                   --> Head      --> : removes head with 1 input edge if rate_in == rate_out
         *                   --> Repeat    --> : removes repeat if rate_in == rate_out
         * @see: https://tel.archives-ouvertes.fr/tel-01301642
         * @param graph Pointer to the graph to be optimized.
         * @return true if optimization was applied, false else.
         */
        bool reduceUnitaryRateActors(pisdf::Graph *graph);
    }
}

#endif //SPIDER2_OPTIMIZATIONS_H
