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

#include <graphs-tools/exporter/DOTExporter.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/Interface.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Param.h>
#include <cmath>

/* === Static variable(s) === */

/* === Static function(s) === */

static std::string vertexColor(PiSDFVertexType type) {
    switch (type) {
        case PiSDFVertexType::DELAY:
        case PiSDFVertexType::NORMAL:
            return "#eeeeeeff";
        case PiSDFVertexType::FORK:
            return "#fabe58ff";
        case PiSDFVertexType::JOIN:
            return "#aea8d3ff";
        case PiSDFVertexType::DUPLICATE:
            return "#2c3e50ff";
        case PiSDFVertexType::TAIL:
            return "#f1e7feff";
        case PiSDFVertexType::HEAD:
            return "#dcc6e0ff";
        case PiSDFVertexType::INIT:
            return "#c8f7c5ff";
        case PiSDFVertexType::END:
            return "#ff9478ff";
        case PiSDFVertexType::UPSAMPLE:
            return "#fff68fff";
        default:
            return "eeeeeeff";
    }
}

/* === Method(s) implementation === */

void Spider::PiSDF::DOTExporter::print() const {
    print("./pisdf-graph.dot");
}

void Spider::PiSDF::DOTExporter::print(const std::string &path) const {
    std::ofstream file{ path, std::ios::out };
    print(file);

    /* == We should not do this manually but this will ensure that data are correctly written even if it crashes == */
    file.close();
}

void Spider::PiSDF::DOTExporter::print(std::ofstream &file) const {
    graphPrinter(file, graph_);
}

/* === Private method(s) === */

void
Spider::PiSDF::DOTExporter::graphPrinter(std::ofstream &file, const Graph *graph, const std::string &offset) const {
    auto fwOffset{ offset };
    if (graph->containingGraph()) {
        file << fwOffset << "subgraph cluster {" << '\n';
        fwOffset += '\t';
        file << fwOffset << R"(label=<<font point-size="40" face="inconsolata">")" << graph->name()
             << R"("</font>>;)" << '\n';
        file << fwOffset << "style=dotted;" << '\n';
        file << fwOffset << R"(fillcolor="#ffffff")" << '\n';
        file << fwOffset << R"(color="#393c3c";)" << '\n';
        file << fwOffset << "penwidth=2;" << '\n';
    } else {
        file << "digraph {\n";
        file << '\t' << R"(label=<<font point-size="40" face="inconsolata">topgraph</font>>;)" << '\n';
        file << '\t' << "rankdir=LR;" << '\n';
        file << '\t' << R"(ranksep="2";)" << '\n';
    }

    /* == Write vertices == */
    file << '\n' << fwOffset << R"(// Vertices)" << '\n';
    for (const auto &vertex : graph->vertices()) {
        if (!vertex->hierarchical()) {
            vertexPrinter(file, vertex, fwOffset);
        }
    }

    /* == Write interfaces in case of hierarchical graphs == */
    if (graph->containingGraph()) {
        file << '\n' << fwOffset << R"(// Interfaces)" << '\n';
        for (const auto &inputIF : graph->inputInterfaceArray()) {
            inputIFPrinter(file, inputIF, fwOffset);
        }
        for (const auto &outputIF : graph->outputInterfaceArray()) {
            outputIFPrinter(file, outputIF, fwOffset);
        }
    }

    /* == Write parameters (if any) == */
    if (!graph->params().empty()) {
        file << '\n' << fwOffset << R"(// Parameters)" << '\n';
        for (const auto &param : graph->params()) {
            paramPrinter(file, param, fwOffset);
        }
    }

    /* == Write subgraphs (if any == */
    if (!graph->subgraphs().empty()) {
        file << '\n' << fwOffset << R"(// Subgraphs)" << '\n';
        for (const auto &subgraph : graph->subgraphs()) {
            graphPrinter(file, subgraph, fwOffset);
        }
    }

    /* == Write edges == */
    file << '\n' << fwOffset << R"(// Edges)" << '\n';
    for (const auto &edge : graph->edges()) {
        edgePrinter(file, edge, fwOffset);
    }

    /* == Write footer == */
    if (graph->containingGraph()) {
        file << offset;
    }
    file << "}" << '\n';
}

