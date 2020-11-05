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
#include <scheduling/launcher/TaskLauncher.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Graph.h>
#include <graphs-tools/transformation/pisdf/GraphFiring.h>
#include <graphs-tools/transformation/pisdf/GraphHandler.h>

#include <archi/Platform.h>
#include <api/archi-api.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::PiSDFTask::PiSDFTask(pisdf::GraphFiring *handler, const pisdf::Vertex *vertex) :
        Task(), handler_{ handler } {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    vertexIx_ = static_cast<u32>(vertex->ix());
}

void spider::sched::PiSDFTask::visit(TaskLauncher *launcher) {
    launcher->visit(this);
}

void spider::sched::PiSDFTask::setOnFiring(u32 firing) {
#ifndef NDEBUG
    if (firing >= handler_->getRV(vertex())) {
        throwSpiderException("invalid firing value for vertex: %s", vertex()->name().c_str());
    }
#endif
    currentFiring_ = firing;
}

spider::vector<spider::pisdf::DependencyIterator> spider::sched::PiSDFTask::computeExecDependencies() const {
    return pisdf::computeExecDependencies(handler_, vertex(), currentFiring_);
}

bool spider::sched::PiSDFTask::receiveParams(const spider::array<i64> &values) {
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
    return handler_->isResolved();
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
        const auto hdlFiring = handler->firingValue();
        name = graph->name() + std::string(":").append(std::to_string(hdlFiring)).append(":").append(name);
        handler = handler->getParent()->base();
        vertex = graph;
    }
    return name.append(this->vertex()->name()).append(":").append(std::to_string(currentFiring_));
}

bool spider::sched::PiSDFTask::isMappableOnPE(const spider::PE *pe) const {
    return vertex()->runtimeInformation()->isPEMappable(pe);
}

u64 spider::sched::PiSDFTask::timingOnPE(const spider::PE *pe) const {
    return static_cast<u64>(vertex()->runtimeInformation()->timingOnPE(pe, handler_->getParams()));
}

const spider::PE *spider::sched::PiSDFTask::mappedLRT() const {
    const auto *pe = this->mappedPe();
    if (!pe) {
        return nullptr;
    }
    return pe->attachedLRT();
}

u32 spider::sched::PiSDFTask::ix() const noexcept {
    return handler_->getTaskIx(vertex(), currentFiring_);
}

u32 spider::sched::PiSDFTask::firing() const {
    return currentFiring_;
}

const spider::pisdf::Vertex *spider::sched::PiSDFTask::vertex() const {
    return handler_->vertex(vertexIx_);
}

void spider::sched::PiSDFTask::setIx(u32 ix) noexcept {
    handler_->setTaskIx(vertex(), currentFiring_, ix);
}