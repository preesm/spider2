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

#include <scheduling/schedule/exporter/SchedSVGGanttExporter.h>
#include <api/archi-api.h>
#include <graphs/pisdf/Graph.h>
#include <scheduling/schedule/Schedule.h>
#include <scheduling/schedule/ScheduleTask.h>
#include <iomanip>

/* === Static variable(s) === */

static constexpr u32 OFFSET_X = 3;
static constexpr u32 OFFSET_Y = 3;
static constexpr u32 BORDER = 5;
static constexpr u32 ARROW_SIZE = 8;
static constexpr u32 ARROW_STROKE = 2;
static constexpr u32 TASK_HEIGHT = 50;
static constexpr u32 TASK_SPACE = 5;
static constexpr u32 TASK_MIN_WIDTH = 50;
static constexpr u32 TASK_MAX_WIDTH = 600;
static constexpr u32 TEXT_BORDER = 2;
static constexpr u32 TEXT_MAX_HEIGHT = TASK_HEIGHT - 20;

/* === Static function(s) === */

/* === Method(s) implementation === */

spider::SchedSVGGanttExporter::SchedSVGGanttExporter(const Schedule *schedule) : Exporter(),
                                                                                 schedule_{ schedule },
                                                                                 widthMin_{ TASK_MIN_WIDTH },
                                                                                 widthMax_{ TASK_MAX_WIDTH } {
    /* == Compute values needed for printing == */
    u64 minExecTime = UINT64_MAX;
    u64 maxExecTime = 0;
    for (auto &task : schedule_->tasks()) {
        const auto &execTime = task->endTime() - task->startTime();
        minExecTime = std::min(execTime, minExecTime);
        maxExecTime = std::max(execTime, maxExecTime);
    }
    const auto ratio = static_cast<double>(maxExecTime) / static_cast<double>(minExecTime);
    if (widthMin_ * ratio > widthMax_) {
        widthMax_ = widthMin_ * ratio;
    }
    alpha_ = widthMax_ / static_cast<double>(maxExecTime);

    /* == Compute dimensions of the Gantt == */
    const auto endPoint = schedule_->stats().minStartTime() + schedule_->stats().makespan();
    makespanWidth_ = computeWidth(endPoint);
    width_ = makespanWidth_ + 2 * BORDER + OFFSET_X + ARROW_STROKE + ARROW_SIZE;
    const auto *platform = archi::platform();
    const auto PECount = platform->PECount();
    height_ = PECount * (TASK_HEIGHT + TASK_SPACE) + TASK_SPACE + ARROW_STROKE + ARROW_SIZE + OFFSET_Y;
}

void spider::SchedSVGGanttExporter::print() const {
    Exporter::printFromPath("./gantt.svg");
}

void spider::SchedSVGGanttExporter::printFromFile(std::ofstream &file) const {
    /* == Print header == */
    headerPrinter(file);

    /* == Print the arrows == */
    axisPrinter(file);

    /* == Print the jobs == */
    for (auto &task : schedule_->tasks()) {
        jobPrinter(file, task.get());
    }

    file << "  </g>" << std::endl;
    file << "</svg>" << std::endl;
}

u64 spider::SchedSVGGanttExporter::computeWidth(u64 time) const {
    return static_cast<u64>(alpha_ * static_cast<double>(time));
}

