/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
#ifndef SPIDER2_GRAPHFIRING_H
#define SPIDER2_GRAPHFIRING_H

/* === Include(s) === */

#include <common/Types.h>
#include <memory/unique_ptr.h>
#include <containers/vector.h>
#include <containers/array.h>

namespace spider {

    namespace pisdf {
        class Graph;

        class Param;
    }

    namespace srless {

        class GraphHandler;

        class GraphFiring;

        /* === Class definition === */

        class GraphFiring {
        public:
            GraphFiring(const GraphHandler *parent,
                        const spider::vector<std::shared_ptr<pisdf::Param>> &params,
                        u32 firing);

            GraphFiring(GraphFiring &&) = default;

            GraphFiring(const GraphFiring &) = default;

            GraphFiring &operator=(GraphFiring &&) = default;

            GraphFiring &operator=(const GraphFiring &) = default;

            ~GraphFiring();

            /* === Method(s) === */

            /**
             * @brief Registers the Task ix for a given firing of a given vertex.
             * @param vertex  Pointer to the vertex.
             * @param firing  Value of the firing.
             * @param taskIx  Value of the task ix to set.
             */
            void registerTaskIx(const pisdf::Vertex *vertex, u32 firing, u32 taskIx);

            /**
             * @brief Compute BRV and save the values based on current value of parameters.
             * @remark this method automatically set the resolved_ flag to true.
             */
            void resolveBRV();

            /**
             * @brief Clears every values, and set resolved_ flag to false.
             */
            void clear();

            int64_t getSourceRate(const pisdf::Edge *edge) const;

            int64_t getSinkRate(const pisdf::Edge *edge) const;

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
            inline const spider::array<GraphHandler *> &subgraphFirings() const { return subgraphHandlers_; }

            /**
             * @brief non const overload of the @refitem GraphFiring::subgraphFirings method.
             * @return non const reference to the array of subgraphs GraphFiring.
             */
            inline spider::array<GraphHandler *> &subgraphFirings() { return subgraphHandlers_; }

            /**
             * @brief Get the parameters of this graph firing.
             * @return parameters vector.
             */
            inline const spider::vector<std::shared_ptr<pisdf::Param>> &getParams() const { return params_; }

            inline u32 firingValue() const { return firing_; }

            /**
             * @brief Get the status of this graph firing (resolved or not).
             * @return true if the graph firing is resolved (i.e its parameters are set), false else.
             */
            inline bool isResolved() const { return resolved_; }

            /**
             * @brief Get the repetition value of a vertex for this graph firing.
             * @param vertex Pointer to the vertex.
             * @return repetition value of the vertex in this graph firing.
             * @warning if this graph firing has not yet been resolved, value should be UINT32_MAX but it is not guarenteed.
             * @throw @refitem spider::Exception if vertex does belong to the graph associated to this graph firing.
             */
            u32 getRV(const pisdf::Vertex *vertex) const;

            /**
             * @brief Get the task index associated with a given firing of a given vertex for this graph firing.
             * @param vertex  Pointer to the vertex.
             * @param firing  Firing of the vertex.
             * @return value of the task index.
             * @warning if this graph firing has not yet been resolved, value should be UINT32_MAX but it is not guarenteed.
             * @throw @refitem spider::Exception if firing is greater or equal to the repetition value of the vertex.
             */
            u32 getTaskIx(const pisdf::Vertex *vertex, u32 firing) const;

            /**
             * @brief Get the @refitem GraphFiring of a subgraph in this graph firing context.
             * @param subgraph Pointer to the subgraph.
             * @param firing   Firing of the subgraph to fetch.
             * @return pointer to the subgraph @refitem GraphFiring (if resolved).
             * @throw @refitem spider::Exception if subgraph does belong to the graph associated to this graph firing.
             */
            const GraphFiring *getSubgraphGraphFiring(const pisdf::Graph *subgraph, u32 firing) const;

            /**
             * @brief Get the parameter value of index ix.
             * @param ix Index of the parameter.
             * @return parameter value.
             */
            int64_t getParamValue(size_t ix);

            /* === Setter(s) === */

            /**
             * @brief Set the parameter value of param of index ix.
             * @param ix    Index of the parameter.
             * @param value Value of the parameter to be set.
             */
            void setParamValue(size_t ix, int64_t value);

        private:
            struct EdgeRate {
                int64_t srcRate_;
                int64_t snkRate_;
            };
            spider::vector<std::shared_ptr<pisdf::Param>> params_;
            spider::array<GraphHandler *> subgraphHandlers_; /* == match between subgraphs and their handler == */
            spider::array<u32> brv_; /* == BRV of this firing of the graph == */
            spider::array<u32 *> taskIxRegister_;
            spider::array<EdgeRate> rates_;
            const GraphHandler *parent_;
            u32 firing_{ };
            u32 dynamicParamCount_{ };
            u32 paramResolvedCount_{ };
            bool resolved_;

            /* === private method(s) === */

            std::shared_ptr<pisdf::Param> copyParameter(const std::shared_ptr<pisdf::Param> &param);
        };
    }
}

#endif //SPIDER2_GRAPHFIRING_H
