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
#ifndef SPIDER2_DEPENDENCIES_H
#define SPIDER2_DEPENDENCIES_H

/* === Includes === */

#include <common/Types.h>
#include <graphs-tools/numerical/detail/DependencyIterator.h>

/* === Methods prototype === */

/* === Inline function(s) === */

namespace spider {

    namespace srless {
        class GraphFiring;
    }

    namespace pisdf {

        class Vertex;

        /**
         * @brief Compute execution dependencies for a given input edge of a given firing of a given vertex.
         *        Execution dependencies corresponds to all the vertices (and their firing) that are directly needed for
         *        this particular input edge and this firing of the vertex inside the graph firing specified by the handler.
         *        Dependencies are computed through input / output interfaces and through delays.
         * @param vertex  Pointer to the vertex.
         * @param firing  Firing of the vertex.
         * @param edgeIx  Index of the input edge.
         * @param handler Pointer to the @refitem srless::GraphFiring to which this particular firing of the vertex belongs.
         * @param count   If not nullptr, this will be filled with the number of consumer dependencies.
         * @return a @refitem DependencyIterator to the execution dependencies of the vertex.
         * @throw nullpointer exception in DEBUG mode only if handler or vertex is nullptr.
         */
        DependencyIterator computeExecDependency(const Vertex *vertex,
                                                 u32 firing,
                                                 size_t edgeIx,
                                                 const srless::GraphFiring *handler,
                                                 i32 *count = nullptr);

        /**
         * @brief Compute the number of execution dependencies for a given input edge of a given firing of a given vertex.
         *        Execution dependencies corresponds to all the vertices (and their firing) that are directly needed for
         *        this particular input edge and this firing of the vertex inside the graph firing specified by the handler.
         *        Dependencies are computed through input / output interfaces and through delays.
         * @param vertex  Pointer to the vertex.
         * @param firing  Firing of the vertex.
         * @param edgeIx  Index of the input edge.
         * @param handler Pointer to the @refitem srless::GraphFiring to which this particular firing of the vertex belongs.
         * @return the exec dependency count.
         * @throw nullpointer exception in DEBUG mode only if handler or vertex is nullptr.
         */
        i32 computeExecDependencyCount(const Vertex *vertex,
                                       u32 firing,
                                       size_t edgeIx,
                                       const srless::GraphFiring *handler);

        /**
         * @brief Compute consumer dependencies for a given output edge of a given firing of a given vertex.
         *        Consumer dependencies corresponds to all the vertices (and their firing) that directly depends from
         *        this particular output edge and this firing of the vertex inside the graph firing specified by the handler.
         *        Dependencies are computed through input / output interfaces and through delays.
         * @param vertex  Pointer to the vertex.
         * @param firing  Firing of the vertex.
         * @param edgeIx  Index of the output edge.
         * @param handler Pointer to the @refitem srless::GraphFiring to which this particular firing of the vertex belongs.
         * @param count   If not nullptr, this will be filled with the number of execution dependencies.
         * @return a @refitem DependencyIterator to the consumer dependencies of the vertex.
         * @throw nullpointer exception in DEBUG mode only if handler or vertex is nullptr.
         */
        DependencyIterator computeConsDependency(const Vertex *vertex,
                                                 u32 firing,
                                                 size_t edgeIx,
                                                 const srless::GraphFiring *handler,
                                                 i32 *count = nullptr);

        /**
         * @brief Compute the number of consumer dependencies for a given input edge of a given firing of a given vertex.
         *        Execution dependencies corresponds to all the vertices (and their firing) that are directly needed for
         *        this particular input edge and this firing of the vertex inside the graph firing specified by the handler.
         *        Dependencies are computed through input / output interfaces and through delays.
         * @param vertex  Pointer to the vertex.
         * @param firing  Firing of the vertex.
         * @param edgeIx  Index of the input edge.
         * @param handler Pointer to the @refitem srless::GraphFiring to which this particular firing of the vertex belongs.
         * @return the exec dependency count.
         * @throw nullpointer exception in DEBUG mode only if handler or vertex is nullptr.
         */
        i32 computeConsDependencyCount(const Vertex *vertex,
                                       u32 firing,
                                       size_t edgeIx,
                                       const srless::GraphFiring *handler);

        /**
         * @brief Compute the lower consumption dependencies of a vertex in a flat graph:
         *                     |   k * c - d   |
         *         lower_dep = | ------------- |
         *                     |_      p      _|
         *
         *         with c = consumption, p = production, k = firing and d = delay.
         *
         *        see https://hal-univ-rennes1.archives-ouvertes.fr/hal-02355636 for more details.
         * @remark edge: sourceRate -> delay -> sinkRate
         * @remark If dependency is inferior to 0, it will be bound to -1. Such value means that the instance depends on
         * the delay initialization.
         * @param consumption  Consumption value on the edge.
         * @param production   Production value on the edge.
         * @param firing       Firing of the vertex.
         * @param delay        Value of the delay.
         * @return value of the lower firing dependency on the producer.
         */
        ifast64 computeConsLowerDep(ifast64 consumption, ifast64 production, ifast32 firing, ifast64 delay);

        /**
         * @brief Compute the upper consumption dependencies of a vertex in a flat graph:
         *                     |  (k + 1) * c - d - 1  |
         *         upper_dep = | --------------------- |
         *                     |_         p           _|
         *
         *         with c = consumption, p = production, k = firing and d = delay.
         *
         *        see https://hal-univ-rennes1.archives-ouvertes.fr/hal-02355636 for more details.
         * @remark: edge: sourceRate -> delay -> sinkRate
         * @remark If dependency is inferior to 0, it will be bound to -1. Such value means that the instance depends on
         * the delay initialization.
         * @param consumption  Consumption value on the edge.
         * @param production   Production value on the edge.
         * @param firing       Firing of the vertex.
         * @param delay        Value of the delay.
         * @return value of the upper firing dependency on the producer.
         */
        ifast64 computeConsUpperDep(ifast64 consumption, ifast64 production, ifast32 firing, ifast64 delay);
    }
}

#endif //SPIDER2_DEPENDENCIES_H
