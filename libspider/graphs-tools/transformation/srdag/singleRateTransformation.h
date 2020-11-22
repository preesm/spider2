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
#ifndef SPIDER2_SINGLERATETRANSFORMATION_H
#define SPIDER2_SINGLERATETRANSFORMATION_H

#ifndef _NO_BUILD_LEGACY_RT

/* === Includes === */

#include <graphs-tools/transformation/srdag/TransfoJob.h>

namespace spider {
    namespace srdag {

        /* === Functions prototype === */

        /**
         * @brief Perform static single rate transformation for a given input job.
         * @remark If one of the subgraph of the job is dynamic then it is automatically split into two graphs.
         * @warning This function expect that dynamic graphs have been split using @refitem splitDynamicGraph before hand.
         * @param job    TransfoJob containing information on the transformation to perform.
         * @param srdag  Graph to append result of the transformation.
         * @return a pair of @refitem JobStack, the first one containing future static jobs, second one containing
         * jobs of dynamic graphs.
         * @throws @refitem Spider::Exception if srdag is nullptr
         */
        std::pair<JobStack, JobStack> singleRateTransformation(TransfoJob &job, srdag::Graph *srdag);

        namespace detail {

            struct TransfoVertex {
                srdag::Vertex *vertex_ = nullptr;
                i64 rate_ = -1;
                u32 portIx_ = UINT32_MAX;
                u32 lowerDep_ = UINT32_MAX;
                u32 upperDep_ = 0;

                TransfoVertex() = default;

                TransfoVertex(int64_t rate, uint32_t portIx, srdag::Vertex *vertex) : vertex_{ vertex },
                                                                                      rate_{ rate },
                                                                                      portIx_{ portIx } { }
            };

            using TransfoVertexVector = spider::vector<TransfoVertex>;

            /**
             * @brief Update values of dynamic dependent parameters of a job.
             * @param job Reference of the @refitem TransfoJob.
             */
            void updateParams(TransfoJob &job);

            /**
             * @brief Clone a given @refitem pisdf::Vertex and add the clone to the @refitem srdag::Graph.
             * @param vertex Pointer to the vertex.
             * @param firing Firing of the clone.
             * @param srdag  Pointer to the graph.
             * @param job    Reference of the @refitem TransfoJob.
             */
            void cloneVertex(const pisdf::Vertex *vertex, u32 firing, srdag::Graph *srdag, const TransfoJob &job);

            /**
             * @brief Creates future transformation jobs.
             * @param graph           Pointer to the reference graph.
             * @param srdag           Pointer to the srdag.
             * @param ref2CloneVector Vector of pisdf vertex to srdag clone index.
             * @param jobParams       Param vector of current job.
             * @return
             */
            std::pair<JobStack, JobStack> makeFutureJobs(const pisdf::Graph *graph,
                                                         srdag::Graph *srdag,
                                                         const spider::vector<size_t> &ref2CloneVector,
                                                         const spider::vector<std::shared_ptr<pisdf::Param>> &jobParams);

            /**
             * @brief Copy a parameter.
             * @param param     Reference to the original parameter.
             * @param jobParams Param vector of current job.
             * @return shared_ptr of the copied parameter.
             */
            std::shared_ptr<pisdf::Param> copyParameter(const std::shared_ptr<pisdf::Param> &param,
                                                        const spider::vector<std::shared_ptr<pisdf::Param>> &jobParams);


            /**
             * @brief Apply single rate linkage on a one edge.
             * @param edge            Pointer to the edge.
             * @param job             Reference of the @refitem TransfoJob.
             * @param srdag           Pointer to the srdag.
             * @param ref2CloneVector Vector of pisdf vertex to srdag clone index.
             */
            void singleRateLinkage(const pisdf::Edge *edge,
                                   TransfoJob &job,
                                   srdag::Graph *srdag,
                                   const spider::vector<size_t> &ref2CloneVector);

            /**
             * @brief Check if an edge is null (i.e has null production and consumption rates).
             * @param graph           Pointer to the reference graph.
             * @param edge            Pointer to the edge.
             * @param srdag           Pointer to the srdag.
             * @param ref2CloneVector Vector of pisdf vertex to srdag clone index.
             * @param jobParams       Param vector of current job.
             * @return true if edge is null, false else.
             */
            bool checkForNullEdge(const pisdf::Edge *edge,
                                  TransfoJob &job,
                                  srdag::Graph *srdag,
                                  const spider::vector<size_t> &ref2CloneVector);

