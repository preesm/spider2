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
#ifndef SPIDER2_SINGLERATETRANSFORMER_H
#define SPIDER2_SINGLERATETRANSFORMER_H

/* === Include(s) === */

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

            SingleRateTransformer(TransfoJob &job, pisdf::Graph *srdag);

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

            using TransfoVertexVector = vector<TransfoVertex>;

            /* === Private member(s) === */

            vector<size_t> ref2Clone_;
            TransfoJob &job_;
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
            static size_t getIx(const pisdf::Vertex *vertex, const pisdf::Graph *graph);

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
             * @brief Copy parameter according to its type.
             * @param param  Const reference to the parameter.
             * @return Parameter copy shared_ptr.
             */
            std::shared_ptr<pisdf::Param> copyParameter(const std::shared_ptr<pisdf::Param> &param) const;

            /**
             * @brief Check if the given edge is null and if so, add non-exec vertices to snk and src ports.
             *        An edge is considered null if and only if the source rate is null AND the sink rate is null.
             * @param edge  Pointer to the edge.
             * @return true if null edge, false else.
             */
            bool checkForNullEdge(pisdf::Edge *edge);

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

            template<class ConnectEdge>
            void connectForkOrJoin(pisdf::Vertex *vertex,
                                   vector<TransfoVertex> &workingVector,
                                   vector<TransfoVertex> &oppositeVector,
                                   const ConnectEdge &edgeConnector) const;

            /**
             * @brief Add a @refitem ForkVertex into the single-rate graph and connect it.
             * @param srcVector   Vector of @refitem TransfoVertex.
             * @param snkVector   Vector of @refitem TransfoVertex.
             * @param srdag       Single-Rate graph.
             */
            void addForkVertex(vector<TransfoVertex> &srcVector, vector<TransfoVertex> &snkVector);

            /**
             * @brief Add a @refitem JoinVertex into the single-rate graph and connect it.
             * @param srcVector   Vector of @refitem TransfoVertex.
             * @param snkVector   Vector of @refitem TransfoVertex.
             */
            void addJoinVertex(vector<TransfoVertex> &srcVector, vector<TransfoVertex> &snkVector);

            /**
             * @brief Populate a given vector of @refitem TransfoVertex from given values.
             * @param vector     Vector to populate.
             * @param reference  Reference of the vertex to populate.
             * @param rate       Rate to set.
             * @param portIx     Port ix to use for the linkage.
             */
            void populateTransfoVertexVector(spider::vector<TransfoVertex> &vector,
                                             const pisdf::Vertex *reference,
                                             int64_t rate,
                                             size_t portIx) const;

            /**
             * @brief Build a vector of @refitem TransfoVertex of the sink clones of a given edge.
             * @param edge Edge to evaluate.
             * @return vector of TransfoVertex.
             */
            TransfoVertexVector buildSinkLinkerVector(pisdf::Edge *edge);

            /**
             * @brief Build a vector of @refitem TransfoVertex of the source clones of a given edge.
             * @param edge Edge to evaluate.
             * @return vector of TransfoVertex.
             */
            TransfoVertexVector buildSourceLinkerVector(pisdf::Edge *edge);

            /**
             * @brief Populate vector from delay vertex and removes the edge.
             *        If edge is the last the four possible, removes the clone as well.
             * @param vector     Vector currently evaluated.
             * @param edge       Corresponding edge in the four possible (in_0: setter, in_1: producer, out_0:getter, out_1:consumer)
             * @param isSink     Boolean corresponding to type (true if calling for sink, false else)
             */
            void populateFromDelayVertex(spider::vector<TransfoVertex> &vector, pisdf::Edge *edge, bool isSink);
        };
    }
}

#endif //SPIDER2_SINGLERATETRANSFORMER_H
