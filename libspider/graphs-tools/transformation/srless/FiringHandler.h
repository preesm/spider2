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
#ifndef SPIDER2_FIRINGHANDLER_H
#define SPIDER2_FIRINGHANDLER_H

/* === Include(s) === */

#include <common/Types.h>
#include <memory/unique_ptr.h>
#include <containers/vector.h>
#include <containers/array.h>

namespace spider {

    namespace pisdf {
        class Graph;

        class Param;
    }

    namespace srless {

        class GraphHandler;

        class FiringHandler;

        struct ExecDependencyInfo {
            const pisdf::Vertex *vertex_;
            const FiringHandler *handler_;
            size_t rate_;
            u32 edgeIx_;
            u32 memoryStart_;
            u32 memoryEnd_;
            u32 firingStart_;
            u32 firingEnd_;
        };

        struct ExecDependency {
            ExecDependencyInfo first_;
            ExecDependencyInfo second_;
        };

        /* === Class definition === */

        class FiringHandler {
        public:
            FiringHandler(const GraphHandler *parent,
                          const spider::vector<std::shared_ptr<pisdf::Param>> &params,
                          u32 firing);

            FiringHandler(FiringHandler &&) = default;

            FiringHandler(const FiringHandler &) = default;

            FiringHandler &operator=(FiringHandler &&) = default;

            FiringHandler &operator=(const FiringHandler &) = default;

            ~FiringHandler();

            /* === Method(s) === */

            ExecDependency computeExecDependency(const pisdf::Vertex *vertex, u32 firing, u32 edgeIx) const;

            spider::vector<ExecDependency>
            computeRelaxedExecDependency(const pisdf::Vertex *vertex, u32 firing, u32 edgeIx) const;

            ExecDependency
            computeConsDependenciesByEdge(const pisdf::Vertex *vertex, u32 firing, u32 edgeIx) const;

            bool isInputInterfaceTransparent(size_t ix) const;

            /**
             * @brief Registers the Task ix for a given firing of a given vertex.
             * @param vertex  Pointer to the vertex.
             * @param firing  Value of the firing.
             * @param taskIx  Value of the task ix to set.
             */
            void registerTaskIx(const pisdf::Vertex *vertex, u32 firing, u32 taskIx);

            /**
             * @brief Registers the Task ix for a given input interface.
             * @param interface Pointer to the interface.
             * @param taskIx    Task ix.
             */
            void registerTaskIx(const pisdf::Interface *interface, u32 taskIx);

            /**
             * @brief Compute BRV and save the values based on current value of parameters.
             * @remark this method automatically set the resolved_ flag to true.
             */
            void resolveBRV();

            /**
             * @brief Clears every values, and set resolved_ flag to false.
             */
            void clear();

            /* === Getter(s) === */

            inline const spider::array<GraphHandler *> &children() const { return children_; }

            inline spider::array<GraphHandler *> &children() { return children_; }

            inline const spider::vector<std::shared_ptr<pisdf::Param>> &getParams() const { return params_; }

            inline size_t ix() const { return ix_; }

            inline u32 firingValue() const { return firing_; }

            inline bool isResolved() const { return resolved_; }

            u32 getRV(const pisdf::Vertex *vertex) const;

            u32 getTaskIx(const pisdf::Vertex *vertex, u32 vertexFiring) const;

            u32 getTaskIx(const pisdf::Interface *interface) const;

            const FiringHandler *getChildFiring(const pisdf::Graph *subgraph, u32 firing) const;

            int64_t getParamValue(size_t ix);

            /* === Setter(s) === */

            inline void setIx(size_t ix) { ix_ = ix; }

            inline void setFiring(u32 firing) { firing_ = firing; }

            void setParamValue(size_t ix, int64_t value);

        private:
            spider::vector<std::shared_ptr<pisdf::Param>> params_;
            spider::array<GraphHandler *> children_; /* == match between subgraphs and their handler == */
            spider::array<u32> brv_;
            spider::array<u32 *> taskIxRegister_;
            const GraphHandler *parent_;
            size_t ix_{ };
            u32 firing_{ };
            bool resolved_;

            /* === private method(s) === */

            std::shared_ptr<pisdf::Param> copyParameter(const std::shared_ptr<pisdf::Param> &param,
                                                        const spider::vector<std::shared_ptr<pisdf::Param>> &parentParams);

            int64_t computeSrcRate(const pisdf::Edge *edge) const;

            ExecDependency computeExecDependency(const spider::pisdf::Edge *edge,
                                                 int64_t lowerCons,
                                                 int64_t upperCons) const;

            ExecDependency computeConsDependency(const spider::pisdf::Edge *edge,
                                                 int64_t lowerProd,
                                                 int64_t upperProd) const;

            spider::vector<ExecDependency> computeRelaxedExecDependency(const pisdf::Edge *edge,
                                                                        int64_t lowerCons,
                                                                        int64_t upperCons,
                                                                        int64_t prevSrcRate,
                                                                        u32 prevSrcRV,
                                                                        u32 prevLowDep,
                                                                        u32 prevUpDep) const;

            spider::vector<ExecDependency> computeRelFirstExecDependency(const pisdf::Edge *edge,
                                                                         int64_t consumption,
                                                                         int64_t prevSrcRate,
                                                                         u32 prevSrcRV,
                                                                         u32 prevDep) const;

            spider::vector<ExecDependency> computeRelLastExecDependency(const pisdf::Edge *edge,
                                                                        int64_t consumption,
                                                                        int64_t prevSrcRate,
                                                                        u32 prevSrcRV,
                                                                        u32 prevDep) const;

            spider::vector<ExecDependency> computeRelMidExecDependency(const pisdf::Edge *edge) const;
        };
    }
}

#endif //SPIDER2_FIRINGHANDLER_H
