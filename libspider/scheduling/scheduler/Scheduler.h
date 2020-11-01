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
#ifndef SPIDER2_SCHEDULER_H
#define SPIDER2_SCHEDULER_H

/* === Include(s) === */

#include <containers/vector.h>
#include <memory/unique_ptr.h>
#include <scheduling/task/Task.h>

namespace spider {

#ifndef _NO_BUILD_LEGACY_RT
    namespace srdag {
        class Graph;

        class Vertex;
    }
#endif

    namespace pisdf {
        class GraphHandler;

        class GraphFiring;

        struct VertexFiring {
            pisdf::GraphFiring *handler_;
            u32 vertexIx_;
            u32 firing_;
        };
    }

    namespace sched {

        class PiSDFTask;

        class SRDAGTask;

        /* === Class definition === */

        class Scheduler {
        public:
            Scheduler() = default;

            virtual ~Scheduler() noexcept = default;

            /* === Method(s) === */

#ifndef _NO_BUILD_LEGACY_RT
            /**
             * @brief Update internal state of the scheduler (mostly for dynamic applications)
             * @param graph  Graph to use to perform the update.
             */
            inline virtual spider::vector<SRDAGTask *> schedule(const srdag::Graph *) { return { }; }
#endif

            /**
             * @brief Update internal state of the scheduler (mostly for dynamic applications)
             * @param graphHandler Handler of the top graph.
             */
            inline virtual spider::vector<PiSDFTask *> schedule(pisdf::GraphHandler *) { return { }; }

            /**
             * @brief Clears scheduler resources.
             */
            inline virtual void clear() { }
        };
    }
}

#endif //SPIDER2_SCHEDULER_H
