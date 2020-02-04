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

/* === Static constant(s) === */

static constexpr size_t MAX_LENGTH = 30;

/* === Function(s) definition === */

void spider::pisdf::PiSDFDOTExporterVisitor::visit(Graph *graph) {
    if (!graph->graph()) {
        file_ << "digraph {\n";
        file_ << '\t' << R"(rankdir = LR;)" << '\n';
        file_ << '\t' << R"(ranksep = 1;)" << '\n';
        file_ << '\t' << R"(nodesep = 1;)" << '\n';
    }

    /* == Subgraph header == */
    params_ = &(graph->params());
    file_ << offset_ << "subgraph \"cluster_" + graph->vertexPath() + "\" {" << '\n';
    offset_ += "\t";
    file_ << offset_ << R"(label=<<font point-size="40" face="inconsolata">)" << graph->name() << R"(</font>>;)"
          << '\n';
    file_ << offset_ << "style=dotted;" << '\n';
    file_ << offset_ << R"(fillcolor="#ffffff")" << '\n';
    file_ << offset_ << R"(color="#393c3c";)" << '\n';
    file_ << offset_ << "penwidth=2;" << '\n';

    /* == Write parameters (if any) == */
    file_ << '\n' << offset_ << R"(// Parameters)" << '\n';
    for (const auto &param : graph->params()) {
        if (param->graph() == graph) {
            param->visit(this);
        }
    }

    /* == Write interfaces in case of hierarchical graphs == */
    file_ << '\n' << offset_ << R"(// Interfaces)" << '\n';
    if (graph->inputEdgeCount()) {
        file_ << offset_ << "{" << '\n';
        offset_ += "\t";
        file_ << offset_ << "rank=source;" << '\n';
        for (const auto &interface : graph->inputInterfaceVector()) {
            interface->visit(this);
        }
        offset_.pop_back();
        file_ << offset_ << "}" << '\n';
    }
    if (graph->outputEdgeCount()) {
        file_ << offset_ << "{" << '\n';
        offset_ += "\t";
        file_ << offset_ << "rank=sink;" << '\n';
        for (const auto &interface : graph->outputInterfaceVector()) {
            interface->visit(this);
        }
        offset_.pop_back();
        file_ << offset_ << "}" << '\n';
    }

    /* == Write vertices == */
    file_ << '\n' << offset_ << R"(// Vertices)" << '\n';
    for (const auto &vertex : graph->vertices()) {
        if (!vertex->hierarchical()) {
            vertex->visit(this);
        }
    }

    /* == Write subgraphs == */
    if (graph->subgraphCount()) {
        file_ << '\n' << offset_ << R"(// Subgraphs)" << '\n';
        for (const auto &subgraph : graph->subgraphs()) {
            subgraph->visit(this);
        }
    }

    file_ << '\n';
    /* == draw invisible edges between params to put them on the same line == */
    for (auto iterator = graph->params().begin(); iterator != graph->params().end(); ++iterator) {
        if ((iterator + 1) != graph->params().end()) {
            auto *param = (*iterator).get();
            auto *nextParam = (*(iterator + 1)).get();
            file_ << offset_ << R"(")" << param->graph()->vertexPath() + ":" + param->name() << R"(" ->)"
                  << R"(")" << nextParam->graph()->vertexPath() + ":" + nextParam->name() << R"(" [style="invis"])"
                  << '\n';
        }
    }

    /* == Write edges == */
    file_ << '\n' << offset_ << R"(// Edges)" << '\n';
    for (const auto &edge : graph->edges()) {
        edgePrinter(edge);
    }

    /* == Footer == */
    if (graph->graph()) {
        offset_.pop_back();
        file_ << offset_;
        file_ << "}" << '\n' << '\n';
    } else {
        file_ << '\t' << "}";
        file_ << "}";
    }
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(DelayVertex *) { }

void spider::pisdf::PiSDFDOTExporterVisitor::visit(ExecVertex *vertex) {
/* == Vertex printer == */
    vertexPrinter(vertex, "#eeeeeeff");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(NonExecVertex *vertex) {
/* == Vertex printer == */
    vertexPrinter(vertex, "#eeeeeeff");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(ConfigVertex *vertex) {
/* == Vertex printer == */
    vertexPrinter(vertex, "#ffffccff", 2, "rounded");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(ForkVertex *vertex) {
/* == Vertex printer == */
    vertexPrinter(vertex, "#fabe58ff");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(JoinVertex *vertex) {
/* == Vertex printer == */
    vertexPrinter(vertex, "#aea8d3ff");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(HeadVertex *vertex) {
/* == Vertex printer == */
    vertexPrinter(vertex, "#dcc6e0ff");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(TailVertex *vertex) {
/* == Vertex printer == */
    vertexPrinter(vertex, "#f1e7feff");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(DuplicateVertex *vertex) {
/* == Vertex printer == */
    vertexPrinter(vertex, "#e87e04ff");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(RepeatVertex *vertex) {
/* == Vertex printer == */
    vertexPrinter(vertex, "#fff68fff");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(InitVertex *vertex) {
/* == Vertex printer == */
    vertexPrinter(vertex, "#c8f7c5ff");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(EndVertex *vertex) {
/* == Vertex printer == */
    vertexPrinter(vertex, "#ff9478ff");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(InputInterface *interface) {
/* == Header == */
    vertexHeaderPrinter(interface->vertexPath(), "#ffffff00", 0);

/* == Interface printer == */
    interfaceBodyPrinter(interface, "#87d37cff");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(OutputInterface *interface) {
/* == Header == */
    vertexHeaderPrinter(interface->vertexPath(), "#ffffff00", 0);

/* == Interface printer == */
    interfaceBodyPrinter(interface, "#ec644bff");
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(Param *param) {
    paramPrinter(param);
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(InHeritedParam *param) {
    paramPrinter(param);
}

void spider::pisdf::PiSDFDOTExporterVisitor::visit(DynamicParam *param) {
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
    file_ << offset_ << R"(")" << name
          << R"(" [shape=plain, color="#393c3c", width=0, height=0, label=<)"
          << '\n';
    file_ << offset_ << '\t' << R"(<table border=")" << border << R"(" style=")" << style
          << R"(" bgcolor=")" << color << R"(" fixedsize="false" cellspacing="0" cellpadding="0">)" << '\n';

    file_ << offset_ << '\t' << '\t'
          << R"(<tr> <td border="0" colspan="4" fixedsize="false" height="10"></td></tr>)" << '\n';
}

void spider::pisdf::PiSDFDOTExporterVisitor::vertexNamePrinter(Vertex *vertex, size_t columnCount) const {
    auto name = vertex->name();
    if (name.size() > MAX_LENGTH) {
        /* == Split name to avoid too big dot vertex == */
        while (!name.empty()) {
            size_t size = std::min(MAX_LENGTH, name.size());
            file_ << offset_ << '\t' << '\t'
                  << R"(<tr> <td border="0" colspan=")" << columnCount
                  << R"("><font point-size="25" face="inconsolata">)"
                  << name.substr(0, size) << "</font></td></tr>" << '\n';
            name = name.substr(size, name.size() - size);
        }
    } else {
        file_ << offset_ << '\t' << '\t'
              << R"(<tr> <td border="0" colspan=")" << columnCount
              << R"("><font point-size="25" face="inconsolata">)"
              << name << "</font></td></tr>" << '\n';
    }
}

void spider::pisdf::PiSDFDOTExporterVisitor::vertexPrinter(Vertex *vertex,
                                                           const std::string &color,
                                                           int_fast32_t border,
                                                           const std::string &style) const {
    /* == Header == */
    vertexHeaderPrinter(vertex->vertexPath(), color, border, style);

    /* == Vertex name == */
    vertexNamePrinter(vertex, 4);

    /* == Get widths == */
    const auto &digitCount = computeMaxDigitCount(vertex);
    const auto &rateWidth = 32 + std::max(digitCount - 2, 0l) * 8;
    const auto &nameWidth = static_cast<int_fast32_t >(std::min(vertex->name().size(), MAX_LENGTH) * 16);
    const auto centerWidth = 20 + std::max(nameWidth - (2 * 20 + 2 * rateWidth), 0l);

    /* == Export data ports == */
    size_t nOutput = 0;
    for (const auto &edge : vertex->inputEdgeVector()) {
        file_ << offset_ << '\t' << '\t'
              << R"(<tr> <td border="0" style="invis" colspan="4" fixedsize="false" height="10"></td></tr>)"
              << '\n';
        file_ << offset_ << '\t' << '\t' << R"(<tr>)" << '\n';

        /* == Export input port == */
        portPrinter(edge, rateWidth, color);

        /* == Middle separation == */
        file_ << offset_ << '\t' << '\t' << '\t'
              << R"(<td border="0" style="invis" colspan="2" bgcolor=")" << color << R"(" fixedsize="true" width=")"
              << centerWidth
              << R"(" height="20"></td>)" << '\n';

        /* == Export output port == */
        if (nOutput < vertex->outputEdgeCount()) {
            portPrinter(vertex->outputEdge(nOutput), rateWidth, color, false);
        } else {
            dummyPortPrinter(rateWidth, color, false);
        }

        file_ << offset_ << '\t' << '\t' << R"(</tr>)" << '\n';
        nOutput += 1;
    }

    /* == Trailing output ports == */
    for (size_t i = nOutput; i < vertex->outputEdgeCount(); ++i) {
        auto *edge = vertex->outputEdge(i);
        file_ << offset_ << '\t' << '\t'
              << R"(<tr> <td border="0" style="invis" colspan="4" fixedsize="false" height="10"></td></tr>)"
              << '\n';
        file_ << offset_ << '\t' << '\t' << R"(<tr>)" << '\n';

        /* == Export dummy input port == */
        dummyPortPrinter(rateWidth, color, true);

        /* == Middle separation == */
        file_ << offset_ << '\t' << '\t' << '\t'
              << R"(<td border="0" style="invis" colspan="2" bgcolor=")" << color << R"(" fixedsize="true" width=")"
              << centerWidth
              << R"(" height="20"></td>)" << '\n';

        /* == Export output port == */
        portPrinter(edge, rateWidth, color, false);
        file_ << offset_ << '\t' << '\t' << R"(</tr>)" << '\n';
    }

    /* == Footer == */
    file_ << offset_ << '\t' << '\t'
          << R"(<tr> <td border="0" style="invis" colspan="4" fixedsize="false" height="10"></td></tr>)"
          << '\n';
    file_ << offset_ << '\t' << "</table>>" << '\n';
    file_ << offset_ << "];" << '\n' << '\n';
}

void
spider::pisdf::PiSDFDOTExporterVisitor::interfaceBodyPrinter(Interface *interface, const std::string &color) const {
    /* == Interface name == */
    vertexNamePrinter(interface, 5);

    /* == Get widths == */
    const auto &digitCount = computeMaxDigitCount(interface);
    const auto &rateWidth = 24 + digitCount * 6;
    const auto &nameWidth = static_cast<int_fast32_t >(std::min(interface->name().size(), MAX_LENGTH) * 16);
    const auto balanceWidth = std::max((nameWidth - (2 * rateWidth + 20)) / 2, 20l);

    auto *inputEdge = interface->inputEdge();
    auto *outputEdge = interface->outputEdge();
    auto inIx = inputEdge->sinkPortIx();
    auto outIx = outputEdge->sourcePortIx();
    auto inRate = inputEdge->sinkRateValue();
    auto outRate = outputEdge->sourceRateExpression().evaluate((*params_));
    file_ << offset_ << '\t' << '\t'
          << "<tr>"
          << '\n' << offset_ << '\t' << '\t' << '\t'
          << R"(<td border="0" bgcolor="#ffffff00" fixedsize="true" width=")" << balanceWidth
          << R"(" height="60"></td>)"
          << '\n' << offset_ << '\t' << '\t' << '\t'
          << R"(<td port="in_)" << inIx << R"(" align="left" border="0" bgcolor="#ffffff00" fixedsize="true" width=")"
          << rateWidth << R"(" height="60"><font point-size="12" face="inconsolata"> )" << inRate << "</font></td>"
          << '\n' << offset_ << '\t' << '\t' << '\t'
          << R"(<td border="1" bgcolor=")" << color << R"(" fixedsize="true" width="20" height="60"></td>)"
          << '\n' << offset_ << '\t' << '\t' << '\t'
          << R"(<td port="out_)" << outIx
          << R"(" align="right" border="0" bgcolor="#ffffff00" fixedsize="true" width=")" << rateWidth
          << R"(" height="60"><font point-size="12" face="inconsolata">)" << outRate << R"( </font></td>)"
          << '\n' << offset_ << '\t' << '\t' << '\t'
          << R"(<td border="0" bgcolor="#ffffff00" fixedsize="true" width=")" << balanceWidth
          << R"(" height="60"></td>)"
          << '\n' << offset_ << '\t' << '\t'
          << "</tr>" << '\n';

    /* == Footer == */
    file_ << offset_ << '\t' << "</table>>" << '\n';
    file_ << offset_ << "];" << '\n' << '\n';
}

