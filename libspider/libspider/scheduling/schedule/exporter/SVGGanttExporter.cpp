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

#include <scheduling/schedule/exporter/SVGGanttExporter.h>
#include <spider-api/archi.h>
#include <spider-api/pisdf.h>
#include <archi/Platform.h>
#include <archi/ProcessingElement.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/Graph.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/schedule/Job.h>
#include <iomanip>

/* === Static variable(s) === */

static constexpr std::uint32_t offset = 3;
static constexpr std::uint32_t border = 5;
static constexpr std::uint32_t arrowSize = 8;
static constexpr std::uint32_t arrowStroke = 2;
static constexpr std::uint32_t taskHeight = 50;
static constexpr std::uint32_t taskSpace = 5;

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::SVGGanttExporter::SVGGanttExporter(const sched::Schedule *schedule,
                                           const pisdf::Graph *graph) : Exporter(),
                                                                        schedule_{ schedule },
                                                                        graph_{ graph } {
    /* == Compute values needed for printing == */
    std::uint32_t minExecTime = UINT32_MAX;
    std::uint32_t maxExecTime = 0;
    for (auto &job : schedule_->jobs()) {
        const std::uint32_t &execTime = job.mappingInfo().endTime - job.mappingInfo().startTime;
        minExecTime = std::min(execTime, minExecTime);
        maxExecTime = std::max(execTime, maxExecTime);
    }
    const auto &ratio = static_cast<double>(maxExecTime) / static_cast<double>(minExecTime);
    if (widthMin_ * ratio > widthMax_) {
        widthMax_ = widthMin_ * ratio;
    }
    scaleFactor_ = static_cast<double>(widthMax_) / static_cast<double>(maxExecTime);

    /* == Compute dimensions of the Gantt == */
    makespanWidth_ =
            static_cast<double>(schedule_->stats().minStartTime() + schedule_->stats().makespan()) * scaleFactor_;
    width_ = makespanWidth_ + 2 * border + offset + arrowStroke + arrowSize;
    const auto *platform = spider::platform();
    const auto &PECount = platform->PECount();
    height_ = PECount * (taskHeight + taskSpace) + taskSpace + arrowStroke + arrowSize + offset;
}

void spider::SVGGanttExporter::print() const {
    print("./gantt.svg");
}

void spider::SVGGanttExporter::print(const std::string &path) const {
    std::ofstream file{ path, std::ios::out };
    print(file);

    /* == We should not do this manually but this will ensure that data are correctly written even if it crashes == */
    file.close();
}

void spider::SVGGanttExporter::print(std::ofstream &file) const {
    /* == Print header == */
    headerPrinter(file);

    /* == Print the arrows == */
    axisPrinter(file);

    /* == Print the jobs == */
    for (auto &job : schedule_->jobs()) {
        jobPrinter(file, job);
    }

    file << "  </g>" << std::endl;
    file << "</svg>" << std::endl;
}

void spider::SVGGanttExporter::headerPrinter(std::ofstream &file) const {
    file << R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!-- Created with Spider 2.0 (http://www.github.com/preesm/spider-2.0) -->

<svg
   xmlns:dc="http://purl.org/dc/elements/1.1/"
   xmlns:cc="http://creativecommons.org/ns#"
   xmlns:rdf="http://www.w3.org/1999/02/22-rdf-syntax-ns#"
   xmlns:svg="http://www.w3.org/2000/svg"
   xmlns="http://www.w3.org/2000/svg"
   xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.dtd"
   xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape"
   id="svg0"
   version="1.1"
   width=")" << width_ << R"("
   height=")" << height_ << R"(">
   <metadata
     id="metadata5">
    <rdf:RDF>
      <cc:Work
         rdf:about="">
        <dc:format>image/svg+xml</dc:format>
        <dc:type
           rdf:resource="http://purl.org/dc/dcmitype/StillImage" />
        <dc:title />
      </cc:Work>
    </rdf:RDF>
  </metadata>
  <g
     inkscape:label="Calque 1"
     inkscape:groupmode="layer">)" << '\n';
}

