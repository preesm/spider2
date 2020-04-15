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
#ifndef SPIDER2_SRLESSHANDLER_H
#define SPIDER2_SRLESSHANDLER_H

/* === Include(s) === */

#include <common/Types.h>
#include <containers/vector.h>
#include <containers/unordered_map.h>
#include <memory/unique_ptr.h>
#include <memory/shared_ptr.h>

namespace spider {

    /* === Forward declaration(s) === */

    namespace pisdf {

        class Graph;

        class Param;

        class Vertex;

    }

    struct ExecDependency {
        pisdf::Vertex *vertex_;
        i64 rate_;
        u32 memoryStart_;
        u32 memoryEnd_;
        u32 firingStart_;
        u32 firingEnd_;
    };

    using VertexDependencies = spider::vector<ExecDependency>;

    namespace srdagless {

        /* === Struct(s) === */

        /* === Class definition === */

        class SRLessHandler {
        public:
            explicit SRLessHandler(pisdf::Graph *graph, const SRLessHandler *parentHandler = nullptr);

            ~SRLessHandler() = default;

            /* === Method(s) === */

            void resolveStatic();

            /* === Getter(s) === */

            const spider::vector<VertexDependencies> &getVertexDependencies(const pisdf::Vertex *vertex) const;

            const spider::vector<std::shared_ptr<pisdf::Param>> &getParameters() const;

            /* === Setter(s) === */

        private:
            pisdf::Graph *graph_ = nullptr;
            const SRLessHandler *parentHandler_ = nullptr;
            spider::vector<std::shared_ptr<pisdf::Param>> params_;
            spider::vector<spider::unique_ptr<SRLessHandler>> subHandlers_;
            spider::vector<spider::vector<VertexDependencies>> vertexDependencies_;

            /* === Private method(s) === */

            void computeDependencies(const pisdf::Vertex *vertex);

            void computeDependency(const pisdf::Edge *edge,
                                   u32 firing,
                                   spider::vector<ExecDependency> &firingDependency);

            void computeDelayedDependency(const pisdf::Edge *edge,
                                          u32 firing,
                                          spider::vector<ExecDependency> &firingDependency);

            void computeGetterDependency(const pisdf::Edge *edge,
                                         u32 firing,
                                         spider::vector<ExecDependency> &firingDependency);
        };
    }
}
#endif //SPIDER2_SRLESSHANDLER_H