struct GetVertexVisitor final : public spider::pisdf::DefaultVisitor {

    void doVertex(spider::pisdf::Vertex *vertex) {
        vertex_ = vertex;
        name_ = vertex->vertexPath();
    }

    void visit(spider::pisdf::ExecVertex *vertex) {
        this->doVertex(static_cast<spider::pisdf::Vertex *>(vertex));
    }

    void visit(spider::pisdf::NonExecVertex *vertex) {
        this->doVertex(static_cast<spider::pisdf::Vertex *>(vertex));
    }

    void visit(spider::pisdf::InputInterface *interface) {
        this->doVertex(static_cast<spider::pisdf::Vertex *>(interface));
    }

    void visit(spider::pisdf::OutputInterface *interface) {
        this->doVertex(static_cast<spider::pisdf::Vertex *>(interface));
    }

    void visit(spider::pisdf::Graph *graph) {
        source_ ? this->visit(graph->outputInterface(ix_)) : this->visit(graph->inputInterface(ix_));
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
        file_ << offset_ << R"(")" << delay->vertex()->vertexPath()
              << R"(" [shape=circle, style=filled, color="#393c3c", fillcolor="#393c3c", label=""])"
              << '\n';

        /* == Connect source to delay == */
        file_ << offset_ << R"(")" << srcName << R"(":out_)" << srcPortIx << R"(:e -> ")";
        file_ << delay->vertex()->vertexPath() << R"(":w [penwidth=3, color="#393c3c", arrowhead=none];)" << '\n';

        /* == Connect delay to sink == */
        file_ << offset_ << R"(")" << delay->vertex()->vertexPath() << R"(":e -> ")";
        file_ << snkName << R"(":in_)" << snkPortIx << R"(:w [penwidth=3, color="#393c3c", dir=forward];)" << '\n';
    } else if (sink->subtype() == VertexType::DELAY) {
        /* == Connect setter to delay == */
        file_ << offset_ << R"(")" << srcName << R"(":out_)" << srcPortIx << R"(:e -> ")";
        file_ << snkName << R"(":sw [penwidth=3, style=dotted, color="#393c3c", dir=forward];)" << '\n';
    } else if (source->subtype() == VertexType::DELAY) {
        /* == Connect delay to getter == */
        file_ << offset_ << R"(")" << srcName << R"(":se -> ")";
        file_ << snkName << R"(":in_)" << snkPortIx << R"(:w [penwidth=3, style=dotted, color="#393c3c", dir=forward];)"
              << '\n';
    } else {
        /* == General case == */
        file_ << offset_ << R"(")" << srcName << R"(":out_)" << srcPortIx << R"(:e -> ")";
        file_ << snkName << R"(":in_)" << snkPortIx << R"(:w [penwidth=3, color="#393c3c", dir=forward];)" << '\n';
    }
    if (edge->source()->hierarchical() && edge->sink()->hierarchical()) {
        /* == Add invisible edge to insure layout == */
        file_ << offset_ << '\"' << source->vertexPath() << R"(" -> ")"
              << sink->vertexPath() << R"(" [style="invis"];)" << '\n';
    }
}