            /**
             * @brief Compute all real dependencies of current instances of source / sink of given edge.
             * @param edge        Edge to evaluate.
             * @param srcVector   Vector of @refitem TransfoVertex corresponding to the sources of the edge of the job.
             * @param snkVector   Vector of @refitem TransfoVertex corresponding to the sinks of the edge of the job.
             */
            void computeDependencies(const pisdf::Edge *edge,
                                     spider::vector<TransfoVertex> &srcVector,
                                     spider::vector<TransfoVertex> &snkVector);

            template<class ConnectEdge>
            void connectForkOrJoin(srdag::Vertex *vertex,
                                   spider::vector<TransfoVertex> &workingVector,
                                   spider::vector<TransfoVertex> &oppositeVector,
                                   const ConnectEdge &edgeConnector);

            /**
             * @brief Add a @refitem ForkVertex into the single-rate graph and connect it.
             * @param srcVector   Vector of @refitem TransfoVertex.
             * @param snkVector   Vector of @refitem TransfoVertex.
             * @param srdag       Single-Rate graph.
             */
            void addForkVertex(spider::vector<TransfoVertex> &srcVector,
                               spider::vector<TransfoVertex> &snkVector,
                               srdag::Graph *srdag);

            /**
             * @brief Add a @refitem JoinVertex into the single-rate graph and connect it.
             * @param srcVector   Vector of @refitem TransfoVertex.
             * @param snkVector   Vector of @refitem TransfoVertex.
             * @param srdag       Single-Rate graph.
             */
            void addJoinVertex(spider::vector<TransfoVertex> &srcVector,
                               spider::vector<TransfoVertex> &snkVector,
                               srdag::Graph *srdag);

            /**
             * @brief Build a vector of @refitem TransfoVertex of the sink clones of a given edge.
             * @param edge            Edge to evaluate.
             * @param job             Reference of the @refitem TransfoJob.
             * @param srdag           Pointer to the srdag.
             * @param ref2CloneVector Vector of pisdf vertex to srdag clone index.
             * @return vector of TransfoVertex.
             */
            TransfoVertexVector buildSinkLinkerVector(const pisdf::Edge *edge,
                                                      const TransfoJob &job,
                                                      srdag::Graph *srdag,
                                                      const spider::vector<size_t> &ref2CloneVector);

            /**
             * @brief Build a vector of @refitem TransfoVertex of the source clones of a given edge.
             * @param edge            Edge to evaluate.
             * @param job             Reference of the @refitem TransfoJob.
             * @param srdag           Pointer to the srdag.
             * @param ref2CloneVector Vector of pisdf vertex to srdag clone index.
             * @return vector of TransfoVertex.
             */
            TransfoVertexVector buildSourceLinkerVector(const pisdf::Edge *edge,
                                                        const TransfoJob &job,
                                                        srdag::Graph *srdag,
                                                        const spider::vector<size_t> &ref2CloneVector);

            /**
             * @brief Checks if an interface is transparent (from the p.o.v of data rates)
             * @param job       Reference of the @refitem TransfoJob.
             * @param interface Pointer to the interface.
             * @return true if interface is transparent, false else.
             */
            bool isInterfaceTransparent(const TransfoJob &job, const pisdf::Interface *interface);

            /**
             * @brief Populate a given vector of @refitem TransfoVertex from given values.
             * @param vector          Vector to populate.
             * @param reference       Reference of the vertex to populate.
             * @param rate            Rate to set.
             * @param portIx          Port ix to use for the linkage.
             * @param job             Reference of the @refitem TransfoJob.
             * @param srdag           Pointer to the srdag.
             * @param ref2CloneVector Vector of pisdf vertex to srdag clone index.
             */
            void populateTransfoVertexVector(spider::vector<TransfoVertex> &vector,
                                             const pisdf::Vertex *reference,
                                             i64 rate,
                                             size_t portIx,
                                             const TransfoJob &job,
                                             srdag::Graph *srdag,
                                             const spider::vector<size_t> &ref2CloneVector);

            /**
             * @brief Populate vector from delay vertex and removes the edge.
             *        If edge is the last the four possible, removes the clone as well.
             * @param vector     Vector currently evaluated.
             * @param edge       Corresponding edge in the four possible (in_0: setter, in_1: producer, out_0:getter, out_1:consumer)
             * @param isSink     Boolean corresponding to type (true if calling for sink, false else)
             */
            void populateFromDelayVertex(spider::vector<TransfoVertex> &vector, srdag::Edge *edge, bool isSink);
        }
    }
}
#endif
#endif //SPIDER2_SINGLERATETRANSFORMATION_H