void spider::SchedSVGGanttExporter::headerPrinter(std::ofstream &file) const {
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

void spider::SchedSVGGanttExporter::axisPrinter(std::ofstream &file) const {

    /* == Print vertical arrow == */
    const auto arrowColor = "393c3c";
    const auto verticalHeight = height_ - ((3 * ARROW_SIZE - 4) / 2);
    file << R"(
    <rect
       fill="#)" << arrowColor << R"("
       stroke="none"
       id="rect_arrow_vertical"
       width=")" << ARROW_STROKE << R"("
       height=")" << verticalHeight << R"("
       x=")" << OFFSET_X << R"("
       y=")" << (ARROW_SIZE - 1) << R"(" />
    <path
       fill="#)" << arrowColor << R"("
       display="inline"
       stroke="none"
       fill-rule="evenodd"
       d="M )" << OFFSET_X + 1 << "," << 0 << " " // x top sommet, y top sommet, x right corner, height, x left corner
         << OFFSET_X + 1 + (ARROW_SIZE / 2) << "," << ARROW_SIZE << " H "
         << OFFSET_X + 1 - (ARROW_SIZE / 2) << R"( Z"
       id="arrow_vertical_head"
       inkscape:connector-curvature="0" />)";

    /* == Print vertical grid == */
    const auto gridColor = "e8e8e8";
    const auto gridCount = makespanWidth_ / 40;
    for (uint32_t i = 0; i <= gridCount; ++i) {
        file << R"(
    <rect
       fill="#)" << gridColor << R"("
       stroke="none"
       id="rect_grid"
       width="1"
       height=")" << verticalHeight << R"("
       x=")" << (OFFSET_X + ARROW_STROKE + BORDER + i * 40) << R"("
       y=")" << (ARROW_SIZE - 1) << R"(" />)";
    }

    file << R"(
    <rect
       fill="#)" << arrowColor << R"("
       stroke="none"
       id="rect_arrow_horizontal"
       width=")" << (width_ - (OFFSET_X + (ARROW_SIZE - 1))) << R"("
       height=")" << ARROW_STROKE << R"("
       x=")" << OFFSET_X << R"("
       y=")" << (height_ - (((ARROW_SIZE + ARROW_STROKE) / 2))) << R"(" />
    <path
       fill="#)" << arrowColor << R"("
       display="inline"
       stroke="none"
       fill-rule="evenodd"
       d="M )" << width_ << "," << (height_ - (ARROW_SIZE / 2)) << " "
         << (width_ - ARROW_SIZE) << "," << height_ << " V "
         << (height_ - ARROW_SIZE) << R"( Z"
       id="arrow_horizontal_head"
       inkscape:connector-curvature="0" />)";
    /* == Print horizontal arrow == */
}

static double computeWidthFromFontSize(double fontSize, size_t count) {
    static constexpr double alpha = 0.6016;
    static constexpr double beta = 0.6855;
    return fontSize * (beta + alpha * static_cast<double>(count));
}

static double computeFontSize(const std::string &name, u64 boxWidth) {
    const auto maxWidth = static_cast<double>(boxWidth - 2 * TEXT_BORDER);
    const auto count = name.length();
    auto width = computeWidthFromFontSize(static_cast<double>(TEXT_MAX_HEIGHT), count);
    if (width > maxWidth) {
        width = maxWidth;
        return width / computeWidthFromFontSize(1.0, count);
    }
    return static_cast<double>(TEXT_MAX_HEIGHT);
}

void spider::SchedSVGGanttExporter::jobPrinter(std::ofstream &file, const ScheduleTask *task) const {
    /* == Compute color and width == */
    const auto name = task->name();
    u32 color = task->color();
    i32 red = static_cast<u8>((color >> 16u) & 0xFFu);
    i32 green = static_cast<u8>((color >> 8u) & 0xFFu);
    i32 blue = static_cast<u8>(color & 0xFFu);
    const auto taskWidth = computeWidth(task->endTime() - task->startTime());

    /* == Compute coordinates == */
    const auto x = OFFSET_X + ARROW_STROKE + BORDER + computeWidth(task->startTime());
    const auto y = height_ - (OFFSET_Y + ARROW_STROKE + (task->mappedPE() + 1) * (TASK_HEIGHT + BORDER));
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
       id=)" << R"("rect_)" + name << R"("
       width=")" << taskWidth << R"("
       height=")" << TASK_HEIGHT << R"("
       x=")" << x << R"("
       y=")" << y << R"("
       ry="4" />)";

    /* == Write the text == */
    const auto fontSize = computeFontSize(name, taskWidth);
    const auto textWidth = computeWidthFromFontSize(fontSize, name.length());
    /* == Don't mind the magic constant for offsetting x and y, they are based on the following observation:
     *    xText = realX - fontSize * alpha; (same for y) and some empirical measurement.
     *    where "realX" is the value you should obtain but fontSize seem to influence text positioning in SVG.. == */
    const auto xText =
            (static_cast<double>(x) + (static_cast<double>(taskWidth) - textWidth) / 2.0) - 0.2588 * fontSize;
    const auto yText =
            (static_cast<double>(y) + (static_cast<double>(TASK_HEIGHT) + fontSize) / 2.0) - 0.2358 * fontSize;
    file << R"(
    <text
       style="font-size:)" << fontSize << R"(px;font-family:monospace;fill:#ffffff;fill-opacity:1;"
       x=")" << xText << R"("
       y=")" << yText << R"("
       >|)" << name << R"(|</text>)";
}


