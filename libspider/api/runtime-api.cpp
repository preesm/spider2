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

#include <api/runtime-api.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/PE.h>
#include <archi/MemoryUnit.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/ExecVertex.h>

/* === Methods implementation === */

/* === General runtime related API === */

void spider::api::setVertexMappableOnCluster(pisdf::ExecVertex *vertex, const Cluster *cluster, bool value) {
    auto *runtimeInfo = vertex->runtimeInformation();
    if (!runtimeInfo) {
        runtimeInfo = vertex->createConstraints();
    }
    for (auto &pe : cluster->array()) {
        runtimeInfo->setMappableConstraintOnPE(pe, value);
    }
}

void spider::api::setVertexMappableOnCluster(pisdf::ExecVertex *vertex, uint32_t clusterIx, bool value) {
    auto *&platform = spider::platform();
    auto *cluster = platform->cluster(clusterIx);
    spider::api::setVertexMappableOnCluster(vertex, cluster, value);
}

void spider::api::setVertexMappableOnPE(pisdf::ExecVertex *vertex, const spider::PE *pe, bool value) {
    auto *runtimeInfo = vertex->runtimeInformation();
    if (!runtimeInfo) {
        runtimeInfo = vertex->createConstraints();
    }
    runtimeInfo->setMappableConstraintOnPE(pe, value);
}

void spider::api::setVertexMappableOnAllPE(pisdf::ExecVertex *vertex, bool value) {
    auto *runtimeInfo = vertex->runtimeInformation();
    if (!runtimeInfo) {
        runtimeInfo = vertex->createConstraints();
    }
    runtimeInfo->setMappableConstraintOnAllPE(value);
}

void spider::api::setVertexExecutionTimingOnPE(pisdf::ExecVertex *vertex, const PE *pe, std::string timingExpression) {
    auto *runtimeInfo = vertex->runtimeInformation();
    if (!runtimeInfo) {
        runtimeInfo = vertex->createConstraints();
    }
    runtimeInfo->setTimingOnPE(pe, Expression(std::move(timingExpression)));
}

void spider::api::setVertexExecutionTimingOnPE(pisdf::ExecVertex *vertex, const PE *pe, int64_t timing) {
    auto *runtimeInfo = vertex->runtimeInformation();
    if (!runtimeInfo) {
        runtimeInfo = vertex->createConstraints();
    }
    runtimeInfo->setTimingOnPE(pe, timing);
}

void spider::api::setVertexExecutionTimingOnAllPE(pisdf::ExecVertex *vertex, std::string timingExpression) {
    auto *runtimeInfo = vertex->runtimeInformation();
    if (!runtimeInfo) {
        runtimeInfo = vertex->createConstraints();
    }
    runtimeInfo->setTimingOnAllPE(Expression(std::move(timingExpression)));
}

void spider::api::setVertexExecutionTimingOnAllPE(pisdf::ExecVertex *vertex, int64_t timing) {
    auto *runtimeInfo = vertex->runtimeInformation();
    if (!runtimeInfo) {
        runtimeInfo = vertex->createConstraints();
    }
    runtimeInfo->setTimingOnAllPE(timing);
}