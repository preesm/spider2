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
#ifndef SPIDER2_GRAPHHANDLER_H
#define SPIDER2_GRAPHHANDLER_H

/* === Include(s) === */

#include <common/Types.h>
#include <memory/unique_ptr.h>
#include <containers/array_view.h>
#include <containers/vector.h>

namespace spider {

    namespace pisdf {

        class GraphFiring;

        class Graph;

        class Param;

        /* === Class definition === */

        class GraphHandler {
        public:
            GraphHandler(const pisdf::Graph *graph,
                         const spider::vector<std::shared_ptr<pisdf::Param>> &params,
                         u32 repetitionCount,
                         const pisdf::GraphFiring *handler = nullptr);

            GraphHandler(GraphHandler &&) = default;

            GraphHandler(const GraphHandler &) = delete;

            GraphHandler &operator=(GraphHandler &&) = default;

            GraphHandler &operator=(const GraphHandler &) = delete;

            ~GraphHandler();

            /* === Method(s) === */

            void clear();

            void resolveFirings();

            /* === Getter(s) === */

            inline array_view<GraphFiring *> firings() const { return make_view(firings_.get(), repetitionCount_); }

            inline GraphFiring *firing(size_t ix) const { return firings_[ix]; }

            inline array_view<GraphFiring *> firings() { return make_view(firings_.get(), repetitionCount_); }

            inline const GraphFiring *base() const { return handler_; }

            const pisdf::Graph *graph() const { return graph_; }

            inline u32 repetitionCount() const { return repetitionCount_; };

            inline bool isStatic() const { return static_; }

        private:
            spider::unique_ptr<GraphFiring *> firings_;
            const pisdf::GraphFiring *handler_;
            const pisdf::Graph *graph_;
            u32 repetitionCount_;
            bool static_;
        };
    }
}

#endif //SPIDER2_GRAPHHANDLER_H
