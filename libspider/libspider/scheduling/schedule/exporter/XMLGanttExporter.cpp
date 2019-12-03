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

#include <scheduling/schedule/exporter/XMLGanttExporter.h>
#include <api/archi.h>
#include <archi/Platform.h>
#include <archi/PE.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/Graph.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/schedule/ScheduleJob.h>
#include <iomanip>

/* === Static variable(s) === */

/* === Static function(s) === */

/* === Method(s) implementation === */

void spider::XMLGanttExporter::print() const {
    print("./gantt.xml");
}

void spider::XMLGanttExporter::print(const std::string &path) const {
    std::ofstream file{ path, std::ios::out };
    print(file);

    /* == We should not do this manually but this will ensure that data are correctly written even if it crashes == */
    file.close();
}

void spider::XMLGanttExporter::print(std::ofstream &file) const {
    file << "<data>" << '\n';
    for (const auto &job : schedule_->jobs()) {
        jobPrinter(file, job);
    }
    file << "</data>" << '\n';
}

void spider::XMLGanttExporter::jobPrinter(std::ofstream &file, const sched::Job &job) const {
    const auto *vertex = graph_->vertex(job.vertexIx());
    const auto *platform = spider::platform();
    auto PEIx = platform->findPE(job.mappingInfo().clusterIx, job.mappingInfo().PEIx).hardwareIx();

    /* == Let's compute a color based on the value of the pointer == */
    const auto *reference = vertex->reference();
    int32_t red = static_cast<uint8_t>((reinterpret_cast<uintptr_t>(reference) >> 3u) * 50 + 100);
    int32_t green = static_cast<uint8_t>((reinterpret_cast<uintptr_t>(reference) >> 2u) * 50 + 100);
    int32_t blue = static_cast<uint8_t>((reinterpret_cast<uintptr_t>(reference) >> 4u) * 50 + 100);
    file << '\t' << "<event" << '\n';
    file << '\t' << '\t' << R"(start=")" << job.mappingInfo().startTime << R"(")" << '\n';
    file << '\t' << '\t' << R"(end=")" << job.mappingInfo().endTime << R"(")" << '\n';
    file << '\t' << '\t' << R"(title=")" << vertex->name() << R"(")" << '\n';
    file << '\t' << '\t' << R"(mapping="PE)" << PEIx << R"(")" << '\n';
    std::ios savedFormat{ nullptr };
    savedFormat.copyfmt(file);
    file << '\t' << '\t' << R"(color="#)";
    file << std::setfill('0') << std::setbase(16);
    file << std::setw(2) << red << std::setw(2) << green << std::setw(2) << blue << '\"' << '\n';
    file.copyfmt(savedFormat);
    file << '\t' << '\t' << ">" << vertex->name() << ".</event>" << '\n';
}
