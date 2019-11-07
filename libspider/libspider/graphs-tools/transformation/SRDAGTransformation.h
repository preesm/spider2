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
#ifndef SPIDER2_SRDAGTRANSFORMATION_H
#define SPIDER2_SRDAGTRANSFORMATION_H

/* === Includes === */

#include <containers/StlContainers.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Param.h>

namespace Spider {
    namespace SRDAG {

        /* === Forward declaration(s) === */

        struct Job;

        struct VertexLinker;

        /* === Type definition(s) === */

        using JobStack = Spider::vector<Job, StackID::TRANSFO>;

        using TransfoTracker = Spider::vector<std::uint32_t, StackID::TRANSFO>;

        using LinkerVector = Spider::vector<VertexLinker, StackID::TRANSFO>;

        /* === Structure definition(s) === */

        struct Job {
            const PiSDFGraph *reference_ = nullptr;
            const std::uint32_t &srdagIx_;
            std::uint32_t instanceValue_ = 0;
            Spider::vector<PiSDFParam *> params_;

            Job(Job &&) = default;

            Job(const Job &) = default;

            Job(const PiSDFGraph *graph, const std::uint32_t &srdagIx, std::uint32_t instance) : reference_{graph},
                                                                                                 srdagIx_{srdagIx},
                                                                                                 instanceValue_{
                                                                                                         instance} {
                params_.reserve(graph->paramCount());
            }

            ~Job() {
                for (auto &param : params_) {
                    if (!param->containingGraph()) {
                        Spider::destroy(param);
                        Spider::deallocate(param);
                    }
                }
            }
        };

        struct VertexLinker {
            std::int64_t rate_ = -1;
            std::uint32_t portIx_ = UINT32_MAX;
            PiSDFAbstractVertex *vertex_ = nullptr;
            std::uint32_t lowerDep_ = UINT32_MAX;
            std::uint32_t upperDep_ = 0;

            VertexLinker() = default;

            VertexLinker(std::int64_t rate, std::uint32_t portIx, PiSDFAbstractVertex *vertex) : rate_{rate},
                                                                                                 portIx_{portIx},
                                                                                                 vertex_{vertex} { }
        };

        struct JobLinker {
            const PiSDFEdge *edge_ = nullptr;
            PiSDFGraph *srdag_ = nullptr;
            const Job &job_;
            JobStack &nextJobs_;
            JobStack &dynaJobs_;
            TransfoTracker &tracker_;

            JobLinker() = delete;

            JobLinker(const PiSDFEdge *edge, PiSDFGraph *graph,
                      const Job &job, JobStack &nextJobs, JobStack &dynaJobs,
                      TransfoTracker &tracker) : edge_{edge}, srdag_{graph},
                                                 job_{job}, nextJobs_{nextJobs}, dynaJobs_{dynaJobs},
                                                 tracker_{tracker} { }
        };

        /* === Functions prototype === */

        /**
         * @brief Perform static single rate transformation for a given input job.
         * @remark If one of the subgraph of the job is dynamic then it is automatically split into two graphs.
         * @param job    Job containing information on the transformation to perform.
         * @param srdag  Graph to append result of the transformation.
         * @return a pair of @refitem JobStack, the first one containing future static jobs, second one containing
         * jobs of dynamic graphs.
         * @throws @refitem Spider::Exception if srdag is nullptr
         */
        std::pair<JobStack, JobStack> staticSingleRateTransformation(const Job &job, PiSDFGraph *srdag);

        void staticEdgeSingleRateLinkage(JobLinker &linker);
    }
}


#endif //SPIDER2_SRDAGTRANSFORMATION_H