void Spider::PiSDF::DOTExporter::vertexPrinter(std::ofstream &file,
                                               const Vertex *vertex,
                                               const std::string &offset) const {
    file << offset << R"(")" << vertex->name()
         << R"(" [shape=plain, style=filled, fillcolor=")" << vertexColor(vertex->subtype())
         << R"(", width=0, height=0, label=<)" << '\n';
    file << offset << '\t' << R"(<table border="0" fixedsize="false" cellspacing="0" cellpadding="0">)" << '\n';

    /* == Vertex name == */
    file << offset << '\t' << '\t'
         << R"(<tr> <td border="1" sides="lrt" colspan="4" fixedsize="false" height="10"></td></tr>)" << '\n';
    file << offset << '\t' << '\t'
         << R"(<tr> <td border="1" sides="lr" colspan="4"><font point-size="25" face="inconsolata">)"
         << vertex->name() << "</font></td></tr>" << '\n';

    /* == Compute widths == */
    auto n = vertex->name().size();
    auto centerWidth = static_cast<std::uint32_t>(15. * (n - 8.) * (n > 8) +
                                                  std::ceil(20. * (1 + 1. / (1 + std::exp(-10. * (n - 7.))))));
    double longestRateLen = 0;
    for (const auto &e: vertex->inputEdgeArray()) {
        longestRateLen = std::max(longestRateLen, std::log10(e->sinkRateExpression().evaluate(params_)));
    }
    for (const auto &e: vertex->outputEdgeArray()) {
        longestRateLen = std::max(longestRateLen, std::log10(e->sourceRateExpression().evaluate(params_)));
    }
    auto rateWidth = 32 + std::max(static_cast<std::int32_t>(longestRateLen) + 1 - 3, 0) * 8;

    /* == Export data ports == */
    std::uint32_t nOutput = 0;
    for (const auto &edge : vertex->inputEdgeArray()) {
        file << offset << '\t' << '\t'
             << R"(<tr> <td border="1" sides="lr" colspan="4" fixedsize="false" height="10"></td></tr>)" << '\n';
        file << offset << '\t' << '\t' << R"(<tr>)" << '\n';

        /* == Export input port == */
        inputDataPortPrinter(file, edge, offset, rateWidth);

        /* == Middle separation == */
        file << offset << '\t' << '\t' << '\t'
             << R"(<td border="0" colspan="2" bgcolor="#00000000" fixedsize="true" width=")" << centerWidth
             << R"(" height="20"></td>)" << '\n';

        /* == Export output port == */
        if (nOutput < vertex->edgesOUTCount()) {
            outputDataPortPrinter(file, vertex->outputEdge(nOutput), offset, rateWidth);
        } else {
            dummyDataPortPrinter(file, offset, rateWidth, false);
        }

        file << offset << '\t' << '\t' << R"(</tr>)" << '\n';
        nOutput += 1;
    }

    /* == Trailing output ports == */
    for (std::uint32_t i = nOutput; i < vertex->edgesOUTCount(); ++i) {
        auto *edge = vertex->outputEdge(i);
        file << offset << '\t' << '\t'
             << R"(<tr> <td border="1" sides="lr" colspan="4" fixedsize="false" height="10"></td></tr>)" << '\n';
        file << offset << '\t' << '\t' << R"(<tr>)" << '\n';

        /* == Export dummy input port == */
        dummyDataPortPrinter(file, offset, rateWidth, true);

        /* == Middle separation == */
        file << offset << '\t' << '\t' << '\t'
             << R"(<td border="0" colspan="2" bgcolor="#00000000" fixedsize="true" width=")" << centerWidth
             << R"(" height="20"></td>)" << '\n';

        /* == Export output port == */
        outputDataPortPrinter(file, edge, offset, rateWidth);

        file << offset << '\t' << '\t' << R"(</tr>)" << '\n';
    }

    /* == Footer == */
    file << offset << '\t' << '\t'
         << R"(<tr> <td border="1" colspan="4" fixedsize="false" height="10" sides="lbr"></td></tr>)" << '\n';
    file << offset << '\t' << "</table>>" << '\n';
    file << offset << "];" << '\n' << '\n';
}

void Spider::PiSDF::DOTExporter::edgePrinter(std::ofstream &file, const Edge *edge, const std::string &offset) {
    auto *source = edge->source()->type() == Spider::PiSDF::VertexType::GRAPH ? edge->source<true>() : edge->source();
    auto *sink = edge->sink()->type() == Spider::PiSDF::VertexType::GRAPH ? edge->sink<true>() : edge->sink();
    file << offset << R"(")" << source->name();
    file << R"(":out_)" << edge->sourcePortIx() << R"(:e -> ")" << sink->name() << R"(":in_)" << edge->sinkPortIx();
    file << R"(:w [penwidth=3, color="#393c3c", dir=forward];)" << '\n';
}

