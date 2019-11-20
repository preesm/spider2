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
#ifndef SPIDER2_TRANSFORMATION_H
#define SPIDER2_TRANSFORMATION_H

/* === Includes === */

#include <containers/StlContainers.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/params/Param.h>
#include <graphs-tools/transformation/srdag/Helper.h>


namespace spider {
    namespace srdag {

        /* === Forward declaration(s) === */

        struct Job;

        struct TransfoVertex;

        /* === Type definition(s) === */

        using JobStack = spider::vector<Job>;

        using TransfoTracker = spider::vector<std::uint32_t>;

        using TransfoStack = spider::vector<TransfoVertex>;

        /* === Structure definition(s) === */

        struct Job {
            const PiSDFGraph *reference_ = nullptr;
            const std::uint32_t &srdagIx_;
            std::uint32_t instanceValue_ = 0;
            spider::vector<PiSDFParam *> params_;

            Job(Job &&) = default;

            Job(const Job &) = default;

            Job(const PiSDFGraph *graph, const std::uint32_t &srdagIx, std::uint32_t instance) : reference_{ graph },
                                                                                                 srdagIx_{ srdagIx },
                                                                                                 instanceValue_{
                                                                                                         instance } {
                params_.reserve(graph->paramCount());
            }

            ~Job() {
                if (reference_ && reference_->dynamic()) {
                    for (auto &param : params_) {
                        if (!param->containingGraph()) {
                            spider::destroy(param);
                            spider::deallocate(param);
                        }
                    }
                }
            }
        };

        struct TransfoVertex {
            std::int64_t rate_ = -1;
            std::uint32_t portIx_ = UINT32_MAX;
            PiSDFAbstractVertex *vertex_ = nullptr;
            std::uint32_t lowerDep_ = UINT32_MAX;
            std::uint32_t upperDep_ = 0;

            TransfoVertex() = default;

            TransfoVertex(std::int64_t rate, std::uint32_t portIx, PiSDFAbstractVertex *vertex) : rate_{ rate },
                                                                                                  portIx_{ portIx },
                                                                                                  vertex_{ vertex } { }
        };

        struct TransfoJob {
            const Job &job_;
            const PiSDFEdge *edge_ = nullptr;
            PiSDFGraph *srdag_ = nullptr;
            JobStack &nextJobs_;
            JobStack &dynaJobs_;
            TransfoTracker &tracker_;
            TransfoTracker &init2dynamic_;

            TransfoJob() = delete;

            TransfoJob(const Job &job,
                       const PiSDFEdge *edge,
                       PiSDFGraph *graph,
                       JobStack &nextJobs,
                       JobStack &dynaJobs,
                       TransfoTracker &tracker,
                       TransfoTracker &init2dynamic) : job_{ job },
                                                      edge_{ edge },
                                                      srdag_{ graph },
                                                      nextJobs_{ nextJobs },
                                                      dynaJobs_{ dynaJobs },
                                                      tracker_{ tracker },
                                                      init2dynamic_{ init2dynamic } { }
        };

        /* === Functions prototype === */

        /**
         * @brief Split dynamic graphs into two subgraphs: an init graph and a run graph.
         * @remark This method changes original graph.
         * @param subgraph  Subgraph to split (if static nothing happen).
         * @return true if subgraph was split, false else.
         */
        std::pair<PiSDFGraph *, PiSDFGraph *> splitDynamicGraph(PiSDFGraph *subgraph);

        /**
         * @brief Perform static single rate transformation for a given input job.
         * @remark If one of the subgraph of the job is dynamic then it is automatically split into two graphs.
         * @warning This function expect that dynamic graphs have been split using @refitem splitDynamicGraph before hand.
         * @param job    Job containing information on the transformation to perform.
         * @param srdag  Graph to append result of the transformation.
         * @return a pair of @refitem JobStack, the first one containing future static jobs, second one containing
         * jobs of dynamic graphs.
         * @throws @refitem Spider::Exception if srdag is nullptr
         */
        std::pair<JobStack, JobStack> staticSingleRateTransformation(const Job &job, PiSDFGraph *srdag);

        /**
         * @brief Perform single rate transformation for a given edge.
         * @param transfoJob Job information.
         */
        void staticEdgeSingleRateLinkage(TransfoJob &transfoJob);
    }
}


#endif //SPIDER2_TRANSFORMATION_H
