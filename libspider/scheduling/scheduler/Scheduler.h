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
#ifndef SPIDER2_SCHEDULER_H
#define SPIDER2_SCHEDULER_H

/* === Include(s) === */

#include <scheduling/schedule/Schedule.h>
#include <graphs/pisdf/Graph.h>

namespace spider {

    /* === Class definition === */

    class Scheduler {
    public:

        explicit Scheduler(PiSDFGraph *graph) : graph_{ graph },
                                                params_{ graph->params() } { };

        Scheduler(PiSDFGraph *graph,
                  const spider::vector<PiSDFParam *> &params) : graph_{ graph },
                                                                params_{ params } { };

        virtual ~Scheduler() = default;

        /* === Method(s) === */

        /**
         * @brief Perform the mapping and scheduling of a given graph.
         * @return @refitem Schedule.
         */
        virtual sched::Schedule &mappingScheduling() = 0;

        /* === Getter(s) === */

        /**
         * @brief Return the @refitem Schedule owned by the Scheduler.
         * @return @refitem Schedule
         */
        inline const sched::Schedule &schedule() const;

        /* === Setter(s) === */

    protected:
        PiSDFGraph *graph_ = nullptr;
        const spider::vector<PiSDFParam *> &params_;
        sched::Schedule schedule_;

        /**
         * @brief Set the different information of a @refitem ScheduleJob.
         * @remark This method also update the schedule_ based on TransfoJob information.
         * @param job         TransfoJob to evaluate.
         * @param slave       Slave (cluster and pe) to execute on.
         * @param startTime   Start time of the job.
         * @param endTime     End time of the job.
         */
        void setJobInformation(sched::Job *job,
                               std::pair<uint32_t, uint32_t> slave,
                               uint64_t startTime,
                               uint64_t endTime);

        /**
         * @brief Compute the minimum start time possible for a given vertex.
         * @param vertex  Vertex to evaluate.
         * @return Minimum start time for the vertex.
         */
        uint64_t computeMinStartTime(const PiSDFAbstractVertex *vertex);

        /**
         * @brief Default vertex mapper that try to best fit.
         * @param vertex Vertex to map.
         */
        virtual void vertexMapper(const PiSDFAbstractVertex *vertex);
    };

    /* === Inline method(s) === */

    const sched::Schedule &Scheduler::schedule() const {
        return schedule_;
    }
}
#endif //SPIDER2_SCHEDULER_H