void
Spider::PiSDF::DOTExporter::paramPrinter(std::ofstream &file, const Param *param, const std::string &offset) {
    file << offset << R"(")" << param->containingGraph()->name() + ":" + param->name()
         << R"("[shape=house, style=filled, fillcolor="#89c4f4", width=0, height=0, label=<)" << '\n';
    file << offset << '\t' << R"(<table border="0" fixedsize="false" cellspacing="0" cellpadding="0">)" << '\n';
    file << offset << '\t' << '\t'
         << R"(<tr> <td border="0" fixedsize="false" height="20"></td></tr>)" << '\n';
    file << offset << '\t' << '\t'
         << R"(<tr> <td border="0"><font point-size="20" face="inconsolata">)"
         << param->name() << "</font></td></tr>" << '\n';
    file << offset << '\t' << R"(</table>>];)" << '\n';

}

void Spider::PiSDF::DOTExporter::inputIFPrinter(std::ofstream &file,
                                                const InputInterface *interface,
                                                const std::string &offset) const {
    file << offset << R"(")" << interface->name()
         << R"(" [shape=plain, style=filled, fillcolor="#fff68fff", width=0, height=0, label=<)" << '\n';
    interfacePrinter(file, interface, offset);
}

void Spider::PiSDF::DOTExporter::outputIFPrinter(std::ofstream &file,
                                                 const OutputInterface *interface,
                                                 const std::string &offset) const {
    file << offset << R"(")" << interface->name()
         << R"(" [shape=plain, style=filled, fillcolor="#dcc6e0ff", width=0, height=0, label=<)" << '\n';
    interfacePrinter(file, interface, offset);
}

void Spider::PiSDF::DOTExporter::interfacePrinter(std::ofstream &file,
                                                  const Interface *interface,
                                                  const std::string &offset) const {
    file << offset << '\t' << R"(<table border="0" fixedsize="false" cellspacing="0" cellpadding="0">)" << '\n';

    /* == Vertex name == */
    file << offset << '\t' << '\t'
         << R"(<tr> <td border="1" sides="lrt" colspan="4" fixedsize="false" height="10"></td></tr>)" << '\n';
    file << offset << '\t' << '\t'
         << R"(<tr> <td border="1" sides="lr" colspan="4"><font point-size="25" face="inconsolata">)"
         << interface->name() << "</font></td></tr>" << '\n';

    /* == Compute widths == */
    auto n = interface->name().size();
    auto centerWidth = static_cast<std::uint32_t>(15. * (n - 8.) * (n > 8) +
                                                  std::ceil(20. * (1 + 1. / (1 + std::exp(-10. * (n - 7.))))));
    double longestRateLen = std::max(0., std::log10(interface->inputEdge()->sinkRateExpression().evaluate(params_)));
    longestRateLen = std::max(longestRateLen,
                              std::log10(interface->outputEdge()->sourceRateExpression().evaluate(params_)));
    auto rateWidth = 32 + std::max(static_cast<std::int32_t>(longestRateLen) + 1 - 3, 0) * 8;

    /* == Export data ports == */
    file << offset << '\t' << '\t'
         << R"(<tr> <td border="1" sides="lr" colspan="4" fixedsize="false" height="10"></td></tr>)" << '\n';
    file << offset << '\t' << '\t' << "<tr>" << '\n';

    /* == Export input port == */
    inputDataPortPrinter(file, interface->inputEdge(), offset, rateWidth);

    /* == Middle separation == */
    file << offset << '\t' << '\t' << '\t'
         << R"(<td border="0" colspan="2" bgcolor="#00000000" fixedsize="true" width=")" << centerWidth
         << R"(" height="20"></td>)" << '\n';

    /* == Export output port == */
    outputDataPortPrinter(file, interface->outputEdge(), offset, rateWidth);
    file << offset << '\t' << '\t' << "</tr>" << '\n';

    /* == Footer == */
    file << offset << '\t' << '\t'
         << R"(<tr> <td border="1" colspan="4" fixedsize="false" height="10" sides="lbr"></td></tr>)" << '\n';
    file << offset << '\t' << "</table>>" << '\n';
    file << offset << "];" << '\n' << '\n';
}


