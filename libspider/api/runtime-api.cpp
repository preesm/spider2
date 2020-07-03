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
/* === Includes === */

#include <api/runtime-api.h>
#include <api/archi-api.h>
#include <archi/Platform.h>
#include <archi/Cluster.h>
#include <archi/PE.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Graph.h>
#include <runtime/common/RTKernel.h>
#include <runtime/runner/JITMSRTRunner.h>
#include <runtime/interface/ThreadRTCommunicator.h>
#include <runtime/platform/ThreadRTPlatform.h>
#include <runtime/special-kernels/specialKernels.h>

/* === Static function(s) === */

static void createSpecialRTKernels() {
    auto *&rtPlatform = spider::rt::platform();
    if (!rtPlatform) {
        throwSpiderException("runtime platform should be created first.");
    }
    auto *forkKernel = spider::make<spider::RTKernel, StackID::RUNTIME>(spider::rt::fork);
    rtPlatform->addKernel(forkKernel);

    /* == Join Kernel == */
    auto *joinKernel = spider::make<spider::RTKernel, StackID::RUNTIME>(spider::rt::join);
    rtPlatform->addKernel(joinKernel);

    /* == Head Kernel == */
    auto *headKernel = spider::make<spider::RTKernel, StackID::RUNTIME>(spider::rt::head);
    rtPlatform->addKernel(headKernel);

    /* == Tail Kernel == */
    auto *tailKernel = spider::make<spider::RTKernel, StackID::RUNTIME>(spider::rt::tail);
    rtPlatform->addKernel(tailKernel);

    /* == Repeat Kernel == */
    auto *repeatKernel = spider::make<spider::RTKernel, StackID::RUNTIME>(spider::rt::repeat);
    rtPlatform->addKernel(repeatKernel);

    /* == Duplicate Kernel == */
    auto *duplicateKernel = spider::make<spider::RTKernel, StackID::RUNTIME>(spider::rt::duplicate);
    rtPlatform->addKernel(duplicateKernel);

    /* == Init Kernel == */
    auto *initKernel = spider::make<spider::RTKernel, StackID::RUNTIME>(spider::rt::init);
    rtPlatform->addKernel(initKernel);

    /* == End Kernel == */
    auto *endKernel = spider::make<spider::RTKernel, StackID::RUNTIME>(spider::rt::end);
    rtPlatform->addKernel(endKernel);

    /* == Extern input Kernel == */
    auto *extInKernel = spider::make<spider::RTKernel, StackID::RUNTIME>(spider::rt::externIn);
    rtPlatform->addKernel(extInKernel);

    /* == Extern output Kernel == */
    auto *extOutKernel = spider::make<spider::RTKernel, StackID::RUNTIME>(spider::rt::externOut);
    rtPlatform->addKernel(extOutKernel);
}

/* === Runtime platform related API === */

void spider::api::createThreadRTPlatform() {
    const auto *platform = archi::platform();
    if (!platform) {
        throwSpiderException("function should be called after definition of the physical platform.");
    }
    auto *&rtPlatform = rt::platform();
    if (rtPlatform) {
        throwSpiderException("there can be only one runtime platform.");
    }
    rtPlatform = make<ThreadRTPlatform, StackID::RUNTIME>(platform->LRTCount());

    /* == Add special actors refinements == */
    createSpecialRTKernels();

    /* == Create the communicator == */
    auto *communicator = make<ThreadRTCommunicator, StackID::RUNTIME>(platform->LRTCount());
    rtPlatform->setCommunicator(communicator);

    /* == Create the runtime runners == */
    size_t runnerIx = 0;
    for (auto &pe : platform->peArray()) {
        if (pe->isLRT()) {
            auto *runner = make<JITMSRTRunner, StackID::RUNTIME>(pe, runnerIx++, pe->affinity());
            rtPlatform->addRunner(runner);
        }
    }
}

/* === Runtime kernel related API === */

spider::RTKernel *spider::api::createRuntimeKernel(Kernel kernel) {
    if (rt::platform()) {
        auto *runtimeKernel = make<RTKernel, StackID::RUNTIME>(std::move(kernel));
        rt::platform()->addKernel(runtimeKernel);
        return runtimeKernel;
    }
    return nullptr;
}

