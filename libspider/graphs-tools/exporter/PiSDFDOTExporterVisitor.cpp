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

/* === Function(s) definition === */

void spider::pisdf::PiSDFDOTExporterVisitor::visit(Graph *graph) {
    params_ = &(graph->params());
    if (graph->graph()) {
        file_ << offset_ << "subgraph \"cluster_" << graph->name() << "\" {" << '\n';
        offset_ += "\t";
        file_ << offset_ << R"(label=<<font point-size="40" face="inconsolata">)" << graph->name() << R"(</font>>;)"
              << '\n';
        file_ << offset_ << "style=dotted;" << '\n';
        file_ << offset_ << R"(fillcolor="#ffffff")" << '\n';
        file_ << offset_ << R"(color="#393c3c";)" << '\n';
        file_ << offset_ << "penwidth=2;" << '\n';
    } else {
        file_ << "digraph {\n";
        file_ << '\t' << R"(label=<<font point-size="40" face="inconsolata">)" << graph->name() << R"(</font>>;)"
              << '\n';
        file_ << '\t' << "rankdir=LR;" << '\n';
        file_ << '\t' << R"(ranksep="2";)" << '\n';
    }

    /* == Write vertices == */
    file_ << '\n' << offset_ << R"(// Vertices)" << '\n';
    for (const auto &vertex : graph->vertices()) {
        vertex->visit(this);
    }

    /* == Write interfaces in case of hierarchical graphs == */
    file_ << '\n' << offset_ << R"(// Interfaces)" << '\n';
    for (const auto &interface : graph->inputInterfaceArray()) {
        interface->visit(this);
    }
    for (const auto &interface : graph->outputInterfaceArray()) {
        interface->visit(this);
    }

    /* == Write parameters (if any) == */
    file_ << '\n' << offset_ << R"(// Parameters)" << '\n';
    for (const auto &param : graph->params()) {
        param->visit(this);
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
    }
    file_ << "}" << '\n' << '\n';
}

/* === Private method(s) === */

std::pair<int32_t, int32_t> spider::pisdf::PiSDFDOTExporterVisitor::computeConstantWidth(Vertex *vertex) const {
    /* == Compute widths (based on empirical measurements)       == */
    /* ==                           _                        _   == */
    /* ==                          |               1          |  == */
    /* == w(n) = 15*(n-8)*U(n-8) + |20*(1 + ------------------|  == */
    /* ==                          |        1 + exp(-10*(n-7))|  == */
    /* ==                                                        == */
    /* == with U(x) the Heaviside function                       == */
    auto n = static_cast<double>(vertex->name().size());
    const auto &centerWidth = static_cast<int32_t>(15. * (n - 8.) * (n > 8) +
                                                   std::ceil(20. *
                                                             (1 +
                                                              1. / (1 + std::exp(-10. * (n - 7.))))));

    /* == Get the maximum number of digits == */
    double longestRateLen = 0;
    for (const auto &e: vertex->inputEdgeArray()) {
        if (!e) {
            throwSpiderException("vertex [%s]: null input edge.", vertex->name().c_str());
        }
        const auto &rate = e->sinkRateExpression().evaluate((*params_));
        longestRateLen = std::max(longestRateLen, std::log10(rate));
    }
    for (const auto &e: vertex->outputEdgeArray()) {
        if (!e) {
            throwSpiderException("vertex [%s]: null output edge.", vertex->name().c_str());
        }
        const auto &rate = e->sourceRateExpression().evaluate((*params_));
        longestRateLen = std::max(longestRateLen, std::log10(rate));
    }
    return std::make_pair(centerWidth, static_cast<int32_t>(longestRateLen));
}