void spider::pisdf::PiSDFDOTExporterVisitor::paramPrinter(Param *param) const {
    file_ << offset_ << R"(")" << param->graph()->vertexPath() + ":" + param->name()
          << R"("[shape=house, style=filled, fillcolor=")" << (param->dynamic() ? "#19b5fe" : "#89c4f4")
          << R"(", margin=0, width=0, height=0, label=<)" << '\n';
    file_ << offset_ << '\t' << R"(<table border="0" style="" cellspacing="0" cellpadding="0">)"
          << '\n';
    if (param->dynamic()) {
        file_ << offset_ << '\t' << '\t'
              << R"(<tr> <td border="1" style="rounded" bgcolor="#ffffff" fixedsize="true" width="25" height="25"></td></tr>)"
              << '\n';
    }
    file_ << offset_ << '\t' << '\t'
          << R"(<tr> <td border="0" fixedsize="false" height="20"></td></tr>)" << '\n';
    file_ << offset_ << '\t' << '\t'
          << R"(<tr> <td border="0"><font point-size="20" face="inconsolata">)"
          << param->name() << "</font></td></tr>" << '\n';
    file_ << offset_ << '\t' << R"(</table>>];)" << '\n';
}

void spider::pisdf::PiSDFDOTExporterVisitor::portHeaderPrinter() const {
    file_ << offset_ << '\t' << '\t' << '\t' << R"(<td border="0" colspan="1" align="left">)" << '\n';
    file_ << offset_ << '\t' << '\t' << '\t' << '\t'
          << R"(<table border="0" cellpadding="0" cellspacing="0">)" << '\n';
    file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << R"(<tr>)" << '\n';
}

