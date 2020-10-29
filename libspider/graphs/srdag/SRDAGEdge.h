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
#ifndef SPIDER2_SRDAGEDGE_H
#define SPIDER2_SRDAGEDGE_H

#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <common/Types.h>
#include <runtime/common/Fifo.h>

namespace spider {
    namespace srdag {

        class Vertex;

        /* === Class definition === */

        class Edge {
        public:

            Edge(srdag::Vertex *source, size_t srcIx, srdag::Vertex *sink, size_t snkIx, i64 rate);

            ~Edge() = default;

            /* === Method(s) === */

            /**
             * @brief Build and return a name of the edge.
             * @return Name of the edge in format "#source -> #sink"
             */
            std::string name() const;

            /* === Getter(s) === */

            /**
             * @brief Get the ix of the edge in the containing graph.
             * @return ix of the edge (SIZE_MAX if no ix).
             */
            inline size_t ix() const { return ix_; }

            /**
             * @brief Get the source port ix of the edge
             * @return source port ix
             */
            inline size_t sourcePortIx() const { return srcPortIx_; }

            /**
             * @brief Get the sink port ix of the edge
             * @return sink port ix
             */
            inline size_t sinkPortIx() const { return snkPortIx_; }

            /**
             * @brief Shortcurt for the method of @refitem Expression::value.
             * @return value of the source rate expression.
             */
            inline int64_t sourceRateValue() const { return rate_; }

            /**
             * @brief Shortcurt for the method of @refitem Expression::value.
             * @return value of the sink rate expression.
             */
            inline int64_t sinkRateValue() const { return rate_; }

            /**
             * @brief Get source rate of the edge.
             * @return value of the source rate expression.
             */
            inline int64_t rate() const { return rate_; }

            /**
             * @brief Get the source reference vertex.
             * @return reference to source
             */
            inline srdag::Vertex *source() const { return source_; }

            /**
             * @brief Get the sink reference vertex.
             * @return reference to sink
             */
            inline srdag::Vertex *sink() const { return sink_; }

            /**
             * @brief Get the allocated virtual address of this edge.
             * @return virtual memory address.
             */
            inline size_t address() const { return alloc_.address_; }

            inline u32 offset() const { return alloc_.offset_; }

            /* === Setter(s) === */

            inline void setRate(i64 rate) { rate_ = rate; }

            /**
             * @brief Set the ix of the edge in the containing graph.
             * @remark This method will override current value.
             * @param ix Ix to set.
             */
            inline void setIx(size_t ix) { ix_ = ix; }

            /**
             * @brief Set the source vertex of the edge.
             * @remark This method disconnect any previous connected edge on vertex and disconnect current source.
             * @param vertex  Vertex to connect to.
             * @param ix      Output port ix.
             * @param rate    Value of the rate.
             * @return pointer to the old output @refitem Edge of vertex, nullptr else.
             */
            void setSource(srdag::Vertex *vertex, size_t ix);

            /**
             * @brief Set the sink vertex of the edge.
             * @remark This method disconnect any previous connected edge on vertex and disconnect current sink.
             * @param vertex  Vertex to connect to.
             * @param ix      Input port ix.
             * @param rate    Value of the rate.
             * @return pointer to the old input @refitem Edge of vertex, nullptr else.
             */
            void setSink(srdag::Vertex *vertex, size_t ix);

            /**
             * @brief Set the virtual address of this edge.
             * @param address Address to set.
             */
            inline void setAddress(size_t address) { alloc_.address_ = address; }

            inline void setOffset(u32 offset) { alloc_.offset_ = offset; }

        private:
            srdag::Vertex *source_ = nullptr;
            srdag::Vertex *sink_ = nullptr;
            i64 rate_ = 0;
            size_t srcPortIx_ = SIZE_MAX;  /* = Index of the Edge in the source outputEdgeArray = */
            size_t snkPortIx_ = SIZE_MAX;  /* = Index of the Edge in the sink inputEdgeArray = */
            size_t ix_ = SIZE_MAX;         /* = Index of the Edge in the Graph (used for add and remove) = */
            FifoAlloc alloc_{ };
        };


    }
}
#endif
#endif //SPIDER2_SRDAGEDGE_H