void spider::pisdf::PiSDFDOTExporterVisitor::vertexPrinter(ExecVertex *vertex,
                                                           const std::string &color,
                                                           int32_t border,
                                                           const std::string &style) const {
    /* == Header == */
    vertexHeaderPrinter(vertex->name(), color, border, style);

    /* == Vertex name == */
    file_ << offset_ << '\t' << '\t'
          << R"(<tr> <td border="0" colspan="4" fixedsize="false" height="10"></td></tr>)"
          << '\n';
    file_ << offset_ << '\t' << '\t'
          << R"(<tr> <td border="0" colspan="4"><font point-size="25" face="inconsolata">)"
          << vertex->name() << "</font></td></tr>" << '\n';

    /* == Get widths == */
    const auto &widthPair = computeConstantWidth(vertex);
    const auto &centerWidth = widthPair.first;
    const auto &rateWidth = static_cast<uint32_t>(32 + std::max(widthPair.second + 1 - 3, 0) * 8);

    /* == Export data ports == */
    uint32_t nOutput = 0;
    for (const auto &edge : vertex->inputEdgeArray()) {
        file_ << offset_ << '\t' << '\t'
              << R"(<tr> <td border="0" style="invis" colspan="4" fixedsize="false" height="10"></td></tr>)"
              << '\n';
        file_ << offset_ << '\t' << '\t' << R"(<tr>)" << '\n';

        /* == Export input port == */
        portPrinter<true>(edge, rateWidth, color);

        /* == Middle separation == */
        file_ << offset_ << '\t' << '\t' << '\t'
              << R"(<td border="0" style="invis" colspan="2" bgcolor=")" << color << R"(" fixedsize="true" width=")"
              << centerWidth
              << R"(" height="20"></td>)" << '\n';

        /* == Export output port == */
        if (nOutput < vertex->outputEdgeCount()) {
            portPrinter<false>(vertex->outputEdge(nOutput), rateWidth, color);
        } else {
            dummyPortPrinter<false>(rateWidth, color);
        }

        file_ << offset_ << '\t' << '\t' << R"(</tr>)" << '\n';
        nOutput += 1;
    }

    /* == Trailing output ports == */
    for (uint32_t i = nOutput; i < vertex->outputEdgeCount(); ++i) {
        auto *edge = vertex->outputEdge(i);
        file_ << offset_ << '\t' << '\t'
              << R"(<tr> <td border="0" style="invis" colspan="4" fixedsize="false" height="10"></td></tr>)"
              << '\n';
        file_ << offset_ << '\t' << '\t' << R"(<tr>)" << '\n';

        /* == Export dummy input port == */
        dummyPortPrinter<true>(rateWidth, color);

        /* == Middle separation == */
        file_ << offset_ << '\t' << '\t' << '\t'
              << R"(<td border="0" style="invis" colspan="2" bgcolor=")" << color << R"(" fixedsize="true" width=")"
              << centerWidth
              << R"(" height="20"></td>)" << '\n';

        /* == Export output port == */
        portPrinter<false>(edge, rateWidth, color);
        file_ << offset_ << '\t' << '\t' << R"(</tr>)" << '\n';
    }

    /* == Footer == */
    file_ << offset_ << '\t' << '\t'
          << R"(<tr> <td border="0" style="invis" colspan="4" fixedsize="false" height="10"></td></tr>)"
          << '\n';
    file_ << offset_ << '\t' << "</table>>" << '\n';
    file_ << offset_ << "];" << '\n' << '\n';
}

void spider::pisdf::PiSDFDOTExporterVisitor::interfaceBodyPrinter(Interface *interface, const std::string &color) const {
    /* == Interface name == */
    file_ << offset_ << '\t' << '\t'
          << R"(<tr> <td border="0" colspan="5" bgcolor="#ffffff00"><font point-size="25" face="inconsolata">)"
          << interface->name() << "</font></td></tr>" << '\n';

    /* == Get widths == */
    const auto &widthPair = computeConstantWidth(interface);
    const auto &balanceWidth = widthPair.first;
    const auto &rateWidth = 24 + std::max(widthPair.second + 1 - 1, 0) * 6;
    auto *inputEdge = interface->inputEdge();
    auto *outputEdge = interface->outputEdge();
    auto inIx = inputEdge->sinkPortIx();
    auto outIx = outputEdge->sourcePortIx();
    auto inRate = inputEdge->sinkRateExpression().evaluate((*params_));
    auto outRate = outputEdge->sourceRateExpression().evaluate((*params_));
    file_ << offset_ << '\t' << '\t'
          << R"(<tr>
                    <td border="0" bgcolor="#ffffff00" fixedsize="true" width=")" << balanceWidth << R"(" height="60"></td>
                    <td port="in_)" << inIx
          << R"(" align="left" border="0" bgcolor="#ffffff00" fixedsize="true" width=")" << rateWidth
          << R"(" height="60"><font point-size="12" face="inconsolata"> )" << inRate << R"(</font></td>
                    <td border="1" bgcolor=")" << color << R"(" fixedsize="true" width="20" height="60"></td>
                    <td port="out_)" << outIx
          << R"(" align="right" border="0" bgcolor="#ffffff00" fixedsize="true" width=")" << rateWidth
          << R"(" height="60"><font point-size="12" face="inconsolata">)" << outRate << R"( </font></td>
                    <td border="0" bgcolor="#ffffff00" fixedsize="true" width=")" << balanceWidth << R"(" height="60"></td>
                </tr>)" << '\n';

    /* == Footer == */
    file_ << offset_ << '\t' << "</table>>" << '\n';
    file_ << offset_ << "];" << '\n' << '\n';
}

