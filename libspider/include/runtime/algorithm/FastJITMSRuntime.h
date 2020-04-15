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
#ifndef SPIDER2_FASTJITMSRUNTIME_H
#define SPIDER2_FASTJITMSRUNTIME_H

/* === Include(s) === */

#include <runtime/algorithm/Runtime.h>
#include <graphs-tools/transformation/srdag/TransfoJob.h>
#include <scheduling/allocator/FifoAllocator.h>
#include <graphs-tools/transformation/srdagless/SRLessHandler.h>

namespace spider {

    /* === Forward declaration(s) === */

    class SRLessScheduler;

    /* === Class definition === */

    class FastJITMSRuntime final : public Runtime {
    public:
        explicit FastJITMSRuntime(pisdf::Graph *graph,
                                  SchedulingPolicy schedulingAlgorithm = SchedulingPolicy::SRLESS_LIST_BEST_FIT,
                                  FifoAllocatorType type = FifoAllocatorType::DEFAULT);

        ~FastJITMSRuntime() override = default;

        /* === Method(s) === */

        inline void setup() override { };

        bool execute() override;

        /* === Getter(s) === */

        /* === Setter(s) === */

    private:
        unique_ptr<SRLessScheduler> scheduler_;
        unique_ptr<FifoAllocator> fifoAllocator_;
        bool isFullyStatic_ = true;

        /* === Private method(s) === */

        /**
         * @brief Handle execution of static applications.
         * @return true
         */
        bool staticExecute();

        /**
         * @brief Handle execution of dynamic applications.
         * @return true
         */
        bool dynamicExecute();

        void handleStaticGraph(pisdf::Graph *graph);
    };
}

#endif //SPIDER2_FASTJITMSRUNTIME_H