spider::RTKernel *spider::api::createRuntimeKernel(const pisdf::Vertex *vertex, Kernel kernel) {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    if (vertex->executable()) {
        auto *runtimeInfo = vertex->runtimeInformation();
        if (runtimeInfo->kernelIx() != SIZE_MAX) {
            throwSpiderException("vertex %s already has a runtime kernel.", vertex->name().c_str());
        }
        if (rt::platform()) {
            auto *runtimeKernel = make<RTKernel, StackID::RUNTIME>(std::move(kernel));
            const auto index = rt::platform()->addKernel(runtimeKernel);
            runtimeInfo->setKernelIx(index);
            return runtimeKernel;
        }
    }
    return nullptr;
}

/* === Mapping and Timing related API === */

void spider::api::setVertexMappableOnCluster(const pisdf::Vertex *vertex, const Cluster *cluster, bool value) {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    if (vertex->executable()) {
        if (!cluster) {
            throwSpiderException("nullptr cluster.");
        }
        auto *runtimeInfo = vertex->runtimeInformation();
        for (auto &pe : cluster->peArray()) {
            runtimeInfo->setMappableConstraintOnPE(pe, value);
        }
    }
}

void spider::api::setVertexMappableOnCluster(const pisdf::Vertex *vertex, uint32_t clusterIx, bool value) {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    if (!archi::platform()) {
        throwSpiderException("platform must be created first.");
    }
    if (vertex->executable()) {
        auto *&platform = archi::platform();
        const auto *cluster = platform->cluster(clusterIx);
        setVertexMappableOnCluster(vertex, cluster, value);
    }
}

void spider::api::setVertexMappableOnPE(const pisdf::Vertex *vertex, const spider::PE *pe, bool value) {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    if (!pe) {
        throwSpiderException("nullptr pe.");
    }
    if (vertex->executable()) {
        auto *runtimeInfo = vertex->runtimeInformation();
        runtimeInfo->setMappableConstraintOnPE(pe, value);
    }
}

void spider::api::setVertexMappableOnPE(const pisdf::Vertex *vertex, size_t PEId, bool value) {
    if (!archi::platform()) {
        throwSpiderException("platform must be created first.");
    }
    setVertexMappableOnPE(vertex, archi::platform()->processingElement(PEId), value);
}

void spider::api::setVertexMappableOnAllPE(const pisdf::Vertex *vertex, bool value) {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    if (vertex->executable()) {
        auto *runtimeInfo = vertex->runtimeInformation();
        runtimeInfo->setMappableConstraintOnAllPE(value);
    }
}

void spider::api::setVertexExecutionTimingOnHWType(const pisdf::Vertex *vertex, uint32_t hardwareType, int64_t timing) {
    setVertexExecutionTimingOnHWType(vertex, hardwareType, std::to_string(timing));
}

void
spider::api::setVertexExecutionTimingOnHWType(const pisdf::Vertex *vertex, uint32_t hardwareType,
                                              std::string timingExpression) {
    if (!archi::platform()) {
        throwSpiderException("platform must be created first.");
    }
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    if (vertex->executable()) {
        auto *runtimeInfo = vertex->runtimeInformation();
        runtimeInfo->setTimingOnHWType(hardwareType,
                                       Expression(std::move(timingExpression), vertex->graph()->params()));
    }
}

void spider::api::setVertexExecutionTimingOnAllHWTypes(const pisdf::Vertex *vertex, std::string timingExpression) {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    if (vertex->executable()) {
        auto *runtimeInfo = vertex->runtimeInformation();
        runtimeInfo->setTimingOnAllHWTypes(Expression(std::move(timingExpression), vertex->graph()->params()));
    }
}

void spider::api::setVertexExecutionTimingOnAllHWTypes(const pisdf::Vertex *vertex, int64_t timing) {
    if (!vertex) {
        throwSpiderException("nullptr vertex.");
    }
    if (vertex->executable()) {
        auto *runtimeInfo = vertex->runtimeInformation();
        runtimeInfo->setTimingOnAllHWTypes(timing);
    }
}
