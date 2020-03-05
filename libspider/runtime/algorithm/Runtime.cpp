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

/* === Static function(s) definition === */

static void
updateScheduleTaskTimes(spider::TraceMessage &msg,
                        spider::pisdf::Graph *graph,
                        spider::Schedule *schedule,
                        spider::time::time_point offset) {
    auto *vertex = graph->vertex(msg.taskIx_);
    if (vertex && vertex->scheduleTaskIx() != SIZE_MAX) {
        auto *task = schedule->task(vertex->scheduleTaskIx());
        /* == Update start time == */
        auto startTime = static_cast<u64>(spider::time::duration::microseconds(offset, msg.startTime_));
        task->setStartTime(startTime);
        /* == Update end time == */
        auto endTime = static_cast<u64>(spider::time::duration::microseconds(offset, msg.endTime_));
        task->setEndTime(endTime);
        const auto peIx = task->mappedPe();
        schedule->stats().updateStartTime(peIx, startTime);
        schedule->stats().updateIDLETime(peIx, startTime - schedule->stats().endTime(peIx));
        schedule->stats().updateEndTime(peIx, endTime);
        schedule->stats().updateLoadTime(peIx, endTime - startTime);
    }
}

/* === Function(s) definition === */

void spider::Runtime::exportPreExecGantt(Schedule *schedule, const std::string &path) {
    if (api::useSVGOverXMLGantt()) {
        SchedSVGGanttExporter exporter{ schedule };
        exporter.printFromPath(path + ".svg");
    } else {
        SchedXMLGanttExporter exporter{ schedule };
        exporter.printFromPath(path + ".xml");
    }
}

void spider::Runtime::exportPostExecGantt(pisdf::Graph *graph,
                                          Schedule *schedule,
                                          time::time_point offset,
                                          const std::string &path) {
    if (!graph || !schedule) {
        return;
    }
    schedule->stats().reset();
    /* == Get execution traces and update schedule info == */
    Notification notification;
    while (rt::platform()->communicator()->popTraceNotification(notification)) {
        TraceMessage msg;
        rt::platform()->communicator()->pop(msg, archi::platform()->getGRTIx(), notification.notificationIx_);
        switch (notification.type_) {
            case NotificationType::TRACE_TASK:
                updateScheduleTaskTimes(msg, graph, schedule, offset);
                break;
            case NotificationType::TRACE_SCHEDULE:
                break;
            case NotificationType::TRACE_TRANSFO:
                break;
            case NotificationType::TRACE_PARAM:
                break;
            case NotificationType::TRACE_MEMORY:
                break;
            default:
                throwSpiderException("received unexpected notification type");
        }
    }

    /* == Export the schedule == */
    if (api::useSVGOverXMLGantt()) {
        SchedSVGGanttExporter exporter{ schedule };
        exporter.printFromPath(path + ".svg");
    } else {
        SchedXMLGanttExporter exporter{ schedule };
        exporter.printFromPath(path + ".xml");
    }
}
