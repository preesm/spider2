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
#ifndef SPIDER2_SCHEDEDGE_H
#define SPIDER2_SCHEDEDGE_H

/* === Include(s) === */

#include <common/Types.h>
#include <runtime/common/Fifo.h>

namespace spider {
    namespace sched {

        class Vertex;

        /* === Class definition === */

        class Edge {
        public:

            Edge(sched::Vertex *source, u32 srcIx, sched::Vertex *sink, u32 snkIx, Fifo alloc);

            ~Edge() = default;

            /* === Method(s) === */

            /**
             * @brief Build and return a name of the edge.
             * @return Name of the edge in format "#source -> #sink"
             */
            std::string name() const;

            /* === Getter(s) === */

            /**
             * @brief Get the source port ix of the edge
             * @return source port ix
             */
            inline u32 sourcePortIx() const { return srcPortIx_; }

            /**
             * @brief Get the sink port ix of the edge
             * @return sink port ix
             */
            inline u32 sinkPortIx() const { return snkPortIx_; }

            /**
             * @brief Get the source reference vertex.
             * @return reference to source
             */
            inline sched::Vertex *source() const { return source_; }

            /**
             * @brief Get the sink reference vertex.
             * @return reference to sink
             */
            inline sched::Vertex *sink() const { return sink_; }

            /**
             * @brief Get the allocated fifo of this edge.
             * @return @refitem Fifo.
             */
            inline Fifo getAlloc() const { return alloc_; }

            /**
             * @brief Get the allocated size on this edge.
             * @return size of the fifo of the edge.
             */
            inline size_t rate() const { return alloc_.size_; }

            /* === Setter(s) === */

            /**
             * @brief Set the source vertex of the edge.
             * @remark This method disconnect any previous connected edge on vertex and disconnect current source.
             * @param vertex  Vertex to connect to.
             * @param ix      Output port ix.
             * @return pointer to the old output @refitem Edge of vertex, nullptr else.
             */
            void setSource(sched::Vertex *vertex, u32 ix);

            /**
             * @brief Set the sink vertex of the edge.
             * @remark This method disconnect any previous connected edge on vertex and disconnect current sink.
             * @param vertex  Vertex to connect to.
             * @param ix      Input port ix.
             * @return pointer to the old input @refitem Edge of vertex, nullptr else.
             */
            void setSink(sched::Vertex *vertex, u32 ix);

            /**
             * @brief Set the allocated fifo associated with this edge.
             * @param alloc Fifo to set.
             */
            inline void setAlloc(Fifo alloc) { alloc_ = alloc; }

        private:
            Fifo alloc_{ };
            sched::Vertex *source_ = nullptr;
            sched::Vertex *sink_ = nullptr;
            u32 srcPortIx_ = UINT32_MAX;  /* = Index of the Edge in the source outputEdgeArray = */
            u32 snkPortIx_ = UINT32_MAX;  /* = Index of the Edge in the sink inputEdgeArray = */
        };


    }
}
#endif //SPIDER2_SCHEDEDGE_H
