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
#include <spider-api/archi.h>
#include <spider-api/pisdf.h>
#include <archi/ProcessingElement.h>
#include <archi/Platform.h>
#include <graphs/pisdf/Graph.h>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::Scenario::Scenario(const PiSDFGraph *graph) {
    auto *&platform = spider::platform();

    /* == For all vertex inside the graph, we create an entry in the map == */
    auto PECount = platform->PECount();
    auto PETypeCount = platform->PETypeCount();
    for (const auto &vertex : graph->vertices()) {
        vertexMappingConstraintsMap_[vertex] = std::vector<bool>(PECount, true);
        vertexExecutionTimingsMap_[vertex] = spider::vector<Expression>(PETypeCount, Expression(100));
    }
}

bool spider::Scenario::isMappable(const PiSDFAbstractVertex *vertex, const spider::PE *PE) const {
    auto &contraints = vertexMappingConstraintsMap_.at(vertex);
    return contraints.at(PE->spiderPEIx());
}

std::int64_t spider::Scenario::executionTiming(const PiSDFAbstractVertex *vertex, const spider::PE *PE) const {
    auto &timings = vertexExecutionTimingsMap_.at(vertex);
    auto &timing = timings.at(PE->hardwareType());
    return timing.evaluate(vertex->containingGraph()->params());
}

void
spider::Scenario::setMappingConstraint(const PiSDFAbstractVertex *vertex, const spider::PE *PE, bool value) {
    auto &contraints = vertexMappingConstraintsMap_.at(vertex);
    contraints.at(PE->spiderPEIx()) = value;
}

void spider::Scenario::setExecutionTiming(const PiSDFAbstractVertex *vertex, const spider::PE *PE,
                                          std::int64_t value) {
    auto &timings = vertexExecutionTimingsMap_.at(vertex);
    auto &timing = timings.at(PE->hardwareType());
    timing = Expression(value);
}

void spider::Scenario::setExecutionTiming(const PiSDFAbstractVertex *vertex,
                                          std::uint32_t PEType,
                                          const std::string &expression) {
    auto &timings = vertexExecutionTimingsMap_.at(vertex);
    auto &timing = timings.at(PEType);
    timing = Expression(expression, vertex->containingGraph()->params());
}

void spider::Scenario::setExecutionTiming(const PiSDFAbstractVertex *vertex,
                                          const spider::PE *PE,
                                          const std::string &expression) {
    auto &timings = vertexExecutionTimingsMap_.at(vertex);
    auto &timing = timings.at(PE->hardwareType());
    timing = Expression(expression, vertex->containingGraph()->params());
}
