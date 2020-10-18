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
#ifndef SPIDER2_JITMSRUNTIME_H
#define SPIDER2_JITMSRUNTIME_H

#ifndef _NO_BUILD_LEGACY_RT

/* === Include(s) === */

#include <runtime/algorithm/Runtime.h>
#include <graphs-tools/transformation/srdag/TransfoJob.h>

namespace spider {

    /* === Forward declaration(s) === */

    namespace sched {
        class ResourcesAllocator;
    }

    /* === Class definition === */

    /**
     * @brief JITMS runtime to handle dynamic application.
     * @remark see: https://tel.archives-ouvertes.fr/tel-01301642/file/These_HEULOT_Julien.pdf
     */
    class JITMSRuntime final : public Runtime {
    public:

        explicit JITMSRuntime(pisdf::Graph *graph, const RuntimeConfig &cfg);

        ~JITMSRuntime() override = default;

        /* === Method(s) === */

        inline void setup() override { };

        bool execute() override;

        /* === Getter(s) === */

        /* === Setter(s) === */

    private:
        spider::unique_ptr<srdag::Graph> srdag_;
        spider::unique_ptr<sched::ResourcesAllocator> resourcesAllocator_;
        time::time_point startIterStamp_ = time::min();

        /* === Private method(s) === */

        /**
         * @brief Update scheduler, execute scheduler, run schedule and wait.
         */
        void scheduleRunAndWait();

        /**
         * @brief Appends @refitem spider::srdag::TransfoJob from source vector to destination vector using MOVE semantic.
         * @param src       Source vector of the TransfoJobs to move.
         * @param dest      Destination vector to move the TransfoJobs to.
         */
        static void updateJobStack(vector<srdag::TransfoJob> &src, vector<srdag::TransfoJob> &dest) ;

        /**
         * @brief Transform all jobs of a given job stack and update results in static and dynamic job stacks.
         * @param iterJobStack      Stack to iterate on.
         * @param staticJobStack    Stack of static jobs to be updated.
         * @param dynamicJobStack   Stack of dynamic jobs to be updated.
         */
        void transformJobs(vector<srdag::TransfoJob> &iterJobStack,
                           vector<srdag::TransfoJob> &staticJobStack,
                           vector<srdag::TransfoJob> &dynamicJobStack);

        /**
         * @brief Transform all static jobs contained in staticJobStack and update the job stacks with future jobs.
         * @param staticJobStack   Static job stack.
         * @param dynamicJobStack  Dynamic job stack.
         */
        void transformStaticJobs(vector<srdag::TransfoJob> &staticJobStack, vector<srdag::TransfoJob> &dynamicJobStack);

        /**
         * @brief Transform all dynamic jobs contained in dynamicJobStack and update the job stacks with future jobs.
         * @param staticJobStack   Static job stack.
         * @param dynamicJobStack  Dynamic job stack.
         */
        void
        transformDynamicJobs(vector<srdag::TransfoJob> &staticJobStack, vector<srdag::TransfoJob> &dynamicJobStack);
    };
}
#endif
#endif //SPIDER2_JITMSRUNTIME_H