void Spider::PiSDF::DOTExporter::dataPortPrinter(std::ofstream &file,
                                                 const Edge *edge,
                                                 const std::string &offset,
                                                 std::uint32_t width,
                                                 bool input) const {
    /* == Header == */
    file << offset << '\t' << '\t' << '\t' << R"(<td border="0" colspan="1" align="left">)" << '\n';
    file << offset << '\t' << '\t' << '\t' << '\t' << R"(<table border="0" cellpadding="0" cellspacing="0">)" << '\n';
    file << offset << '\t' << '\t' << '\t' << '\t' << '\t' << R"(<tr>)" << '\n';

    /* == Direction specific export == */
    if (input) {
        file << offset << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
             << R"(<td port="in_)" << edge->sinkPortIx()
             << R"(" border="1" bgcolor="#87d37cff" align="left" fixedsize="true" width="20" height="20"></td>)"
             << '\n';
        file << offset << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
             << R"(<td border="0" align="left" bgcolor="#00000000" fixedsize="true" width=")" << width
             << R"(" height="20"><font point-size="12" face="inconsolata"> )"
             << edge->sinkRateExpression().evaluate(params_)
             << R"(</font></td>)" << '\n';
    } else {
        file << offset << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
             << R"(<td border="0" align="right" bgcolor="#00000000" fixedsize="true" width=")" << width
             << R"(" height="20"><font point-size="12" face="inconsolata">)"
             << edge->sourceRateExpression().evaluate(params_)
             << R"( </font></td>)" << '\n';
        file << offset << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
             << R"(<td port="out_)" << edge->sourcePortIx()
             << R"(" border="1" bgcolor="#ec644bff" align="left" fixedsize="true" width="20" height="20"></td>)"
             << '\n';
    }

    /* == Footer == */
    file << offset << '\t' << '\t' << '\t' << '\t' << '\t' << R"(</tr>)" << '\n';
    file << offset << '\t' << '\t' << '\t' << '\t' << R"(</table>)" << '\n';
    file << offset << '\t' << '\t' << '\t' << R"(</td>)" << '\n';
}

void Spider::PiSDF::DOTExporter::inputDataPortPrinter(std::ofstream &file,
                                                      const Edge *edge,
                                                      const std::string &offset,
                                                      std::uint32_t width) const {
    dataPortPrinter(file, edge, offset, width, true);
}

void Spider::PiSDF::DOTExporter::outputDataPortPrinter(std::ofstream &file,
                                                       const Edge *edge,
                                                       const std::string &offset,
                                                       std::uint32_t width) const {
    dataPortPrinter(file, edge, offset, width, false);
}

void Spider::PiSDF::DOTExporter::dummyDataPortPrinter(std::ofstream &file,
                                                      const std::string &offset,
                                                      std::uint32_t width,
                                                      bool input) {
    /* == Header == */
    file << offset << '\t' << '\t' << '\t' << R"(<td border="0" colspan="1" align="left">)" << '\n';
    file << offset << '\t' << '\t' << '\t' << '\t' << R"(<table border="0" cellpadding="0" cellspacing="0">)" << '\n';
    file << offset << '\t' << '\t' << '\t' << '\t' << '\t' << R"(<tr>)" << '\n';

    /* == Direction specific export == */
    if (input) {
        file << offset << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
             << R"(<td border="1" sides="l" bgcolor="#00000000" align="left" fixedsize="true" width="20" height="20"></td>)"
             << '\n';
        file << offset << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
             << R"(<td border="0" align="left" bgcolor="#00000000" fixedsize="true" width=")" << width
             << R"(" height="20"><font color="#00000000" point-size="12" face="inconsolata"> 0</font></td>)" << '\n';
    } else {
        file << offset << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
             << R"(<td border="0" align="right" bgcolor="#00000000" fixedsize="true" width=")" << width
             << R"(" height="20"><font color="#00000000" point-size="12" face="inconsolata">0 </font></td>)" << '\n';
        file << offset << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
             << R"(<td border="1" sides="r" bgcolor="#00000000" align="left" fixedsize="true" width="20" height="20"></td>)"
             << '\n';
    }

    /* == Footer == */
    file << offset << '\t' << '\t' << '\t' << '\t' << '\t' << R"(</tr>)" << '\n';
    file << offset << '\t' << '\t' << '\t' << '\t' << R"(</table>)" << '\n';
    file << offset << '\t' << '\t' << '\t' << R"(</td>)" << '\n';

}

