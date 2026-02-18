/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_GRAPHFIRING_H
#define SPIDER2_GRAPHFIRING_H

/* === Include(s) === */

#include <common/Types.h>
#include <memory/unique_ptr.h>
#include <runtime/common/Fifo.h>
#include <containers/vector.h>
#include <containers/array.h>
#include <graphs-tools/numerical/detail/DependencyIterator.h>

namespace spider {

    namespace sched {
        class PiSDFTask;
    }

    namespace pisdf {

        class Graph;

        class Param;

        class GraphHandler;

        class GraphFiring;

        class GraphAlloc;

        /* === Class definition === */

        class GraphFiring {
        public:
            GraphFiring(const GraphHandler *parent,
                        const spider::vector<std::shared_ptr<Param>> &params,
                        u32 firing);

            GraphFiring(GraphFiring &&) = default;

            GraphFiring(const GraphFiring &) = delete;

            GraphFiring &operator=(GraphFiring &&) = default;

            GraphFiring &operator=(const GraphFiring &) = delete;

            ~GraphFiring();

            /* === Method(s) === */

            /**
             * @brief Compute BRV and save the values based on current value of parameters.
             * @remark this method automatically set the resolved_ flag to true.
             */
            void resolveBRV();

            /**
             * @brief Clears every values, and set resolved_ flag to false.
             */
            void clear();

            /* === Getter(s) === */

            /**
             * @brief Get the @refitem GraphHandler holding this graph firing.
             * @return const pointer to the GraphHandler of this graph firing.
             */
            inline const GraphHandler *getParent() const { return parent_; }

            /**
             * @brief Get the subgraphs @refitem GraphFiring.
             * @return const reference to the array of subgraphs GraphFiring.
             */
            spider::array_view<GraphHandler *> subgraphFirings() const;

            /**
             * @brief non const overload of the @refitem GraphFiring::subgraphFirings method.
             * @return non const reference to the array of subgraphs GraphFiring.
             */
            spider::array_view<GraphHandler *> subgraphHandlers();

            /**
             * @brief Get the firing value of this GraphFiring.
             * @return firing value.
             */
            inline u32 firingValue() const { return firing_; }

            /**
             * @brief Get the status of this graph firing (resolved or not).
             * @return true if the graph firing is resolved (i.e its parameters are set), false else.
             */
            inline bool isResolved() const { return resolved_; }

            /**
             * @brief Get resolved source rate of a given edge.
             * @param edge Pointer to the edge.
             * @return value of the source rate.
             * @throw @refitem spider::Exception in DEBUG only if edge does not belong to the graph.
             */
            int64_t getSrcRate(const Edge *edge) const;

            /**
             * @brief Get resolved sink rate of a given edge.
             * @param edge Pointer to the edge.
             * @return value of the sink rate.
             * @throw @refitem spider::Exception in DEBUG only if edge does not belong to the graph.
             */
            int64_t getSnkRate(const Edge *edge) const;

            /**
             * @brief Get the repetition value of a vertex for this graph firing.
             * @param vertex Pointer to the vertex.
             * @return repetition value of the vertex in this graph firing.
             * @warning if this graph firing has not yet been resolved, value should be UINT32_MAX but it is not guarenteed.
             * @throw @refitem spider::Exception if vertex does belong to the graph associated to this graph firing.
             */
            u32 getRV(const Vertex *vertex) const;

            /**
             * @brief Get the @refitem GraphFiring of a subgraph in this graph firing context.
             * @param subgraph Pointer to the subgraph.
             * @param firing   Firing of the subgraph to fetch.
             * @return pointer to the subgraph @refitem GraphFiring (if resolved).
             * @throw @refitem spider::Exception if subgraph does belong to the graph associated to this graph firing.
             */
            const GraphFiring *getSubgraphGraphFiring(const Graph *subgraph, u32 firing) const;

            /**
             * @brief Get the parameters of this graph firing.
             * @return parameters vector.
             */
            const spider::vector<std::shared_ptr<Param>> &getParams() const;

            /**
             * @brief Get a given vertex from its graph identifier.
             * @param ix  Graph identifier of the vertex in its containing graph.
             * @return const pointer to the vertex.
             */
            const Vertex *vertex(size_t ix) const;

            /**
             * @brief Get a given vertex from its graph identifier.
             * @param ix  Graph identifier of the vertex in its containing graph.
             * @return pointer to the vertex.
             */
            Vertex *vertex(size_t ix);

            sched::PiSDFTask *getTask(const Vertex *vertex) const;

            u32 getTaskIx(const Vertex *vertex, u32 firing) const;

            u32 getTaskIx(u32 vertexIx, u32 firing) const;

            const u32 *getTaskIndexes(const Vertex *vertex) const;

            const u32 *getTaskIndexes(u32 vertexIx) const;

            size_t getEdgeAddress(const Edge *edge, u32 firing) const;

            u32 getEdgeOffset(const Edge *edge, u32 firing) const;

            u32 getEdgeDepCount(const Vertex *vertex, const Edge *edge, u32 firing) const;

            /* === Setter(s) === */

            /**
             * @brief Set the parameter value of param of index ix.
             * @param ix    Index of the parameter.
             * @param value Value of the parameter to be set.
             */
            void setParamValue(size_t ix, int64_t value);

            void setTaskIx(const Vertex *vertex, u32 firing, u32 taskIx);

            void setEdgeAddress(size_t value, const Edge *edge, u32 firing);

            void setEdgeOffset(u32 value, const Edge *edge, u32 firing);

            void setEdgeDepCount(const Vertex *vertex, const Edge *edge, u32 firing, u32 value);

        private:
            struct EdgeRate {
                int64_t srcRate_;
                int64_t snkRate_;
            };
            spider::vector<std::shared_ptr<Param>> params_;
            spider::unique_ptr<GraphHandler *> subgraphHandlers_;  /* == match between subgraphs and their handler == */
            spider::unique_ptr<u32> brvArray_;                     /* == BRV of this firing of the graph == */
            spider::unique_ptr<EdgeRate> ratesArray_;              /* == Array of resolved rates (trade some memory for runtime speed) == */
            spider::unique_ptr<GraphAlloc> alloc_;                 /* == Class used to handle everything related to resource allocation == */
            spider::unique_ptr<u32 *> depsCountArray_;             /* == Array of dependencies count == */
            const GraphHandler *parent_;                           /* == Parent handler == */
            u32 firing_ = 0;                                       /* == Firing of this graph instance == */
            u32 dynamicParamCount_ = 0;                            /* == Number of dynamic parameters == */
            u32 paramResolvedCount_ = 0;                           /* == Number of resolved parameters == */
            bool resolved_;                                        /* == Indicates that the graph has been resolved == */

            /* === private method(s) === */

            void resolveDynamicDependentParams() const;

            std::shared_ptr<pisdf::Param> copyParameter(const std::shared_ptr<pisdf::Param> &param);

            void updateFromRV(const pisdf::Vertex *vertex, u32 rv);

            void createOrUpdateSubgraphHandlers();
        };
    }
}

#endif //SPIDER2_GRAPHFIRING_H
