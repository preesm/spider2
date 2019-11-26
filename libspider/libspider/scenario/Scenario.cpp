/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2019) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2017-2019)
 * Hugo Miomandre <hugo.miomandre@insa-rennes.fr> (2017)
 * Julien Heulot <julien.heulot@insa-rennes.fr> (2013 - 2015)
 * Yaset Oliva <yaset.oliva@insa-rennes.fr> (2013 - 2014)
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

#include <scenario/Scenario.h>
#include <archi/PE.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Vertex.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

const std::vector<bool> &spider::Scenario::mappingConstraints(const spider::pisdf::Vertex *vertex) const {
    return mappingConstraintsVector_.at(vertex->ix());
}

const spider::vector<Expression> &spider::Scenario::executionTimings(const spider::pisdf::Vertex *vertex) const {
    return executionTimingsVector_.at(vertex->ix());
}

int64_t spider::Scenario::executionTiming(const spider::pisdf::Vertex *vertex, uint32_t PEType) const {
    auto &timings = executionTimings(vertex);
    return timings.at(PEType).evaluate(vertex->containingGraph()->params());
}

void spider::Scenario::setMappingConstraints(const spider::pisdf::Vertex *vertex,
                                             const std::initializer_list<bool> &constraints) {
    mappingConstraintsVector_.at(vertex->ix()) = constraints;
}

void spider::Scenario::setMappingConstraints(const spider::pisdf::Vertex *vertex, uint32_t PECount, bool value) {
    mappingConstraintsVector_.at(vertex->ix()) = std::vector<bool>(PECount, value);
}

void spider::Scenario::setMappingConstraint(const spider::pisdf::Vertex *vertex, uint32_t spiderPEIx, bool value) {
    auto &contraints = mappingConstraintsVector_.at(vertex->ix());
    contraints.at(spiderPEIx) = value;
}

bool spider::Scenario::isMappable(const spider::pisdf::Vertex *vertex, const spider::PE *PE) const {
    auto &contraints = mappingConstraintsVector_.at(vertex->ix());
    return contraints.at(PE->spiderPEIx());
}

int64_t spider::Scenario::executionTiming(const spider::pisdf::Vertex *vertex, const spider::PE *PE) const {
    auto &timings = executionTimingsVector_.at(vertex->ix());
    auto &timing = timings.at(PE->hardwareType());
    return timing.evaluate(vertex->containingGraph()->params());
}

void
spider::Scenario::setMappingConstraint(const spider::pisdf::Vertex *vertex, const spider::PE *PE, bool value) {
    auto &contraints = mappingConstraintsVector_.at(vertex->ix());
    contraints.at(PE->spiderPEIx()) = value;
}

void spider::Scenario::setExecutionTimings(const spider::pisdf::Vertex *vertex,
                                           const std::initializer_list<int64_t> &timings) {
    executionTimingsVector_.at(vertex->ix()).clear();
    for (const auto &timing : timings) {
        executionTimingsVector_.at(vertex->ix()).emplace_back(timing);
    }
}

void spider::Scenario::setExecutionTimings(const spider::pisdf::Vertex *vertex,
                                           uint32_t PETypeCount,
                                           int64_t value) {
    executionTimingsVector_.at(vertex->ix()) = spider::vector<Expression>(PETypeCount, Expression(value));
}

void
spider::Scenario::setExecutionTiming(const spider::pisdf::Vertex *vertex, uint32_t PEType, int64_t value) {
    auto &timings = executionTimingsVector_.at(vertex->ix());
    auto &timing = timings.at(PEType);
    timing = Expression(value);
}

void spider::Scenario::setExecutionTiming(const spider::pisdf::Vertex *vertex, const spider::PE *PE,
                                          int64_t value) {
    auto &timings = executionTimingsVector_.at(vertex->ix());
    auto &timing = timings.at(PE->hardwareType());
    timing = Expression(value);
}

void spider::Scenario::setExecutionTiming(const spider::pisdf::Vertex *vertex,
                                          uint32_t PEType,
                                          const std::string &expression) {
    auto &timings = executionTimingsVector_.at(vertex->ix());
    auto &timing = timings.at(PEType);
    timing = Expression(expression, vertex->containingGraph()->params());
}

void spider::Scenario::setExecutionTiming(const spider::pisdf::Vertex *vertex,
                                          const spider::PE *PE,
                                          const std::string &expression) {
    auto &timings = executionTimingsVector_.at(vertex->ix());
    auto &timing = timings.at(PE->hardwareType());
    timing = Expression(expression, vertex->containingGraph()->params());
}