void spider::SVGGanttExporter::axisPrinter(std::ofstream &file) const {

    /* == Print vertical arrow == */
    const auto &arrowColor = "393c3c";
    const auto &verticalHeight = height_ - ((3 * arrowSize - 4) / 2);
    file << R"(
    <rect
       fill="#)" << arrowColor << R"("
       stroke="none"
       id="rect_arrow_vertical"
       width=")" << arrowStroke << R"("
       height=")" << verticalHeight << R"("
       x=")" << offset << R"("
       y=")" << (arrowSize - 1) << R"(" />
    <path
       fill="#)" << arrowColor << R"("
       display="inline"
       stroke="none"
       fill-rule="evenodd"
       d="M )" << (arrowSize / 2) << "," << 0 << " "
         << arrowSize << "," << arrowSize << " H "
         << 0 << R"( Z"
       id="arrow_vertical_head"
       inkscape:connector-curvature="0" />)";

    /* == Print vertical grid == */
    const auto &gridColor = "e8e8e8";
    const auto &gridCount = makespanWidth_ / 40;
    for (std::uint32_t i = 0; i <= gridCount; ++i) {
        file << R"(
    <rect
       fill="#)" << gridColor << R"("
       stroke="none"
       id="rect_grid"
       width="1"
       height=")" << verticalHeight << R"("
       x=")" << (offset + arrowStroke + border + i * 40) << R"("
       y=")" << (arrowSize - 1) << R"(" />)";
    }

    file << R"(
    <rect
       fill="#)" << arrowColor << R"("
       stroke="none"
       id="rect_arrow_horizontal"
       width=")" << (width_ - (offset + (arrowSize - 1))) << R"("
       height=")" << arrowStroke << R"("
       x=")" << offset << R"("
       y=")" << (height_ - (((arrowSize + arrowStroke) / 2))) << R"(" />
    <path
       fill="#)" << arrowColor << R"("
       display="inline"
       stroke="none"
       fill-rule="evenodd"
       d="M )" << width_ << "," << (height_ - (arrowSize / 2)) << " "
         << (width_ - arrowSize) << "," << height_ << " V "
         << (height_ - arrowSize) << R"( Z"
       id="arrow_horizontal_head"
       inkscape:connector-curvature="0" />)";
    /* == Print horizontal arrow == */
}

void spider::SVGGanttExporter::jobPrinter(std::ofstream &file, const sched::Job &job) const {
    /* == Compute color and width == */
    const auto *vertex = graph_->vertex(job.vertexIx());
    const auto *reference = vertex->reference();
    std::int32_t red = static_cast<std::uint8_t>((reinterpret_cast<std::uintptr_t>(reference) >> 3u) * 50 + 100);
    std::int32_t green = static_cast<std::uint8_t>((reinterpret_cast<std::uintptr_t>(reference) >> 2u) * 50 + 100);
    std::int32_t blue = static_cast<std::uint8_t>((reinterpret_cast<std::uintptr_t>(reference) >> 4u) * 50 + 100);
    const auto &taskWidth = static_cast<double>(job.mappingInfo().endTime - job.mappingInfo().startTime) * scaleFactor_;

    /* == Compute coordinates == */
    const auto *platform = spider::platform();
    const auto &PE = platform->findPE(job.mappingInfo().clusterIx, job.mappingInfo().PEIx);
    const auto &x = offset + arrowStroke + border + job.mappingInfo().startTime * scaleFactor_;
    const auto &y = height_ - (offset + arrowStroke + (PE.spiderPEIx() + 1) * (taskHeight + border));
    std::ios savedFormat{ nullptr };
    savedFormat.copyfmt(file);
    file << R"(
    <rect
       fill="#)";
    file << std::setfill('0') << std::setbase(16);
    file << std::setw(2) << red << std::setw(2) << green << std::setw(2) << blue;
    file.copyfmt(savedFormat);
    file << R"("
       stroke="none"
       id=)" << R"("rect_)" + vertex->name() << R"("
       width=")" << taskWidth << R"("
       height=")" << taskHeight << R"("
       x=")" << x << R"("
       y=")" << y << R"("
       ry="4" />)";
}


