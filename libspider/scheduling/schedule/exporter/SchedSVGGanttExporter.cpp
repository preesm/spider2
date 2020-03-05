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
static constexpr u32 TEXT_MAX_HEIGHT = (TASK_HEIGHT - 10);
static constexpr double PE_FONT_SIZE = (TEXT_MAX_HEIGHT / 3.);
static constexpr double X_FONT_OFFSET = 0.2588;
static constexpr double Y_FONT_OFFSET = 0.2358;

/* === Static function(s) === */

static double computeWidthFromFontSize(double fontSize, size_t count) {
    static constexpr double alpha = 0.6016;
    static constexpr double beta = 0.6855;
    return fontSize * (beta + alpha * static_cast<double>(count));
}

static double computeFontSize(const std::string &name, u64 boxWidth) {
    const auto maxWidth = static_cast<double>(boxWidth - 2 * TEXT_BORDER);
    const auto count = name.length();
    constexpr auto MAX_TEXT_FONT_SIZE = (static_cast<double>(TEXT_MAX_HEIGHT) - 2.) * 3. / 5.;
    auto width = computeWidthFromFontSize(MAX_TEXT_FONT_SIZE, count);
    if (width > maxWidth) {
        width = maxWidth;
        return width / computeWidthFromFontSize(1.0, count);
    }
    return MAX_TEXT_FONT_SIZE;
}

static double computeRelativeCenteredX(double xAnchor, double widthAnchor, double width, double fontSize) {
    return (xAnchor + ((widthAnchor - width) / 2.0)) - (X_FONT_OFFSET * fontSize);
}

static double computeRelativeCenteredY(double yAnchor, double heightAnchor, double height, double fontSize) {
    return (yAnchor + ((heightAnchor - height) / 2.0) + fontSize) - (Y_FONT_OFFSET * fontSize);
}

static void printRect(FILE *file, const std::string &color, double width, double height, double x, double y) {
    spider::printer::fprintf(file, R"(
    <rect
       fill="#%s"
       stroke="none"
       width="%f"
       height="%f"
       x="%f"
       y="%f" />)" "\n", color.c_str(), width, height, x, y);
}

static void printText(FILE *file, const std::string &text, double size, double x, double y,
                      const std::string &color = "000000") {
    spider::printer::fprintf(file, R"(
    <text
       style="font-size:%fpx;font-family:monospace;fill:#%s;fill-opacity:1;"
       x="%f"
       y="%f"
       ><tspan style="fill:none">|</tspan>%s<tspan style="fill:none">|</tspan></text>)" "\n",
                             size, color.c_str(), x, y, text.c_str());
}

/* === Method(s) implementation === */

spider::SchedSVGGanttExporter::SchedSVGGanttExporter(const Schedule *schedule) : Exporter(),
                                                                                 schedule_{ schedule },
                                                                                 widthMin_{ TASK_MIN_WIDTH },
                                                                                 widthMax_{ TASK_MAX_WIDTH },
                                                                                 offsetX_{ OFFSET_X } {
    /* == Compute values needed for printing == */
    u64 minExecTime = UINT64_MAX;
    u64 maxExecTime = 0;
    for (auto &task : schedule_->tasks()) {
        const auto execTime = task->endTime() - task->startTime();
        minExecTime = std::min(execTime, minExecTime);
        maxExecTime = std::max(execTime, maxExecTime);
    }
    const auto ratio = static_cast<double>(maxExecTime) / static_cast<double>(minExecTime + 1);
    if (widthMin_ * ratio > widthMax_) {
        widthMax_ = widthMin_ * ratio;
    }
    alpha_ = widthMax_ / static_cast<double>(maxExecTime);

    /* == Compute dimensions of the Gantt == */
    offsetX_ = computeRealXOffset();
    makespanWidth_ = computeWidth(schedule_->stats().minStartTime() + schedule_->stats().makespan());
    width_ = makespanWidth_ + 2 * BORDER + offsetX_ + ARROW_STROKE + ARROW_SIZE;
    height_ = archi::platform()->PECount() * (TASK_HEIGHT + TASK_SPACE) + TASK_SPACE + ARROW_STROKE + ARROW_SIZE +
              OFFSET_Y;
}

