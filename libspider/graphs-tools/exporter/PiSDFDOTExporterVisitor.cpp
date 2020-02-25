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

#include <graphs-tools/exporter/PiSDFDOTExporterVisitor.h>
#include <graphs/pisdf/Delay.h>
#include <common/Types.h>

/* === Static constant(s) === */

static constexpr size_t MAX_LENGTH = 40;

/* === Function(s) definition === */

void spider::pisdf::PiSDFDOTExporterVisitor::visit(Graph *graph) {
    if (!graph->graph()) {
        printer::fprintf(file_, R"(digraph {
    rankdir = LR;
    ranksep = 1;
    nodesep = 1;)" "\n");
    }

    /* == Subgraph header == */
    params_ = &(graph->params());
    printer::fprintf(file_, "%s" R"(subgraph "cluster_%s" {)" "\n", offset_.c_str(), graph->vertexPath().c_str());
    offset_ += "\t";
    printer::fprintf(file_, "%s" R"(label=<<font point-size="40" face="inconsolata">%s</font>>;)" "\n",
                     offset_.c_str(),
                     graph->name().c_str());
    printer::fprintf(file_, "%s" R"(style=dotted;)" "\n", offset_.c_str());
    printer::fprintf(file_, "%s" R"(fillcolor="#ffffff")" "\n", offset_.c_str());
    printer::fprintf(file_, "%s" R"(color="#393c3c";)" "\n", offset_.c_str());
    printer::fprintf(file_, "%s" R"(penwidth=2;)" "\n", offset_.c_str());

    /* == Write parameters (if any) == */
    printer::fprintf(file_, "\n%s" R"(// Parameters)" "\n", offset_.c_str());
    for (const auto &param : graph->params()) {
        if (param->graph() == graph) {
            param->visit(this);
        }
    }

    /* == Write interfaces in case of hierarchical graphs == */
    printer::fprintf(file_, "\n%s" R"(// Interfaces)" "\n", offset_.c_str());
    if (graph->inputEdgeCount()) {
        printer::fprintf(file_, "%s{\n", offset_.c_str());
        offset_ += "\t";
        printer::fprintf(file_, "%s" R"(rank=source;)" "\n", offset_.c_str());
        for (const auto &interface : graph->inputInterfaceVector()) {
            interface->visit(this);
        }
        offset_.pop_back();
        printer::fprintf(file_, "%s}\n", offset_.c_str());
    }
    if (graph->outputEdgeCount()) {
        printer::fprintf(file_, "%s{\n", offset_.c_str());
        offset_ += "\t";
        printer::fprintf(file_, "%s" R"(rank=sink;)" "\n", offset_.c_str());
        for (const auto &interface : graph->outputInterfaceVector()) {
            interface->visit(this);
        }
        offset_.pop_back();
        printer::fprintf(file_, "%s}\n", offset_.c_str());
    }

    /* == Write vertices == */
    printer::fprintf(file_, "\n%s" R"(// Vertices)" "\n", offset_.c_str());
    for (const auto &vertex : graph->vertices()) {
        if (!vertex->hierarchical()) {
            vertex->visit(this);
        }
    }

    /* == Write subgraphs == */
    if (graph->subgraphCount()) {
        printer::fprintf(file_, "\n%s" R"(// Subgraphs)" "\n", offset_.c_str());
        for (const auto &subgraph : graph->subgraphs()) {
            subgraph->visit(this);
        }
    }

    printer::fprintf(file_, "\n");
    /* == draw invisible edges between params to put them on the same line == */
    for (auto iterator = graph->params().begin(); iterator != graph->params().end(); ++iterator) {
        if ((iterator + 1) != graph->params().end()) {
            auto *param = (*iterator).get();
            auto *nextParam = (*(iterator + 1)).get();
            printer::fprintf(file_, "%s" R"("%s:%s" -> "%s:%s" [style="invis"])" "\n", offset_.c_str(),
                             param->graph()->vertexPath().c_str(), param->name().c_str(),
                             nextParam->graph()->vertexPath().c_str(), nextParam->name().c_str());
        }
    }

    /* == Write edges == */
    printer::fprintf(file_, "\n%s" R"(// Edges)" "\n", offset_.c_str());
    for (const auto &edge : graph->edges()) {
        edgePrinter(edge.get());
    }

    /* == Footer == */
    if (graph->graph()) {
        offset_.pop_back();
        printer::fprintf(file_, "%s}\n\n", offset_.c_str());
    } else {
        printer::fprintf(file_, "\t}\n"
                                "}");
    }
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(ExecVertex *vertex) {
    if (vertex->subtype() == VertexType::DELAY) {
        return;
    }
    /* == Vertex printer == */
    vertexPrinter(vertex);
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(NonExecVertex *vertex) {
    /* == Vertex printer == */
    vertexPrinter(vertex);
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(Interface *interface) {
    /* == Header == */
    vertexHeaderPrinter(interface->vertexPath(), "#ffffff00", 0);
    /* == Interface printer == */
    interfaceBodyPrinter(interface, interface->subtype() == VertexType::INPUT ? "#87d37cff" : "#ec644bff");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(Param *param) {
    paramPrinter(param);
}

/* === Private method(s) === */

int_fast32_t spider::pisdf::PiSDFDOTExporterVisitor::computeMaxDigitCount(Vertex *vertex) const {
    /* == Get the maximum number of digits == */
    int_fast32_t maxDigitCount = 0;
    for (const auto &e: vertex->inputEdgeVector()) {
        if (!e) {
            throwSpiderException("vertex [%s]: null input edge.", vertex->name().c_str());
        }
        const auto &rate = e->sinkRateValue();
        maxDigitCount = std::max(maxDigitCount, static_cast<int_fast32_t>(std::log10(rate)));
    }
    for (const auto &e: vertex->outputEdgeVector()) {
        if (!e) {
            throwSpiderException("vertex [%s]: null output edge.", vertex->name().c_str());
        }
        const auto &rate = e->sourceRateValue();
        maxDigitCount = std::max(maxDigitCount, static_cast<int_fast32_t>(std::log10(rate)));
    }
    return maxDigitCount;
}

void spider::pisdf::PiSDFDOTExporterVisitor::vertexHeaderPrinter(const std::string &name,
                                                                 const std::string &color,
                                                                 int_fast32_t border,
                                                                 const std::string &style) const {
    printer::fprintf(file_, "%s" R"("%s" [shape=plain, color="#393c3c", width=0, height=0, label=<)" "\n",
                     offset_.c_str(),
                     name.c_str());
    printer::fprintf(file_,
                     "%s\t" R"(<table border="%ld" style="%s" bgcolor="%s" fixedsize="false" cellspacing="0" cellpadding="0">)" "\n",
                     offset_.c_str(), border, style.c_str(), color.c_str());
    printer::fprintf(file_, "%s\t\t" R"(<tr> <td border="0" colspan="4" fixedsize="false" height="10"></td></tr>)" "\n",
                     offset_.c_str());
}

void spider::pisdf::PiSDFDOTExporterVisitor::vertexNamePrinter(Vertex *vertex, size_t columnCount) const {
    auto name = vertex->name();
    if (name.size() > MAX_LENGTH) {
        /* == Split name to avoid too big dot vertex == */
        while (!name.empty()) {
            size_t size = std::min(MAX_LENGTH, name.size());
            printer::fprintf(file_,
                             "%s\t\t" R"(<tr> <td border="0" colspan="%zu"><font point-size="25" face="inconsolata">%s</font></td></tr>)" "\n",
                             offset_.c_str(), columnCount, name.substr(0, size).c_str());
            name = name.substr(size, name.size() - size);
        }
    } else {
        printer::fprintf(file_,
                         "%s\t\t" R"(<tr> <td border="0" colspan="%zu"><font point-size="25" face="inconsolata">%s</font></td></tr>)" "\n",
                         offset_.c_str(), columnCount, name.c_str());
    }
}

void spider::pisdf::PiSDFDOTExporterVisitor::vertexPrinter(Vertex *vertex) const {
    static constexpr const char *colors[VERTEX_TYPE_COUNT] = { "#eeeeeeff" /* = NORMAL vertex      = */,
                                                               "#ffffccff" /* = CONFIG vertex      = */,
                                                               "#eeeeeeff" /* = DELAY vertex       = */,
                                                               "#fabe58ff" /* = FORK vertex        = */,
                                                               "#aea8d3ff" /* = JOIN vertex        = */,
                                                               "#fff68fff" /* = REPEAT vertex      = */,
                                                               "#e87e04ff" /* = DUPLICATE vertex   = */,
                                                               "#f1e7feff" /* = TAIL vertex        = */,
                                                               "#dcc6e0ff" /* = HEAD vertex        = */,
                                                               "#c8f7c5ff" /* = INIT vertex        = */,
                                                               "#ff9478ff" /* = END vertex         = */, };
    const auto color = colors[static_cast<uint8_t >(vertex->subtype())];
    /* == Header == */
    vertexHeaderPrinter(vertex->vertexPath(), color, 2, vertex->subtype() == VertexType::CONFIG ? "rounded" : "");

    /* == Vertex name == */
    vertexNamePrinter(vertex, 4);

    /* == Get widths == */
    const auto digitCount = computeMaxDigitCount(vertex);
    const auto rateWidth = 32 + std::max(digitCount - 2, ifast32{ 0 }) * 8;
    const auto nameWidth = static_cast<ifast32>(std::min(vertex->name().length(), MAX_LENGTH) * 16);
    const auto centerWidth = 20 + std::max(nameWidth - (2 * 20 + 2 * rateWidth), ifast32{ 0 });

    /* == Export data ports == */
    size_t nOutput = 0;
    for (const auto &edge : vertex->inputEdgeVector()) {
        printer::fprintf(file_,
                         "%s\t\t" R"(<tr> <td border="0" style="invis" colspan="4" fixedsize="false" height="10"></td></tr>)" "\n",
                         offset_.c_str());
        printer::fprintf(file_, "%s\t\t<tr>\n", offset_.c_str());

        /* == Export input port == */
        portPrinter(edge, rateWidth, color);

        /* == Middle separation == */
        printer::fprintf(file_,
                         "%s\t\t\t" R"(<td border="0" style="invis" colspan="2" bgcolor="%s" fixedsize="true" width="%ld" height="20"></td>)" "\n",
                         offset_.c_str(), color, centerWidth);

        /* == Export output port == */
        if (nOutput < vertex->outputEdgeCount()) {
            portPrinter(vertex->outputEdge(nOutput), rateWidth, color, false);
        } else {
            dummyPortPrinter(rateWidth, color, false);
        }

        printer::fprintf(file_, "%s\t\t</tr>\n", offset_.c_str());
        nOutput += 1;
    }

    /* == Trailing output ports == */
    for (size_t i = nOutput; i < vertex->outputEdgeCount(); ++i) {
        auto *edge = vertex->outputEdge(i);
        printer::fprintf(file_,
                         "%s\t\t" R"(<tr> <td border="0" style="invis" colspan="4" fixedsize="false" height="10"></td></tr>)" "\n",
                         offset_.c_str());
        printer::fprintf(file_, "%s\t\t<tr>\n", offset_.c_str());

        /* == Export dummy input port == */
        dummyPortPrinter(rateWidth, color, true);

        /* == Middle separation == */
        printer::fprintf(file_,
                         "%s\t\t\t" R"(<td border="0" style="invis" colspan="2" bgcolor="%s" fixedsize="true" width="%ld" height="20"></td>)" "\n",
                         offset_.c_str(), color, centerWidth);

        /* == Export output port == */
        portPrinter(edge, rateWidth, color, false);
        printer::fprintf(file_, "%s\t\t</tr>\n", offset_.c_str());
    }

    /* == Footer == */
    printer::fprintf(file_,
                     "%s\t\t" R"(<tr> <td border="0" style="invis" colspan="4" fixedsize="false" height="10"></td></tr>)" "\n",
                     offset_.c_str());
    printer::fprintf(file_, "%s\t</table>>\n", offset_.c_str());
    printer::fprintf(file_, "%s];\n\n", offset_.c_str());
}

void
spider::pisdf::PiSDFDOTExporterVisitor::interfaceBodyPrinter(Interface *interface, const std::string &color) const {
    /* == Interface name == */
    vertexNamePrinter(interface, 5);

    /* == Get widths == */
    const auto &digitCount = computeMaxDigitCount(interface);
    const auto &rateWidth = 24 + digitCount * 6;
    const auto &nameWidth = static_cast<int_fast32_t >(std::min(interface->name().length(), MAX_LENGTH) * 16);
    const auto balanceWidth = std::max((nameWidth - (2 * rateWidth + 20)) / 2, ifast32{ 20 });

    auto *inputEdge = interface->inputEdge();
    auto *outputEdge = interface->outputEdge();
    auto inIx = inputEdge->sinkPortIx();
    auto outIx = outputEdge->sourcePortIx();
    auto inRate = inputEdge->sinkRateValue();
    auto outRate = outputEdge->sourceRateExpression().evaluate((*params_));
    printer::fprintf(file_, "%s\t\t<tr>\n", offset_.c_str());
    printer::fprintf(file_,
                     "%s\t\t\t" R"(<td border="0" bgcolor="#ffffff00" fixedsize="true" width="%ld" height="60"></td>)" "\n",
                     offset_.c_str(), balanceWidth);
    printer::fprintf(file_,
                     "%s\t\t\t" R"(<td port="in_%ld" align="left" border="0" bgcolor="#ffffff00" fixedsize="true" width="%ld" height="60"><font point-size="12" face="inconsolata"> %ld</font></td>)" "\n",
                     offset_.c_str(), inIx, rateWidth, inRate);
    printer::fprintf(file_,
                     "%s\t\t\t" R"(<td border="1" bgcolor="%s" fixedsize="true" width="20" height="60"></td>)" "\n",
                     offset_.c_str(), color.c_str());
    printer::fprintf(file_,
                     "%s\t\t\t" R"(<td port="out_%ld" align="right" border="0" bgcolor="#ffffff00" fixedsize="true" width="%ld" height="60"><font point-size="12" face="inconsolata">%ld </font></td>)" "\n",
                     offset_.c_str(), outIx, rateWidth, outRate);
    printer::fprintf(file_,
                     "%s\t\t\t" R"(<td border="0" bgcolor="#ffffff00" fixedsize="true" width="%ld" height="60"></td>)" "\n",
                     offset_.c_str(), balanceWidth);
    printer::fprintf(file_, "%s\t\t</tr>\n", offset_.c_str());

    /* == Footer == */
    printer::fprintf(file_, "%s\t</table>>\n", offset_.c_str());
    printer::fprintf(file_, "%s];\n\n", offset_.c_str());
}

struct GetVertexVisitor final : public spider::pisdf::DefaultVisitor {

    void doVertex(spider::pisdf::Vertex *vertex) {
        vertex_ = vertex;
        name_ = vertex->vertexPath();
    }

    void visit(spider::pisdf::ExecVertex *vertex) override {
        doVertex(vertex);
    }

    void visit(spider::pisdf::NonExecVertex *vertex) override {
        doVertex(vertex);
    }

    void visit(spider::pisdf::Interface *interface) override {
        doVertex(interface);
    }

    void visit(spider::pisdf::Graph *graph) override {
        source_ ? doVertex(graph->outputInterface(ix_)) : doVertex(graph->inputInterface(ix_));
    }

    spider::pisdf::Vertex *vertex_ = nullptr;
    bool source_ = true;
    size_t ix_ = 0;
    std::string name_;
};

void spider::pisdf::PiSDFDOTExporterVisitor::edgePrinter(Edge *edge) const {
    const auto *delay = edge->delay();
    const auto &srcPortIx = edge->sourcePortIx();
    const auto &snkPortIx = edge->sinkPortIx();
    /* == Get the source and sink == */
    GetVertexVisitor visitor;
    visitor.ix_ = srcPortIx;
    edge->source()->visit(&visitor);
    auto *source = visitor.vertex_;
    auto srcName = std::move(visitor.name_);
    visitor.source_ = false;
    visitor.ix_ = snkPortIx;
    edge->sink()->visit(&visitor);
    auto *sink = visitor.vertex_;
    auto snkName = std::move(visitor.name_);
    if (delay) {
        /* == Draw circle of the delay == */
        printer::fprintf(file_,
                         "%s" R"("%s" [shape=circle, style=filled, color="#393c3c", fillcolor="#393c3c", label=""])" "\n",
                         offset_.c_str(), delay->vertex()->vertexPath().c_str());

        /* == Connect source to delay == */
        printer::fprintf(file_, "%s" R"("%s":out_%ld:e -> "%s":w [penwidth=3, color="#393c3c", dir=forward];)" "\n",
                         offset_.c_str(), srcName.c_str(), srcPortIx, delay->vertex()->vertexPath().c_str());

        /* == Connect delay to sink == */
        printer::fprintf(file_, "%s" R"("%s":e -> "%s":in_%ld:w [penwidth=3, color="#393c3c", dir=forward];)" "\n",
                         offset_.c_str(), delay->vertex()->vertexPath().c_str(), snkName.c_str(), snkPortIx);
    } else if (sink->subtype() == VertexType::DELAY) {
        /* == Connect setter to delay == */
        printer::fprintf(file_,
                         "%s" R"("%s":out_%ld:e -> "%s":sw [penwidth=3, style=dotted, color="#393c3c", dir=forward];)" "\n",
                         offset_.c_str(), srcName.c_str(), srcPortIx, snkName.c_str());
    } else if (source->subtype() == VertexType::DELAY) {
        /* == Connect delay to getter == */
        printer::fprintf(file_,
                         "%s" R"("%s":se -> "%s":in_%ld:w [penwidth=3, style=dotted, color="#393c3c", dir=forward];)" "\n",
                         offset_.c_str(), srcName.c_str(), snkName.c_str(), snkPortIx);
    } else {
        /* == General case == */
        printer::fprintf(file_,
                         "%s" R"("%s":out_%ld:e -> "%s":in_%ld:w [penwidth=3, color="#393c3c", dir=forward];)" "\n",
                         offset_.c_str(), srcName.c_str(), srcPortIx, snkName.c_str(), snkPortIx);
    }
    if (edge->source()->hierarchical() && edge->sink()->hierarchical()) {
        /* == Add invisible edge to insure layout == */
        printer::fprintf(file_, "%s" R"("%s" -> "%s" [style="invis"];)" "\n",
                         offset_.c_str(), source->vertexPath().c_str(), sink->vertexPath().c_str());
    }
}

void spider::pisdf::PiSDFDOTExporterVisitor::paramPrinter(Param *param) const {
    printer::fprintf(file_,
                     "%s" R"("%s:%s"[shape=house, style=filled, fillcolor="%s", margin=0, width=0, height=0, label=<)" "\n",
                     offset_.c_str(), param->graph()->vertexPath().c_str(), param->name().c_str(),
                     (param->dynamic() ? "#19b5fe" : "#89c4f4"));
    printer::fprintf(file_, "%s\t" R"(<table border="0" style="" cellspacing="0" cellpadding="0">)" "\n",
                     offset_.c_str());
    if (param->dynamic()) {
        printer::fprintf(file_,
                         "%s\t\t" R"(<tr> <td border="1" style="rounded" bgcolor="#ffffff" fixedsize="true" width="25" height="25"></td></tr>)" "\n",
                         offset_.c_str());
    }
    printer::fprintf(file_, "%s\t\t" R"(<tr> <td border="0" fixedsize="false" height="20"></td></tr>)" "\n",
                     offset_.c_str());
    printer::fprintf(file_,
                     "%s\t\t" R"(<tr> <td border="0"><font point-size="20" face="inconsolata">%s</font></td></tr>)" "\n",
                     offset_.c_str(), param->name().c_str());
    printer::fprintf(file_, "%s\t</table>>];\n", offset_.c_str());
}

void spider::pisdf::PiSDFDOTExporterVisitor::portHeaderPrinter() const {
    printer::fprintf(file_, "%s\t\t\t" R"(<td border="0" colspan="1" align="left">)" "\n", offset_.c_str());
    printer::fprintf(file_, "%s\t\t\t\t" R"(<table border="0" cellpadding="0" cellspacing="0">)" "\n", offset_.c_str());
    printer::fprintf(file_, "%s\t\t\t\t\t<tr>\n", offset_.c_str());
}

void spider::pisdf::PiSDFDOTExporterVisitor::portFooterPrinter() const {
    printer::fprintf(file_, "%s\t\t\t\t\t</tr>\n", offset_.c_str());
    printer::fprintf(file_, "%s\t\t\t\t</table>\n", offset_.c_str());
    printer::fprintf(file_, "%s\t\t\t</td>\n", offset_.c_str());
}

void spider::pisdf::PiSDFDOTExporterVisitor::portPrinter(const Edge *edge,
                                                         int_fast32_t width,
                                                         const std::string &color,
                                                         bool direction) const {
    /* == Header == */
    portHeaderPrinter();

    /* == Direction specific export == */
    if (direction) {
        printer::fprintf(file_,
                         "%s\t\t\t\t\t\t" R"(<td port="in_%zu" border="1" sides="rtb" bgcolor="#87d37cff" align="left" fixedsize="true" width="20" height="20"></td>)" "\n",
                         offset_.c_str(), edge->sourcePortIx());
        printer::fprintf(file_,
                         "%s\t\t\t\t\t\t" R"(<td border="1" sides="l" align="left" bgcolor="%s" fixedsize="true" width="%ld" height="20"><font point-size="12" face="inconsolata"> )" "%" PRId64 R"(</font></td>)" "\n",
                         offset_.c_str(), color.c_str(), width, edge->sinkRateExpression().evaluate((*params_)));
    } else {
        printer::fprintf(file_,
                         "%s\t\t\t\t\t\t" R"(<td border="1" sides="r" align="right" bgcolor="%s" fixedsize="true" width="%ld" height="20"><font point-size="12" face="inconsolata">)" "%" PRId64 R"( </font></td>)" "\n",
                         offset_.c_str(), color.c_str(), width, edge->sourceRateExpression().evaluate((*params_)));
        printer::fprintf(file_,
                         "%s\t\t\t\t\t\t" R"(<td port="out_%zu" border="1" sides="ltb" bgcolor="#ec644bff" align="left" fixedsize="true" width="20" height="20"></td>)" "\n",
                         offset_.c_str(), edge->sourcePortIx());
    }

    /* == Footer == */
    portFooterPrinter();
}

void spider::pisdf::PiSDFDOTExporterVisitor::dummyPortPrinter(int_fast32_t width,
                                                              const std::string &color,
                                                              bool direction) const {
    /* == Header == */
    portHeaderPrinter();

    /* == Direction specific export == */
    if (direction) {
        printer::fprintf(file_,
                         "%s\t\t\t\t\t\t" R"(<td border="0" style="invis" bgcolor="%s" align="left" fixedsize="true" width="20" height="20"></td>)" "\n",
                         offset_.c_str(), color.c_str());
        printer::fprintf(file_,
                         "%s\t\t\t\t\t\t" R"(<td border="0" style="invis" align="left" bgcolor="%s" fixedsize="true" width="%ld" height="20"><font color="%s" point-size="12" face="inconsolata">0 </font></td>)" "\n",
                         offset_.c_str(), color.c_str(), width, color.c_str());
    } else {
        printer::fprintf(file_,
                         "%s\t\t\t\t\t\t" R"(<td border="0" style="invis" align="right" bgcolor="%s" fixedsize="true" width="%ld" height="20"><font color="%s" point-size="12" face="inconsolata">0 </font></td>)" "\n",
                         offset_.c_str(), color.c_str(), width, color.c_str());
        printer::fprintf(file_,
                         "%s\t\t\t\t\t\t" R"(<td border="0" style="invis" bgcolor="%s" align="right" fixedsize="true" width="20" height="20"></td>)" "\n",
                         offset_.c_str(), color.c_str());
    }

    /* == Footer == */
    portFooterPrinter();
}
