/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2020)
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
/* === Include(s) === */

#include <runtime/algorithm/Runtime.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/schedule/exporter/SchedXMLGanttExporter.h>
#include <scheduling/schedule/exporter/SchedSVGGanttExporter.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Vertex.h>
#include <runtime/platform/RTPlatform.h>
#include <runtime/interface/RTCommunicator.h>
#include <runtime/interface/Notification.h>
#include <api/config-api.h>
#include <api/runtime-api.h>
#include <scheduling/allocator/FifoAllocator.h>
#include <scheduling/allocator/DefaultFifoAllocator.h>
#include <scheduling/allocator/SRLessDefaultFifoAllocator.h>
#include <scheduling/allocator/NoSyncDefaultFifoAllocator.h>

/* === Static variable === */

constexpr const char *VERTEX_TASK_COLOR = "#6C7A89";
constexpr const char *SCHEDULE_TASK_COLOR = "#F22613";
constexpr const char *TRANSFO_TASK_COLOR = "#F39C12";
constexpr const char *PARAM_TASK_COLOR = "#5333ED";
constexpr const char *MEMORY_TASK_COLOR = "#26A65B";

/* === Static function(s) definition === */

static u64 getTime(spider::time::time_point value, spider::time::time_point offset) {
    return static_cast<u64>(spider::time::duration::nanoseconds(offset, value));
}

/* === Function(s) definition === */

void spider::Runtime::exportPreExecGantt(const Schedule *schedule, const std::string &path) {
    if (api::useSVGOverXMLGantt()) {
        SchedSVGGanttExporter exporter{ schedule };
        exporter.printFromPath(path + ".svg");
    } else {
        SchedXMLGanttExporter exporter{ schedule };
        exporter.printFromPath(path + ".xml");
    }
}

void spider::Runtime::exportPostExecGantt(const pisdf::Graph *graph,
                                          const Schedule *schedule,
                                          time::time_point offset,
                                          const std::string &path) {
    if (!graph || !schedule) {
        return;
    }
    u64 applicationMinTime = UINT64_MAX;
    u64 applicationMaxTime = 0;
    u64 spiderTime = 0;
    u64 applicationRealTime = 0;
    auto ganttTasks = factory::vector<GanttTask>();
    /* == Get execution traces and update schedule info == */
    Notification notification;
    while (rt::platform()->communicator()->popTraceNotification(notification)) {
        TraceMessage msg;
        rt::platform()->communicator()->pop(msg, archi::platform()->getGRTIx(), notification.notificationIx_);
        GanttTask task;
        task.start_ = getTime(msg.startTime_, offset);
        task.end_ = getTime(msg.endTime_, offset);
        task.pe_ = notification.senderIx_;
        switch (notification.type_) {
            case NotificationType::TRACE_TASK: {
                const auto *vertex = graph->vertex(msg.taskIx_);
                if (vertex) {
                    task.name_ = vertex->name();
                    task.color_ = VERTEX_TASK_COLOR;
                    applicationMinTime = std::min(applicationMinTime, task.start_);
                    applicationMaxTime = std::max(applicationMaxTime, task.end_);
                    applicationRealTime += (task.end_ - task.start_);
                }
            }
                break;
            case NotificationType::TRACE_SCHEDULE:
                task.name_ = "schedule";
                task.color_ = SCHEDULE_TASK_COLOR;
                spiderTime += (task.end_ - task.start_);
                break;
            case NotificationType::TRACE_TRANSFO:
                task.name_ = "transfo";
                task.color_ = TRANSFO_TASK_COLOR;
                spiderTime += (task.end_ - task.start_);
                break;
            case NotificationType::TRACE_PARAM:
                task.name_ = "parameters";
                task.color_ = PARAM_TASK_COLOR;
                spiderTime += (task.end_ - task.start_);
                break;
            case NotificationType::TRACE_MEMORY:
                task.name_ = "memory";
                task.color_ = MEMORY_TASK_COLOR;
                spiderTime += (task.end_ - task.start_);
                break;
            default:
                throwSpiderException("received unexpected notification type");
        }
        ganttTasks.emplace_back(std::move(task));
    }

    /* == Print exec time == */
    const auto applicationUserTime = applicationMaxTime - applicationMinTime;
    log::info("Iteration execution information:\n");
    log::info("    >> Application exec time (user): %" PRId64"\n", applicationUserTime);
    log::info("    >> Application exec time (real): %" PRId64"\n", applicationRealTime);
    log::info("    >> Spider runtime exec time:     %" PRId64"\n", spiderTime);
    log::info("    >> Spider runtime overhead (user):  %f%%\n",
              100. - 100. * ((static_cast<double>(applicationUserTime) - static_cast<double>(spiderTime)) /
                             static_cast<double>(applicationUserTime)));
    log::info("    >> Spider runtime overhead (real):  %f%%\n",
              100. - 100. * ((static_cast<double>(applicationRealTime) - static_cast<double>(spiderTime)) /
                             static_cast<double>(applicationRealTime)));

    /* == Export the schedule == */
    if (api::useSVGOverXMLGantt()) {
        SchedSVGGanttExporter exporter{ schedule };
        exporter.printFromPath(path + ".svg");
    } else {
        SchedXMLGanttExporter exporter{ schedule };
        exporter.printFromTasks(ganttTasks, path + ".xml");
    }
}

spider::FifoAllocator *spider::Runtime::makeFifoAllocator(FifoAllocatorType type) {
    switch (type) {
        case spider::FifoAllocatorType::DEFAULT:
            return spider::make<DefaultFifoAllocator, StackID::RUNTIME>();
        case spider::FifoAllocatorType::DEFAULT_NOSYNC:
            return spider::make<NoSyncDefaultFifoAllocator, StackID::RUNTIME>();
        case spider::FifoAllocatorType::ARCHI_AWARE:
            break;
        default:
            throwSpiderException("unsupported type of FifoAllocator.");
    }
    return nullptr;
}

spider::FifoAllocator *spider::Runtime::makeSRLessFifoAllocator(FifoAllocatorType type) {
    switch (type) {
        case spider::FifoAllocatorType::DEFAULT:
            return spider::make<SRLessDefaultFifoAllocator, StackID::RUNTIME>();
        case spider::FifoAllocatorType::ARCHI_AWARE:
            break;
        default:
            throwSpiderException("unsupported type of FifoAllocator.");
    }
    return nullptr;
}