void spider::SchedSVGGanttExporter::print() const {
    Exporter::printFromPath("./gantt.svg");
}

void spider::SchedSVGGanttExporter::printFromFile(FILE *file) const {
    /* == Print header == */
    headerPrinter(file);

    /* == Print the name of the processors == */
    pePrinter(file);

    /* == Print the arrows == */
    axisPrinter(file);

    /* == Print the jobs == */
    for (auto &task : schedule_->tasks()) {
        taskPrinter(file, task.get());
    }
    printer::fprintf(file, " </g>\n");
    printer::fprintf(file, "</svg>");
}

u32 spider::SchedSVGGanttExporter::computeRealXOffset() const {
    auto maxWidth = static_cast<double>(OFFSET_X);
    auto *archi = archi::platform();
    for (auto &pe : archi->peArray()) {
        if (schedule_->stats().utilizationFactor(pe->virtualIx()) > 0.) {
            const auto width = computeWidthFromFontSize(PE_FONT_SIZE, pe->name().length());
            maxWidth = std::max(maxWidth, width);
        }
    }
    return static_cast<u32>(maxWidth);
}

u64 spider::SchedSVGGanttExporter::computeWidth(u64 time) const {
    return static_cast<u64>(alpha_ * static_cast<double>(time));
}

void spider::SchedSVGGanttExporter::pePrinter(FILE *file) const {
    /* == Print the name of the processors == */
    auto *archi = archi::platform();
    for (auto &pe : archi->peArray()) {
        if (schedule_->stats().utilizationFactor(pe->virtualIx()) > 0.) {
            const auto yLine = height_ - (OFFSET_Y + ARROW_STROKE + (pe->virtualIx() + 1) * (TASK_HEIGHT + BORDER));
            const auto yText = computeRelativeCenteredY(static_cast<double>(yLine), static_cast<double>(TASK_HEIGHT),
                                                        PE_FONT_SIZE, PE_FONT_SIZE);
            /* == Print the PE name == */
            printText(file, pe->name(), PE_FONT_SIZE, -(X_FONT_OFFSET * PE_FONT_SIZE), yText);
        }

    }
}

void spider::SchedSVGGanttExporter::headerPrinter(FILE *file) const {
    printer::fprintf(file, R"(<?xml version="1.0" encoding="UTF-8" standalone="no"?>
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
   width=")" "%" PRIu64 R"("
   height=")" "%" PRIu64 R"(">
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
     inkscape:groupmode="layer">)" "\n", width_, height_);
}

