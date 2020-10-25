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
#ifndef SPIDER2_PISDFTASK_H
#define SPIDER2_PISDFTASK_H

/* === Include(s) === */

#include <scheduling/task/Task.h>

namespace spider {

    namespace pisdf {
        class Vertex;

        class Edge;

        class GraphFiring;
    }

    namespace sched {

        /* === Class definition === */

        class PiSDFTask final : public Task {
        public:
            explicit PiSDFTask(pisdf::GraphFiring *handler, pisdf::Vertex *vertex, u32 firing);

            ~PiSDFTask() noexcept override = default;

            /* === Method(s) === */

            void receiveParams(const spider::array<i64> &values) final;

            void insertSyncTasks(SyncTask *sndTask, SyncTask *rcvTask, size_t ix, const Schedule *schedule) final;

            /* === Getter(s) === */

            i64 inputRate(size_t ix) const final;

            i64 outputRate(size_t ix) const final;

            Task *previousTask(size_t ix, const Schedule *schedule) const final;

            Task *nextTask(size_t ix, const Schedule *schedule) const final;

            u32 color() const final;

            std::string name() const final;

            bool isMappableOnPE(const PE *pe) const final;

            u64 timingOnPE(const PE *pe) const final;

            size_t dependencyCount() const final;

            inline pisdf::Vertex *vertex() const { return vertex_; }

            /* === Setter(s) === */

            void setIx(u32 ix) noexcept final;

        private:
            spider::unique_ptr<Task *> inputs_;
            spider::unique_ptr<Task *> outputs_;
            pisdf::GraphFiring *handler_ = nullptr;
            pisdf::Vertex *vertex_ = nullptr;
            u32 firing_ = UINT32_MAX;

            /* === Private method(s) === */

            u32 getOutputParamsCount() const final;

            u32 getKernelIx() const final;

            spider::unique_ptr<i64> buildInputParams() const final;

            bool updateNotificationFlags(bool *flags, const Schedule *schedule) const final;

            bool shouldBroadCast(const Schedule *schedule) const final;

            std::shared_ptr<JobFifos> buildJobFifos(const Schedule *schedule) const final;

            static Fifo buildInputFifo(const pisdf::Edge *edge, const Schedule *schedule);

            void buildDefaultOutFifos(spider::Fifo *outputFifos, const Schedule *schedule) const;

            void buildExternINOutFifos(spider::Fifo *outputFifos, const Schedule *schedule) const;

            void buildForkOutFifos(spider::Fifo *outputFifos, Fifo inputFifo, const Schedule *schedule) const;

            void buildDupOutFifos(spider::Fifo *outputFifos, Fifo inputFifo, const Schedule *schedule) const;
        };
    }
}
#endif //SPIDER2_PISDFTASK_H
