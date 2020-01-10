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
#ifndef SPIDER2_SINGLERATETRANSFORMER_H
#define SPIDER2_SINGLERATETRANSFORMER_H

/* === Include(s) === */

#include <containers/containers.h>
#include <graphs-tools/transformation/srdag/TransfoJob.h>

namespace spider {

    /* === Forward Declaration(s) === */

    namespace pisdf {

        class Graph;

        class Edge;
    }

    namespace srdag {

        /* === Type definition(s) === */

        using JobStack = spider::vector<TransfoJob>;

        /* === Class definition === */

        class SingleRateTransformer {
        public:
            SingleRateTransformer() = delete;

            SingleRateTransformer(const TransfoJob &job, pisdf::Graph *srdag);

            ~SingleRateTransformer() = default;

            /* === Method(s) === */

            std::pair<JobStack, JobStack> execute();

        private:

            struct TransfoVertex {
                pisdf::Vertex *vertex_ = nullptr;
                int64_t rate_ = -1;
                uint32_t portIx_ = UINT32_MAX;
                uint32_t lowerDep_ = UINT32_MAX;
                uint32_t upperDep_ = 0;

                TransfoVertex() = default;

                TransfoVertex(int64_t rate, uint32_t portIx, pisdf::Vertex *vertex) : vertex_{ vertex },
                                                                                      rate_{ rate },
                                                                                      portIx_{ portIx } { }
            };

            /* === Private member(s) === */

            stack_vector(ref2Clone_, size_t, StackID::TRANSFO);
            const TransfoJob &job_;
            pisdf::Graph *srdag_ = nullptr;

            /* === Private method(s) === */

            /**
             * @brief Uniformize the index of a vertex inside a given graph.
             *        ExecVertex and derivative -> uniformIx = vertex->ix().
             *        Graph                     -> uniformIx = vertex->ix().
             *        InputInterface            -> uniformIx = vertex->ix() + vertexCount.
             *        OutputInterface           -> uniformIx = vertex->ix() + vertexCount + inputInterfaceCount.
             * @param vertex  Vertex to evaluate.
             * @param graph   Graph of the vertex.
             * @return uniform index of the vertex
             */
            static inline size_t uniformIx(const pisdf::Vertex *vertex, const pisdf::Graph *graph) {
                if (vertex->subtype() == pisdf::VertexType::INPUT) {
                    return vertex->ix() + graph->vertexCount();
                } else if (vertex->subtype() == pisdf::VertexType::OUTPUT) {
                    return vertex->ix() + graph->vertexCount() + graph->inputEdgeCount();
                }
                return vertex->ix();
            }

            /**
             * @brief Insert @refitem RepeatVertex and @refitem TailVertex for every input and output interfaces of
             *        a given subgraph instance, respectively.
             */
            void replaceInterfaces();

            /**
             * @brief Create the future @refitem TransfoJob from the subgraphs of current job_.reference_.
             * @return pair of static and dynamic @refitem JobStack.
             */
            std::pair<JobStack, JobStack> makeFutureJobs();

            /**
             * @brief Perform single rate linkage for a given Edge.
             * @param edge  Edge to evaluate.
             */
            void singleRateLinkage(pisdf::Edge *edge);

            /**
             * @brief Compute all real dependencies of current instances of source / sink of given edge.
             * @param edge        Edge to evaluate.
             * @param srcVector   Vector of @refitem TransfoVertex corresponding to the sources of the edge of the job.
             * @param snkVector   Vector of @refitem TransfoVertex corresponding to the sinks of the edge of the job.
             */
            void computeDependencies(pisdf::Edge *edge,
                                     spider::vector<TransfoVertex> &srcVector,
                                     spider::vector<TransfoVertex> &snkVector);

            /**
             * @brief Add a @refitem ForkVertex into the single-rate graph and connect it.
             * @param srcVector   Vector of @refitem TransfoVertex.
             * @param snkVector   Vector of @refitem TransfoVertex.
             * @param srdag       Single-Rate graph.
             */
            void addForkVertex(spider::vector<TransfoVertex> &srcVector, spider::vector<TransfoVertex> &snkVector);

            /**
             * @brief Add a @refitem JoinVertex into the single-rate graph and connect it.
             * @param srcVector   Vector of @refitem TransfoVertex.
             * @param snkVector   Vector of @refitem TransfoVertex.
             */
            void addJoinVertex(spider::vector<TransfoVertex> &srcVector, spider::vector<TransfoVertex> &snkVector);

            /**
             * @brief Build a vector of @refitem TransfoVertex of the sink clones of a given edge.
             * @param edge Edge to evaluate.
             * @return vector of TransfoVertex.
             */
            spider::vector<TransfoVertex> buildSinkLinkerVector(pisdf::Edge *edge);

            /**
             * @brief Build a vector of @refitem TransfoVertex of the source clones of a given edge.
             * @param edge Edge to evaluate.
             * @return vector of TransfoVertex.
             */
            spider::vector<TransfoVertex> buildSourceLinkerVector(pisdf::Edge *edge);

            /**
             * @brief Populate a given vector of @refitem TransfoVertex from given values.
             * @param vector     Vector to populate.
             * @param reference  Reference of the vertex to populate.
             * @param rate       Rate to set.
             * @param portIx     Port ix to use for the linkage.
             */
            inline void populateTransfoVertexVector(spider::vector<TransfoVertex> &vector,
                                                    const pisdf::Vertex *reference,
                                                    int64_t rate,
                                                    size_t portIx) const {
                const auto *clone = srdag_->vertex(ref2Clone_[uniformIx(reference, job_.reference_)]);
                const auto &cloneIx = clone->ix();
                for (auto ix = (cloneIx + reference->repetitionValue()); ix != cloneIx; --ix) {
                    vector.emplace_back(rate, portIx, srdag_->vertex(ix - 1));
                }
            }

            /**
             * @brief Populate vector from delay vertex and removes the edge.
             *        If edge is the last the four possible, removes the clone as well.
             * @param vector     Vector currently evaluated.
             * @param edge       Corresponding edge in the four possible (in_0: setter, in_1: producer, out_0:getter, out_1:consumer)
             * @param isSink     Boolean corresponding to type (true if calling for sink, false else)
             */
            inline void populateFromDelayVertex(spider::vector<TransfoVertex> &vector,
                                                pisdf::Edge *edge,
                                                bool isSink) {
                pisdf::Vertex *vertex = nullptr;
                int64_t rate;
                size_t portIx;
                if (isSink) {
                    vertex = edge->sink();
                    rate = edge->sourceRateExpression().evaluate(job_.params_);
                    portIx = edge->sinkPortIx();
                } else {
                    vertex = edge->source();
                    rate = edge->sinkRateExpression().evaluate(job_.params_);
                    portIx = edge->sourcePortIx();
                }
                vector.emplace_back(rate, portIx, vertex);

                /* == Remove the Edge == */
                srdag_->removeEdge(edge);
            }
        };
    }
}

#endif //SPIDER2_SINGLERATETRANSFORMER_H
