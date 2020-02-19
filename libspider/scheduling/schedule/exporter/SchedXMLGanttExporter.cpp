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

#include <scheduling/schedule/exporter/SchedXMLGanttExporter.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/schedule/ScheduleTask.h>
#include <api/archi-api.h>
#include <archi/PE.h>
#include <archi/Platform.h>
#include <iomanip>
#include <fstream>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

void spider::SchedXMLGanttExporter::print() const {
    Exporter::printFromPath("./gantt.xml");
}

void spider::SchedXMLGanttExporter::printFromFile(std::ofstream &file) const {
    file << "<data>" << '\n';
    for (auto &task : schedule_->tasks()) {
        if (task->state() != TaskState::PENDING) {
            printTask(file, task.get());
        }
    }
    file << "</data>" << '\n';
}

void spider::SchedXMLGanttExporter::printTask(std::ofstream &file, const ScheduleTask *task) const {
    const auto *platform = archi::platform();

    /* == Let's compute a color based on the value of the pointer == */
    const auto name = task->name();
    u32 color = task->color();
    i32 red = static_cast<u8>((color >> 16u) & 0xFFu);
    i32 green = static_cast<u8>((color >> 8u) & 0xFFu);
    i32 blue = static_cast<u8>(color & 0xFFu);
    file << '\t' << "<event" << '\n';
    file << '\t' << '\t' << R"(start=")" << task->startTime() << R"(")" << '\n';
    file << '\t' << '\t' << R"(end=")" << task->endTime() << R"(")" << '\n';
    file << '\t' << '\t' << R"(title=")" << name << R"(")" << '\n';
    file << '\t' << '\t' << R"(mapping=")" << platform->peFromVirtualIx(task->mappedPe())->name() << R"(")" << '\n';
    std::ios savedFormat{ nullptr };
    savedFormat.copyfmt(file);
    file << '\t' << '\t' << R"(color="#)";
    file << std::setfill('0') << std::setbase(16);
    file << std::setw(2) << red << std::setw(2) << green << std::setw(2) << blue << '\"' << '\n';
    file.copyfmt(savedFormat);
    file << '\t' << '\t' << ">" << name << ".</event>" << '\n';
}
