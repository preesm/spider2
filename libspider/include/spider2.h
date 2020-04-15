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
#ifndef SPIDER2_SPIDER2_H
#define SPIDER2_SPIDER2_H

/**
 * @brief Main extern include file for application to use
 */

/* === Include(s) === */

#include <api/archi-api.h>
#include <api/config-api.h>
#include <api/debug-api.h>
#include <api/global-api.h>
#include <api/pisdf-api.h>
#include <api/runtime-api.h>

#include <archi/Cluster.h>
#include <archi/PE.h>
#include <archi/Platform.h>
#include <archi/MemoryBus.h>
#include <archi/MemoryInterface.h>
#include <archi/InterMemoryBus.h>

#include <common/EnumIterator.h>
#include <common/Exception.h>
#include <common/Exporter.h>
#include <common/Logger.h>
#include <common/Math.h>
#include <common/Printer.h>
#include <common/Rational.h>
#include <common/Time.h>
#include <common/Types.h>

#include <common/cxx11-printf/Printf.h>
#include <common/cxx11-printf/Formatters.h>
#include <common/cxx11-printf/Ftoa.h>
#include <common/cxx11-printf/Itoa.h>

#include <containers/array.h>
#include <containers/array_handle.h>
#include <containers/deque.h>
#include <containers/forward_list.h>
#include <containers/list.h>
#include <containers/map.h>
#include <containers/queue.h>
#include <containers/set.h>
#include <containers/stack.h>
#include <containers/string.h>
#include <containers/unordered_map.h>
#include <containers/unordered_set.h>
#include <containers/vector.h>

#include <graphs/abstract/AbstractGraph.h>
#include <graphs/abstract/AbstractVertex.h>
#include <graphs/abstract/AbstractEdge.h>

#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/DynamicParam.h>
#include <graphs/pisdf/InHeritedParam.h>
#include <graphs/pisdf/Vertex.h>
#include <graphs/pisdf/DelayVertex.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/NonExecVertex.h>
#include <graphs/pisdf/Interface.h>
#include <graphs/pisdf/ExternInterface.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>

#include <graphs-tools/exporter/PiSDFDOTExporter.h>
#include <graphs-tools/exporter/PiSDFDOTExporterVisitor.h>
#include <graphs-tools/expression-parser/Expression.h>
#include <graphs-tools/expression-parser/RPNConverter.h>
#include <graphs-tools/helper/visitors/PiSDFVisitor.h>
#include <graphs-tools/helper/visitors/PiSDFDefaultVisitor.h>
#include <graphs-tools/helper/pisdf-helper.h>
#include <graphs-tools/transformation/optims/helper/patternOptimizer.h>
#include <graphs-tools/transformation/optims/helper/unitaryOptimizer.h>
#include <graphs-tools/transformation/optims/optimizations.h>
#include <graphs-tools/transformation/srdag/visitors/SRDAGCopyVertexVisitor.h>
#include <graphs-tools/transformation/srdag/SingleRateTransformer.h>
#include <graphs-tools/transformation/srdag/TransfoJob.h>
#include <graphs-tools/transformation/srdag/Transformation.h>
#include <graphs-tools/transformation/srdagless/SRLessHandler.h>

#include <memory/allocator/AbstractAllocatorPolicy.h>
#include <memory/allocator/dynamic-policies/FreeListAllocatorPolicy.h>
#include <memory/allocator/dynamic-policies/GenericAllocatorPolicy.h>
#include <memory/allocator/static-policies/LinearStaticAllocator.h>
#include <memory/allocator/allocator.h>
#include <memory/Stack.h>
#include <memory/memory.h>
#include <memory/unique_ptr.h>
#include <memory/shared_ptr.h>

#include <runtime/algorithm/Runtime.h>
#include <runtime/algorithm/JITMSRuntime.h>
#include <runtime/algorithm/FastJITMSRuntime.h>
#include <runtime/common/RTFifo.h>
#include <runtime/common/RTInfo.h>
#include <runtime/common/RTKernel.h>
#include <runtime/interface/Message.h>
#include <runtime/interface/Notification.h>
#include <runtime/interface/RTCommunicator.h>
#include <runtime/interface/ThreadRTCommunicator.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/platform/ThreadRTPlatform.h>
#include <runtime/runner/RTRunner.h>
#include <runtime/runner/JITMSRTRunner.h>
#include <runtime/special-kernels/specialKernels.h>

#include <scheduling/allocator/TaskMemory.h>
#include <scheduling/allocator/FifoAllocator.h>
#include <scheduling/allocator/DefaultFifoAllocator.h>
#include <scheduling/allocator/SRLessDefaultFifoAllocator.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/schedule/ScheduleStats.h>
#include <scheduling/schedule/ScheduleTask.h>
#include <scheduling/schedule/exporter/GanttTask.h>
#include <scheduling/schedule/exporter/SchedStatsExporter.h>
#include <scheduling/schedule/exporter/SchedSVGGanttExporter.h>
#include <scheduling/schedule/exporter/SchedXMLGanttExporter.h>
#include <scheduling/scheduler/Scheduler.h>
#include <scheduling/scheduler/GreedyScheduler.h>
#include <scheduling/scheduler/RoundRobinScheduler.h>
#include <scheduling/scheduler/ListScheduler.h>
#include <scheduling/scheduler/BestFitScheduler.h>
#include <scheduling/scheduler/srdagless/SRLessScheduler.h>
#include <scheduling/scheduler/srdagless/SRLessListScheduler.h>
#include <scheduling/scheduler/srdagless/SRLessBestFitScheduler.h>

#include <thread/Thread.h>
#include <thread/Semaphore.h>
#include <thread/Barrier.h>
#include <thread/Queue.h>
#include <thread/IndexedQueue.h>

#endif //SPIDER2_SPIDER2_H
