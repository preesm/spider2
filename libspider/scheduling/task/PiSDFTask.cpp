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

/* === Include(s) === */

#include <scheduling/task/PiSDFTask.h>
#include <scheduling/task/SyncTask.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/launcher/TaskLauncher.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Param.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>
#include <graphs-tools/numerical/dependencies.h>
#include <runtime/common/Fifo.h>
#include <api/runtime-api.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::PiSDFTask::PiSDFTask(pisdf::GraphFiring *handler,
                                    const pisdf::Vertex *vertex,
                                    u32 firing) : Task(),
                                                  handler_{ handler },
                                                  firing_{ firing } {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    vertexIx_ = static_cast<u32>(vertex->ix());
}

void spider::sched::PiSDFTask::visit(TaskLauncher *launcher) {
    launcher->visit(this);
}

void spider::sched::PiSDFTask::receiveParams(const spider::array<i64> &values) {
    const auto *vertex = this->vertex();
    if (vertex->subtype() != pisdf::VertexType::CONFIG) {
        throwSpiderException("Only config vertices can update parameter values.");
    }
    auto paramIterator = values.begin();
    for (const auto ix : vertex->outputParamIxVector()) {
        const auto value = *(paramIterator++);
        handler_->setParamValue(ix, value);
        if (log::enabled<log::TRANSFO>()) {
            log::info<log::TRANSFO>("Parameter [%12s]: received value #%" PRId64".\n",
                                    handler_->getParams()[ix]->name().c_str(), value);
        }
    }
}

void spider::sched::PiSDFTask::insertSyncTasks(SyncTask *sndTask, SyncTask *rcvTask, size_t ix, const Schedule *) {
//    const auto fifo = fifos_->inputFifo(ix);
//    sndTask->setAlloc(fifo);
//    rcvTask->setAlloc(fifo);
}

spider::vector<spider::pisdf::DependencyIterator> spider::sched::PiSDFTask::computeExecDependencies() const {
    auto result = factory::vector<pisdf::DependencyIterator>(StackID::SCHEDULE);
    const auto *vertex = this->vertex();
    if (vertex->inputEdgeCount()) {
        spider::reserve(result, vertex->inputEdgeCount());
        for (const auto *edge : vertex->inputEdges()) {
            result.emplace_back(pisdf::computeExecDependency(vertex, firing_, edge->sinkPortIx(), handler_));
        }
    }
    return result;
}

spider::vector<spider::pisdf::DependencyIterator> spider::sched::PiSDFTask::computeConsDependencies() const {
    auto result = factory::vector<pisdf::DependencyIterator>(StackID::SCHEDULE);
    const auto *vertex = this->vertex();
    if (vertex->outputEdgeCount()) {
        spider::reserve(result, vertex->outputEdgeCount());
        for (const auto *edge : vertex->outputEdges()) {
            result.emplace_back(pisdf::computeConsDependency(vertex, firing_, edge->sourcePortIx(), handler_));
        }
    }
    return result;
}

u32 spider::sched::PiSDFTask::color() const {
    const auto *vertex = this->vertex();
    const u32 red = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex) >> 3u) * 50 + 100);
    const u32 green = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex) >> 2u) * 50 + 100);
    const u32 blue = static_cast<u8>((reinterpret_cast<uintptr_t>(vertex) >> 4u) * 50 + 100);
    return 0u | (red << 16u) | (green << 8u) | (blue);
}

std::string spider::sched::PiSDFTask::name() const {
    std::string name{ };
    const auto *vertex = this->vertex();
    const auto *handler = handler_;
    while (handler) {
        const auto *graph = vertex->graph();
        const auto firing = handler->firingValue();
        name = graph->name() + std::string(":").append(std::to_string(firing)).append(":").append(name);
        handler = handler->getParent()->handler();
        vertex = graph;
    }
    return name.append(this->vertex()->name()).append(":").append(std::to_string(firing_));
}

u32 spider::sched::PiSDFTask::ix() const noexcept {
    return handler_->getTaskIx(vertex(), firing_);
}

void spider::sched::PiSDFTask::setIx(u32 ix) noexcept {
    handler_->registerTaskIx(vertex(), firing_, ix);
}

bool spider::sched::PiSDFTask::isMappableOnPE(const spider::PE *pe) const {
    return vertex()->runtimeInformation()->isPEMappable(pe);
}

u64 spider::sched::PiSDFTask::timingOnPE(const spider::PE *pe) const {
    return static_cast<u64>(vertex()->runtimeInformation()->timingOnPE(pe, handler_->getParams()));
}

const spider::pisdf::Vertex *spider::sched::PiSDFTask::vertex() const {
    return handler_->vertex(vertexIx_);
}

/* === Private method(s) === */

u32 spider::sched::PiSDFTask::getOutputParamsCount() const {
    return static_cast<u32>(vertex()->outputParamCount());
}

u32 spider::sched::PiSDFTask::getKernelIx() const {
    return static_cast<u32>(vertex()->runtimeInformation()->kernelIx());
}

spider::unique_ptr<i64> spider::sched::PiSDFTask::buildInputParams() const {
    return pisdf::buildVertexRuntimeInputParameters(vertex(), handler_->getParams());
}