struct GetVertexVisitor final : public spider::pisdf::DefaultVisitor {
    void visit(spider::pisdf::ExecVertex *vertex) override {
        vertex_ = vertex;
        name_ = vertex->name();
    }

    void visit(spider::pisdf::InputInterface *interface) override {
        vertex_ = interface;
        name_ = "input-" + vertex_->name();
    }

    void visit(spider::pisdf::OutputInterface *interface) override {
        vertex_ = interface;
        name_ = "output-" + vertex_->name();
    }

    void visit(spider::pisdf::Graph *graph) override {
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
        file_ << offset_ << R"(")" << delay->name()
              << R"(" [shape=circle, style=filled, color="#393c3c", fillcolor="#393c3c", label=""])"
              << '\n';

        /* == Connect source to delay == */
        file_ << offset_ << R"(")" << srcName << R"(":out_)" << srcPortIx << R"(:e -> ")";
        file_ << delay->name() << R"(":w [penwidth=3, color="#393c3c", arrowhead=none];)" << '\n';

        /* == Connect delay to sink == */
        file_ << offset_ << R"(")" << delay->name() << R"(":e -> ")";
        file_ << snkName << R"(":in_)" << snkPortIx << R"(:w [penwidth=3, color="#393c3c", dir=forward];)" << '\n';
    } else if (sink->subtype() == VertexType::DELAY) {
        /* == Connect setter to delay == */
        file_ << offset_ << R"(")" << srcName << R"(":out_)" << srcPortIx << R"(:e -> ")";
        file_ << snkName << R"(":sw [penwidth=3, style=dotted, color="#393c3c", dir=forward];)" << '\n';
    } else if (source->subtype() == VertexType::DELAY) {
        /* == Connect delay to getter == */
        file_ << offset_ << R"(")" << srcName << R"(":se -> ")";
        file_ << snkName << R"(":in_)" << snkPortIx << R"(:w [penwidth=3, color="#393c3c", dir=forward];)" << '\n';
    } else {
        /* == General case == */
        file_ << offset_ << R"(")" << srcName << R"(":out_)" << srcPortIx << R"(:e -> ")";
        file_ << snkName << R"(":in_)" << snkPortIx << R"(:w [penwidth=3, color="#393c3c", dir=forward];)" << '\n';
    }
}

void spider::pisdf::PiSDFDOTExporterVisitor::paramPrinter(Param *param) const {
    file_ << offset_ << R"(")" << param->graph()->name() + ":" + param->name()
          << R"("[shape=house, style=filled, fillcolor=")" << (param->dynamic() ? "#19b5fe" : "#89c4f4")
          << R"(", margin=0, width=0, height=0, label=<)" << '\n';
    file_ << offset_ << '\t' << R"(<table border="0" fixedsize="false" cellspacing="0" cellpadding="0">)"
          << '\n';
    file_ << offset_ << '\t' << '\t'
          << R"(<tr> <td border="0" fixedsize="false" height="20"></td></tr>)" << '\n';
    file_ << offset_ << '\t' << '\t'
          << R"(<tr> <td border="0"><font point-size="20" face="inconsolata">)"
          << param->name() << "</font></td></tr>" << '\n';
    file_ << offset_ << '\t' << R"(</table>>];)" << '\n';
}