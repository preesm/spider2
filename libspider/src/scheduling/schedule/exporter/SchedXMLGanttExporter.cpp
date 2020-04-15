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

#include <scheduling/schedule/exporter/SchedXMLGanttExporter.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/schedule/ScheduleTask.h>
#include <api/archi-api.h>
#include <archi/PE.h>
#include <archi/Platform.h>

/* === Method(s) implementation === */

void spider::SchedXMLGanttExporter::print() const {
    Exporter::printFromPath("./gantt.xml");
}

void spider::SchedXMLGanttExporter::printFromFile(FILE *file) const {
    printer::fprintf(file, "<data>\n");
    for (auto &task : schedule_->tasks()) {
        if (task->state() != TaskState::PENDING && task->state() != TaskState::NOT_SCHEDULABLE) {
            printTask(file, task.get());
        }
    }
    printer::fprintf(file, "</data>\n");
}

void spider::SchedXMLGanttExporter::printTask(FILE *file, const ScheduleTask *task) const {
    const auto *platform = archi::platform();

    /* == Let's compute a color based on the value of the pointer == */
    const auto name = task->name();
    u32 color = task->color();
    i32 red = static_cast<u8>((color >> 16u) & 0xFFu);
    i32 green = static_cast<u8>((color >> 8u) & 0xFFu);
    i32 blue = static_cast<u8>(color & 0xFFu);
    printer::fprintf(file, "\t<event\n");
    printer::fprintf(file, "\t\t" R"(start=")" "%" PRIu64"" R"(")" "\n", task->startTime());
    printer::fprintf(file, "\t\t" R"(end=")" "%" PRIu64"" R"(")" "\n", task->endTime());
    printer::fprintf(file, "\t\t" R"(title="%s")" "\n", name.c_str());
    printer::fprintf(file, "\t\t" R"(mapping="%s")" "\n", platform->peFromVirtualIx(task->mappedPe())->name().c_str());
    printer::fprintf(file, "\t\t" R"(color="#%.2X%.2X%.2X")" "\n", red, green, blue);
    printer::fprintf(file, "\t\t>%s.</event>\n", name.c_str());
}

void spider::SchedXMLGanttExporter::printFromTasks(const vector<GanttTask> &taskVector, const std::string &path) {
    auto *file = std::fopen(path.c_str(), "w+");
    if (!file) {
        throwSpiderException("Failed to open file with path [%s]", path.c_str());
    }
    printer::fprintf(file, "<data>\n");
    for (auto &task : taskVector) {
        printer::fprintf(file, "\t<event\n");
        printer::fprintf(file, "\t\t" R"(start=")" "%" PRIu64"" R"(")" "\n", task.start_);
        printer::fprintf(file, "\t\t" R"(end=")" "%" PRIu64"" R"(")" "\n", task.end_);
        printer::fprintf(file, "\t\t" R"(duration=")" "%" PRIu64"" R"(")" "\n", task.end_ - task.start_);
        printer::fprintf(file, "\t\t" R"(title="%s")" "\n", task.name_.c_str());
        printer::fprintf(file, "\t\t" R"(mapping="%s")" "\n", archi::platform()->peFromVirtualIx(task.pe_)->name().c_str());
        printer::fprintf(file, "\t\t" R"(color="%s")" "\n", task.color_);
        printer::fprintf(file, "\t\t>%s.</event>\n", task.name_.c_str());
    }
    printer::fprintf(file, "</data>\n");
    std::fclose(file);
}
