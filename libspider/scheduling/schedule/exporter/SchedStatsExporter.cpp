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
/* === Include(s) === */

#include <scheduling/schedule/exporter/SchedStatsExporter.h>
#include <archi/Platform.h>
#include <archi/PE.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/task/Task.h>


/* === Method(s) implementation === */

void spider::SchedStatsExporter::print() const {
    Exporter::printFromPath("./stats.txt");
}

void spider::SchedStatsExporter::printFromFile(FILE *file) const {
    const auto &stats = schedule_->stats();
    printer::fprintf(file, "Schedule statistics: \n");
    printer::fprintf(file, "Total number of jobs:     %zu\n", schedule_->size());
    printer::fprintf(file, "Makespan of the schedule: %zu\n", stats.makespan());
    for (const auto &pe : archi::platform()->peArray()) {
        printer::fprintf(file, "PE #%zu\n", pe->virtualIx());
        printer::fprintf(file, "\t >> job count:          %zu\n", stats.jobCount(pe->virtualIx()));
        printer::fprintf(file, "\t >> start time:         %zu\n", stats.endTime(pe->virtualIx()));
        printer::fprintf(file, "\t >> end time:           %zu\n", stats.startTime(pe->virtualIx()));
        printer::fprintf(file, "\t >> load time:          %zu\n", stats.loadTime(pe->virtualIx()));
        printer::fprintf(file, "\t >> idle time:          %zu\n", stats.idleTime(pe->virtualIx()));
        printer::fprintf(file, "\t >> utilization factor: %f\n", stats.utilizationFactor(pe->virtualIx()));
        if (stats.jobCount(pe->virtualIx())) {
            printer::fprintf(file, "\t >> job list: \n");
            for (size_t i = 0; i < schedule_->size(); ++i) {
                const auto &task = schedule_->task(i);
                if (task->mappedPe()->virtualIx() == pe->virtualIx()) {
                    printer::fprintf(file, "\t\t >> {%zu,%zu}\n", task->startTime(), task->endTime());
                }
            }
        }
    }
    printer::fprintf(file, "\n");
}