void spider::SchedSVGGanttExporter::axisPrinter(FILE *file) const {
    const auto arrowColor = "393c3c";
    const auto verticalHeight = static_cast<double>((height_ - ((3 * ARROW_SIZE - 4) / 2)));
    /* == Print vertical arrow == */
    printRect(file, arrowColor, ARROW_STROKE, verticalHeight,
              static_cast<double>(offsetX_), static_cast<double>(ARROW_SIZE - 1));
    printer::fprintf(file, R"(
    <path
       fill="#%s"
       display="inline"
       stroke="none"
       fill-rule="evenodd"
       d="M )" "%" PRIu32 R"(,0 )" "%" PRIu32 R"(,)" "%" PRIu32 R"( H )" "%" PRIu32 R"( Z"
       id="arrow_vertical_head"
       inkscape:connector-curvature="0" />)",
                     arrowColor, offsetX_ + 1, offsetX_ + 1 + (ARROW_SIZE / 2), ARROW_SIZE,
                     offsetX_ + 1 - (ARROW_SIZE / 2));
    // x top sommet, y top sommet, x right corner, height, x left corner

    /* == Print vertical grid == */
    const auto gridCount = makespanWidth_ / 40;
    for (uint32_t i = 0; i <= gridCount; ++i) {
        printRect(file, "e8e8e8",
                  1.,
                  verticalHeight,
                  (offsetX_ + ARROW_STROKE + BORDER + i * 40),
                  (ARROW_SIZE - 1));
    }

    /* == Print horizontal arrow == */
    printRect(file, arrowColor,
              static_cast<double >(width_ - (offsetX_ + (ARROW_SIZE - 1))),
              ARROW_STROKE,
              offsetX_,
              static_cast<double>((height_ - ((ARROW_SIZE + ARROW_STROKE) / 2))));

    printer::fprintf(file, R"(
    <path
       fill="#%s"
       display="inline"
       stroke="none"
       fill-rule="evenodd"
       d="M )" "%" PRIu64 R"(,)" "%" PRIu64 R"( )" "%" PRIu64 R"(,)" "%" PRIu64 R"( V )" "%" PRIu64 R"( Z"
       id="arrow_horizontal_head"
       inkscape:connector-curvature="0" />)",
                     arrowColor, width_, (height_ - (ARROW_SIZE / 2)), (width_ - ARROW_SIZE), height_,
                     (height_ - ARROW_SIZE));
}

void spider::SchedSVGGanttExporter::taskPrinter(FILE *file, const ScheduleTask *task) const {
    /* == Compute color and width == */
    const auto name = task->name();

    /* == Compute coordinates == */
    const auto taskWidth = computeWidth((task->endTime() - task->startTime()));
    const auto x = offsetX_ + ARROW_STROKE + BORDER + computeWidth(task->startTime());
    const auto y = height_ - (OFFSET_Y + ARROW_STROKE + (task->mappedPe() + 1) * (TASK_HEIGHT + BORDER));

    /* == Print rect == */
    u32 color = task->color();
    i32 red = static_cast<u8>((color >> 16u) & 0xFFu);
    i32 green = static_cast<u8>((color >> 8u) & 0xFFu);
    i32 blue = static_cast<u8>(color & 0xFFu);
    printer::fprintf(file, R"(
    <g>
        <rect
           fill="#%.2X%.2X%.2X"
           stroke="none"
           id="rect_%s"
           width=")" "%" PRIu64 R"("
           height=")" "%" PRIu32 R"("
           x=")" "%" PRIu64 R"("
           y=")" "%" PRIu64 R"("
           ry="10" />)" "\n", red, green, blue, name.c_str(), taskWidth, TASK_HEIGHT, x, y);


    /* == Write the text == */
    const auto fontSize = computeFontSize(name, taskWidth);
    const auto textWidth = computeWidthFromFontSize(fontSize, name.length());
    /* == Don't mind the magic constant for offsetting x and y, they are based on the following observation:
     *    xText = realX - fontSize * alpha; (same for y) and some empirical measurement.
     *    where "realX" is the value you should obtain but fontSize seem to influence text positioning in SVG.. == */
    const auto xText = computeRelativeCenteredX(static_cast<double>(x), static_cast<double>(taskWidth), textWidth,
                                                fontSize);
    const auto yText = computeRelativeCenteredY(static_cast<double>(y), static_cast<double>(TASK_HEIGHT),
                                                (5. * fontSize / 3.) + 2., fontSize);
    printText(file, name, fontSize, xText, yText, "ffffff");

    const auto timeFontSize = fontSize / 1.5;
    const auto timeString = std::string("[").append(std::to_string(task->startTime()).append(":").append(
            std::to_string(task->endTime()).append("]")));
    const auto timeWidth = computeWidthFromFontSize(timeFontSize, timeString.length());
    const auto xTime = computeRelativeCenteredX(xText, textWidth, timeWidth, timeFontSize);
    const auto yTime = yText + fontSize + 2 - Y_FONT_OFFSET * timeFontSize;
    printText(file, timeString, timeFontSize, xTime, yTime, "ffffff");
    printer::fprintf(file, "</g>");
}


