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

#include <scheduling/task/TaskSRLess.h>
#include <graphs-tools/transformation/srless/GraphHandler.h>
#include <graphs-tools/transformation/srless/FiringHandler.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/Edge.h>

/* === Static function === */

/* === Method(s) implementation === */

spider::sched::TaskSRLess::TaskSRLess(const srless::FiringHandler *handler,
                                      const pisdf::Vertex *vertex,
                                      u32 firing) : Task(),
                                                    handler_{ handler }, vertex_{ vertex }, firing_{ firing } {

}

spider::sched::AllocationRule spider::sched::TaskSRLess::allocationRuleForInputFifo(size_t ix) const {
    return spider::sched::AllocationRule();
}

spider::sched::AllocationRule spider::sched::TaskSRLess::allocationRuleForOutputFifo(size_t ix) const {
    return spider::sched::AllocationRule();
}

spider::sched::Task *spider::sched::TaskSRLess::previousTask(size_t ix) const {
    return nullptr;
}

u32 spider::sched::TaskSRLess::color() const {
    return 0;
}

std::string spider::sched::TaskSRLess::name() const {
    return vertex_->name() + ":" + std::to_string(firing_);
}

void spider::sched::TaskSRLess::updateExecutionConstraints() {

}

spider::JobMessage spider::sched::TaskSRLess::createJobMessage() const {
    return spider::JobMessage();
}

void spider::sched::TaskSRLess::setExecutionDependency(size_t ix, spider::sched::Task *task) {

}

spider::array_handle<spider::sched::Task *> spider::sched::TaskSRLess::getDependencies() const {
    return { nullptr, 0u };
}

std::pair<ufast64, ufast64> spider::sched::TaskSRLess::computeCommunicationCost(const spider::PE *mappedPE) const {
    return { };
}

/* === Private method(s) implementation === */
