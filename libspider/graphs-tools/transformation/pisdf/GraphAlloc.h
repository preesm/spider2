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
#ifndef SPIDER2_GRAPHALLOC_H
#define SPIDER2_GRAPHALLOC_H

/* === Include(s) === */

#include <common/Types.h>
#include <memory/unique_ptr.h>
#include <runtime/common/Fifo.h>

namespace spider {

    namespace sched {
        class PiSDFTask;
    }

    namespace pisdf {

        class Edge;

        class Vertex;

        class Graph;

        class GraphFiring;

        /* === Class definition === */

        class GraphAlloc {
        public:
            explicit GraphAlloc(const Graph *graph);

            ~GraphAlloc() = default;

            /* === Method(s) === */

            void clear(const Graph *graph);

            void reset(const Graph *graph, const u32 *brv);

            void reset(const Vertex *vertex, u32 rv);

            void initialize(GraphFiring *handler, const Vertex *vertex, u32 rv);

            /* === Getter(s) === */

            /**
             * @brief Get the task index associated with a given firing of a given vertex for this graph firing.
             * @param vertex  Pointer to the vertex.
             * @param firing  Firing of the vertex.
             * @return value of the task index.
             * @warning if this graph firing has not yet been resolved, value should be UINT32_MAX but it is not guarenteed.
             * @throw @refitem spider::Exception if firing is greater or equal to the repetition value of the vertex.
             */
            u32 getTaskIx(const Vertex *vertex, u32 firing) const;

            /**
             * @brief Get the allocated memory address of a given edge.
             * @param edge    Pointer to the edge.
             * @param firing  Firing of the vertex producing on this edge.
             * @return allocated memory address.
             */
            size_t getEdgeAddress(const Edge *edge, u32 firing) const;

            /**
             * @brief Get the offset in the allocated memory address of a given edge.
             * @param edge    Pointer to the edge.
             * @param firing  Firing of the vertex producing on this edge.
             * @return offset in the allocated memory address.
             */
            u32 getEdgeOffset(const Edge *edge, u32 firing) const;

            /**
             * @brief Get the schedule task associated with this vertex.
             * @param vertex  Pointer to the vertex.
             * @return pointer to the schedule task.
             */
            sched::PiSDFTask *getTask(const Vertex *vertex);

            /**
             * @brief Get the schedule task associated with this vertex.
             * @param vertex  Pointer to the vertex.
             * @return pointer to the schedule task.
             */
            const sched::PiSDFTask *getTask(const Vertex *vertex) const;

            /* === Setter(s) === */

            /**
             * @brief Registers the Task ix for a given firing of a given vertex.
             * @param vertex  Pointer to the vertex.
             * @param firing  Value of the firing.
             * @param taskIx  Value of the task ix to set.
             */
            void setTaskIx(const Vertex *vertex, u32 firing, u32 taskIx);

            /**
             * @brief Set the allocated address of the edge.
             * @param value    Value of the address.
             * @param edge     Pointer to the edge.
             * @param firing   Firing of the producer of the edge.
             */
            void setEdgeAddress(size_t value, const Edge *edge, u32 firing);

            /**
             * @brief Set the allocated offset of the edge.
             * @param value    Value of the offset.
             * @param edge     Pointer to the edge.
             * @param firing   Firing of the producer of the edge.
             */
            void setEdgeOffset(u32 value, const Edge *edge, u32 firing);

        private:
            spider::unique_ptr<FifoAlloc *> edgeAllocArray_;    /* == Array of allocated information for every edges == */
            spider::unique_ptr<u32 *> taskIxArray_;             /* == Array of schedule task ix == */
            spider::unique_ptr<sched::PiSDFTask *> tasksArray_; /* == Array of schedule tasks == */
        };
    }
}

#endif //SPIDER2_GRAPHALLOC_H
