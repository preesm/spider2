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
#ifndef SPIDER2_RUNTIME_H
#define SPIDER2_RUNTIME_H

/* === Includes === */

#include <common/Exception.h>
#include <common/Time.h>
#include <api/global-api.h>
#include <scheduling/allocator/FifoAllocator.h>

/* === Define(s) === */

#define TRACE_TRANSFO_START()\
    if (api::exportTraceEnabled()) {\
        transfoMsg.startTime_ = time::now();\
    }

#define TRACE_TRANSFO_END() \
    if (api::exportTraceEnabled()) {\
        transfoMsg.endTime_ = time::now();\
        auto *communicator = rt::platform()->communicator();\
        auto msgIx = communicator->push(transfoMsg, archi::platform()->getGRTIx());\
        communicator->pushTraceNotification(Notification{ NotificationType::TRACE_TRANSFO, archi::platform()->getGRTIx(), msgIx });\
    }

#define TRACE_SCHEDULE_START()\
    if (api::exportTraceEnabled()) {\
        schedMsg.startTime_ = time::now();\
    }

#define TRACE_SCHEDULE_END() \
    if (api::exportTraceEnabled()) {\
        schedMsg.endTime_ = time::now();\
        auto *communicator = rt::platform()->communicator();\
        auto msgIx = communicator->push(schedMsg, archi::platform()->getGRTIx());\
        communicator->pushTraceNotification(Notification{ NotificationType::TRACE_SCHEDULE, archi::platform()->getGRTIx(), msgIx });\
    }

namespace spider {

    /* === Forward declaration(s) === */

    class Monitor;

    class Schedule;

    /* === Class definition === */

    class Runtime {
    public:

        explicit Runtime(pisdf::Graph *graph) : graph_{ graph } {
            if (!graph_) {
                throwSpiderException("nullptr graph.");
            }
        };

        virtual ~Runtime() = default;

        /**
         * @brief Setup method of the runtime (maybe empty)
         */
        virtual void setup() = 0;

        /**
         * @brief Main method of the runtime, do a graph iteration.
         * @return true if iteration was successful, false else.
         */
        virtual bool execute() = 0;

    protected:
        pisdf::Graph *graph_ = nullptr;
        Monitor *monitor_ = nullptr;


        /**
         * @brief Export the expected Gantt obtained by the scheduling algorithm.
         * @param schedule Pointer to the schedule.
         * @param path     Path of the file.
         */
        void exportPreExecGantt(Schedule *schedule, const std::string &path = "./sched-gantt");

        /**
         * @brief Export the Gantt of the real execution trace of the application for 1 graph iteration.
         * @remark Requires to have enable the execution traces with @refitem spider::enableExportTrace.
         * @param graph    Pointer to the graph to be used as referece.
         * @param schedule Pointer to the schedule.
         * @param offset   Time offset to apply.
         * @param path     Path of the file.
         */
        void exportPostExecGantt(pisdf::Graph *graph,
                                 Schedule *schedule,
                                 time::time_point offset = time::min(),
                                 const std::string &path = "./exec-gantt");

        static FifoAllocator *makeFifoAllocator(FifoAllocatorType type);

        static FifoAllocator *makeSRLessFifoAllocator(FifoAllocatorType type);
    };
}
#endif //SPIDER2_RUNTIME_H
