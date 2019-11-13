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

/* === Includes === */

#include <spider-api/scenario.h>
#include <spider-api/archi.h>
#include <spider-api/pisdf.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/ProcessingElement.h>
#include <archi/MemoryUnit.h>

/* === Methods implementation === */

/* === General Scenario related API === */

Spider::Scenario &Spider::scenario() {
    static Spider::Scenario scenario{ Spider::pisdfGraph() };
    return scenario;
}

void Spider::API::setVertexMappableOnCluster(const PiSDFAbstractVertex *vertex, const Cluster *cluster, bool value) {
    auto &scenario = Spider::scenario();
    for (auto &PE : cluster->processingElements()) {
        scenario.setMappingConstraint(vertex, PE, value);
    }
}

void Spider::API::setVertexMappableOnCluster(const PiSDFAbstractVertex *vertex, std::uint32_t clusterIx, bool value) {
    auto *&platform = Spider::platform();
    auto *cluster = platform->cluster(clusterIx);
    Spider::API::setVertexMappableOnCluster(vertex, cluster, value);
}

void Spider::API::setVertexMappableOnPE(const PiSDFAbstractVertex *vertex, const ProcessingElement *PE, bool value) {
    auto &scenario = Spider::scenario();
    scenario.setMappingConstraint(vertex, PE, value);
}

void Spider::API::setVertexMappableOnPE(const PiSDFAbstractVertex *vertex, std::uint32_t spiderPEIx, bool value) {
    auto &scenario = Spider::scenario();
    scenario.setMappingConstraint(vertex, spiderPEIx, value);
}

void Spider::API::setVertexMappableOnAllPE(const PiSDFAbstractVertex *vertex, bool value) {
    auto &scenario = Spider::scenario();
    auto *&platform = Spider::platform();
    for (const auto &cluster : platform->clusters()) {
        for (const auto &PE : cluster->processingElements()) {
            scenario.setMappingConstraint(vertex, PE, value);
        }
    }
}

void Spider::API::setVertexExecutionTimingOnPE(const PiSDFAbstractVertex *vertex,
                                               const ProcessingElement *PE,
                                               const std::string &expression) {
    auto &scenario = Spider::scenario();
    scenario.setExecutionTiming(vertex, PE, expression);
}

void Spider::API::setVertexExecutionTimingOnPE(const PiSDFAbstractVertex *vertex,
                                               const ProcessingElement *PE,
                                               std::int64_t timing) {
    auto &scenario = Spider::scenario();
    scenario.setExecutionTiming(vertex, PE, timing);
}

void Spider::API::setVertexExecutionTimingOnPEType(const PiSDFAbstractVertex *vertex,
                                                   std::uint32_t PEType,
                                                   const std::string &expression) {
    auto &scenario = Spider::scenario();
    scenario.setExecutionTiming(vertex, PEType, expression);
}

void Spider::API::setVertexExecutionTimingOnPEType(const PiSDFAbstractVertex *vertex,
                                                   std::uint32_t PEType,
                                                   std::int64_t timing) {
    auto &scenario = Spider::scenario();
    scenario.setExecutionTiming(vertex, PEType, timing);
}

void Spider::API::setVertexExecutionTimingOnAllPEType(const PiSDFAbstractVertex *vertex, std::int64_t timing) {
    auto &scenario = Spider::scenario();
    auto *&platform = Spider::platform();
    for (const auto &cluster : platform->clusters()) {
        for (const auto &PE : cluster->processingElements()) {
            scenario.setExecutionTiming(vertex, PE, timing);
        }
    }
}
