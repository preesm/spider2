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
#ifndef SPIDER2_STATICRUNTIME_H
#define SPIDER2_STATICRUNTIME_H

/* === Include(s) === */

#include <runtime/algorithm/Runtime.h>
#include <graphs-tools/transformation/srdag/TransfoJob.h>
#include <scheduling/allocator/FifoAllocator.h>
#include <containers/unordered_map.h>

namespace spider {

    /* === Forward declaration(s) === */

    class SchedulerLegacy;


    /* === Class definition === */

    /**
     * @brief Runtime to handle static application.
     */
    class StaticRuntime final : public Runtime {
    public:
        explicit StaticRuntime(pisdf::Graph *graph,
                               SchedulingPolicy schedulingAlgorithm = SchedulingPolicy::LIST,
                               FifoAllocatorType type = FifoAllocatorType::DEFAULT);

        ~StaticRuntime() override = default;

        /* === Method(s) === */

        inline void setup() override { };

        bool execute() override;

        /* === Getter(s) === */

        /* === Setter(s) === */

    private:
        time::time_point startIterStamp_ = time::min();
        unique_ptr<pisdf::Graph> srdag_;
        size_t iter_ = 0U;

        /* === Private method(s) === */

        /**
         * @brief Apply the single-rate transformation, perform the scheduling and run the application.
         * @remark This method should be called only on the first iteration of the application.
         */
        void applyTransformationAndRun();

        /**
         * @brief Run the application.
         * @remark This method should be called at any iteration as long as a schedule as been derived.
         */
        void run();
    };
}

#endif //SPIDER2_STATICRUNTIME_H
