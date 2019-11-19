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
#ifndef SPIDER2_LISTSCHEDULER_H
#define SPIDER2_LISTSCHEDULER_H

/* === Include(s) === */

#include <scheduling/scheduler/Scheduler.h>
#include <containers/StlContainers.h>

namespace spider {

    /* === Class definition === */

    class ListScheduler : public Scheduler {
    public:

        explicit ListScheduler(PiSDFGraph *graph) : ListScheduler(graph, graph->params()) { };

        ListScheduler(PiSDFGraph *graph, const spider::vector<PiSDFParam*> &params);

        ~ListScheduler() override = default;

        /* === Method(s) === */

        Schedule &mappingScheduling() override = 0;

        /* === Getter(s) === */

        /* === Setter(s) === */

    protected:
        struct ListVertex {
            PiSDFAbstractVertex *vertex = nullptr;
            std::int32_t level = -1;

            explicit ListVertex(PiSDFAbstractVertex *vertex, std::int32_t level = -1) : vertex{ vertex },
                                                                                        level{ level } { };
        };

        spider::vector<ListVertex> sortedVertexVector_;

        /* === Protected method(s) === */

        std::uint64_t computeMinStartTime(const PiSDFAbstractVertex *vertex);

    private:
        static std::int32_t computeScheduleLevel(ListVertex &listVertex,
                                                 spider::vector<ListVertex> &sortedVertexVector);
    };

    /* === Inline method(s) === */

}

#endif //SPIDER2_LISTSCHEDULER_H