void spider::pisdf::PiSDFDOTExporterVisitor::portFooterPrinter() const {
    file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << R"(</tr>)" << '\n';
    file_ << offset_ << '\t' << '\t' << '\t' << '\t' << R"(</table>)" << '\n';
    file_ << offset_ << '\t' << '\t' << '\t' << R"(</td>)" << '\n';
}

void spider::pisdf::PiSDFDOTExporterVisitor::portPrinter(const Edge *edge,
                                                         int_fast32_t width,
                                                         const std::string &color,
                                                         bool direction) const {
    /* == Header == */
    portHeaderPrinter();

    /* == Direction specific export == */
    if (direction) {
        file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
              << R"(<td port="in_)" << edge->sinkPortIx()
              << R"(" border="1" sides="rtb" bgcolor="#87d37cff" align="left" fixedsize="true" width="20" height="20"></td>)"
              << '\n';
        file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
              << R"(<td border="1" sides="l" align="left" bgcolor=")" << color
              << R"(" fixedsize="true" width=")" << width
              << R"(" height="20"><font point-size="12" face="inconsolata"> )"
              << edge->sinkRateValue()
              << R"(</font></td>)" << '\n';
    } else {
        file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
              << R"(<td border="1" sides="r" align="right" bgcolor=")" << color << R"(" fixedsize="true" width=")"
              << width
              << R"(" height="20"><font point-size="12" face="inconsolata">)"
              << edge->sourceRateExpression().evaluate((*params_))
              << R"( </font></td>)" << '\n';
        file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
              << R"(<td port="out_)" << edge->sourcePortIx()
              << R"(" border="1" sides="ltb" bgcolor="#ec644bff" align="left" fixedsize="true" width="20" height="20"></td>)"
              << '\n';
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
        file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
              << R"(<td border="0" style="invis" bgcolor=")" << color
              << R"(" align="left" fixedsize="true" width="20" height="20"></td>)"
              << '\n';
        file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
              << R"(<td border="0" style="invis" align="left" bgcolor=")" << color
              << R"(" fixedsize="true" width=")" << width
              << R"(" height="20"><font color=")" << color
              << R"(" point-size="12" face="inconsolata"> 0</font></td>)"
              << '\n';
    } else {
        file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
              << R"(<td border="0" style="invis" align="right" bgcolor=")" << color
              << R"(" fixedsize="true" width=")" << width
              << R"(" height="20"><font color=")" << color
              << R"(" point-size="12" face="inconsolata">0 </font></td>)"
              << '\n';
        file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
              << R"(<td border="0" style="invis" bgcolor=")" << color
              << R"(" align="left" fixedsize="true" width="20" height="20"></td>)"
              << '\n';
    }

    /* == Footer == */
    portFooterPrinter();
}
