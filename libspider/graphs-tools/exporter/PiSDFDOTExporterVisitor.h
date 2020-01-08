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
#ifndef SPIDER2_PISDFDOTEXPORTERVISITOR_H
#define SPIDER2_PISDFDOTEXPORTERVISITOR_H

/* === Include(s) === */

#include <graphs/pisdf/visitors/PiSDFDefaultVisitor.h>
#include <graphs-tools/exporter/PiSDFDOTExporter.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/InHeritedParam.h>
#include <graphs/pisdf/DynamicParam.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <cmath>
#include <utility>

namespace spider {
    namespace pisdf {

        /* === Class definition === */

        struct PiSDFDOTExporterVisitor final : public DefaultVisitor {
        public:

            explicit PiSDFDOTExporterVisitor(std::ofstream &file,
                                             std::string offset) : file_{ file },
                                                                   offset_{ std::move(offset) } { }

            /* === Method(s) === */

            void visit(Graph *graph) override;

            inline void visit(DelayVertex *) override { }

            inline void visit(ExecVertex *vertex) override {
                /* == Vertex printer == */
                vertexPrinter(vertex, "#eeeeeeff");
            }

            inline void visit(ConfigVertex *vertex) override {
                /* == Vertex printer == */
                vertexPrinter(vertex, "#ffffccff", 2, "rounded");
            }

            inline void visit(ForkVertex *vertex) override {
                /* == Vertex printer == */
                vertexPrinter(vertex, "#fabe58ff");
            }

            inline void visit(JoinVertex *vertex) override {
                /* == Vertex printer == */
                vertexPrinter(vertex, "#aea8d3ff");
            }

            inline void visit(HeadVertex *vertex) override {
                /* == Vertex printer == */
                vertexPrinter(vertex, "#dcc6e0ff");
            }

            inline void visit(TailVertex *vertex) override {
                /* == Vertex printer == */
                vertexPrinter(vertex, "#f1e7feff");
            }

            inline void visit(DuplicateVertex *vertex) override {
                /* == Vertex printer == */
                vertexPrinter(vertex, "#2c3e50ff");
            }

            inline void visit(RepeatVertex *vertex) override {
                /* == Vertex printer == */
                vertexPrinter(vertex, "#fff68fff");
            }

            inline void visit(InitVertex *vertex) override {
                /* == Vertex printer == */
                vertexPrinter(vertex, "#c8f7c5ff");
            }

            inline void visit(EndVertex *vertex) override {
                /* == Vertex printer == */
                vertexPrinter(vertex, "#ff9478ff");
            }

            inline void visit(InputInterface *interface) override {
                /* == Header == */
                vertexHeaderPrinter("input-" + interface->name(), "#ffffff00", 0);

                /* == Interface printer == */
                interfaceBodyPrinter(interface, "#87d37cff");
            }

            inline void visit(OutputInterface *interface) override {
                /* == Header == */
                vertexHeaderPrinter("output-" + interface->name(), "#ffffff00", 0);

                /* == Interface printer == */
                interfaceBodyPrinter(interface, "#ec644bff");
            }

            inline void visit(Param *param) override {
                paramPrinter(param);
            }

            inline void visit(InHeritedParam *param) override {
                paramPrinter(param);
            }

            inline void visit(DynamicParam *param) override {
                paramPrinter(param);
            }

        private:
            const spider::vector<Param *> *params_ = nullptr;
            std::ofstream &file_;
            std::string offset_;

            inline void vertexHeaderPrinter(const std::string &name,
                                            const std::string &color = "#ffffff00",
                                            int32_t border = 2,
                                            const std::string &style = "") const {
                file_ << offset_ << R"(")" << name
                      << R"(" [shape=plain, color="#393c3c", width=0, height=0, label=<)"
                      << '\n';
                file_ << offset_ << '\t' << R"(<table border=")" << border << R"(" style=")" << style
                      << R"(" bgcolor=")" << color << R"(" fixedsize="false" cellspacing="0" cellpadding="0">)" << '\n';

            }

            std::pair<int32_t, int32_t> computeConstantWidth(Vertex *vertex) const;

            void vertexPrinter(ExecVertex *vertex,
                               const std::string &color,
                               int32_t border = 2,
                               const std::string &style = "") const;

            void interfaceBodyPrinter(Interface *interface, const std::string &color) const;

            void edgePrinter(Edge *edge) const;

            void paramPrinter(Param *param) const;

            template<bool = true>
            inline void dummyPortPrinter(uint32_t width, const std::string &color) const {
                /* == Header == */
                portHeaderPrinter();

                /* == Direction specific export == */
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

                /* == Footer == */
                portFooterPrinter();
            }

            template<bool>
            inline void portPrinter(const Edge *edge, uint32_t width, const std::string &color) const {
                /* == Header == */
                portHeaderPrinter();

                /* == Direction specific export == */
                file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
                      << R"(<td port="in_)" << edge->sinkPortIx()
                      << R"(" border="1" sides="rtb" bgcolor="#87d37cff" align="left" fixedsize="true" width="20" height="20"></td>)"
                      << '\n';
                file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
                      << R"(<td border="1" sides="l" align="left" bgcolor=")" << color
                      << R"(" fixedsize="true" width=")" << width
                      << R"(" height="20"><font point-size="12" face="inconsolata"> )"
                      << edge->sinkRateExpression().evaluate((*params_))
                      << R"(</font></td>)" << '\n';

                /* == Footer == */
                portFooterPrinter();
            }

            inline void portHeaderPrinter() const {
                file_ << offset_ << '\t' << '\t' << '\t' << R"(<td border="0" colspan="1" align="left">)" << '\n';
                file_ << offset_ << '\t' << '\t' << '\t' << '\t'
                      << R"(<table border="0" cellpadding="0" cellspacing="0">)" << '\n';
                file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << R"(<tr>)" << '\n';
            }

            inline void portFooterPrinter() const {
                file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << R"(</tr>)" << '\n';
                file_ << offset_ << '\t' << '\t' << '\t' << '\t' << R"(</table>)" << '\n';
                file_ << offset_ << '\t' << '\t' << '\t' << R"(</td>)" << '\n';
            }
        };

        /* === Inline method(s) === */

        template<>
        inline void PiSDFDOTExporterVisitor::dummyPortPrinter<false>(uint32_t
        width,
        const std::string &color
        ) const {
        /* == Header == */
        portHeaderPrinter();

        /* == Direction specific export == */
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

        /* == Footer == */
        portFooterPrinter();
    }

    template<>
    inline void
    PiSDFDOTExporterVisitor::portPrinter<false>(const Edge *edge, uint32_t width, const std::string &color) const {
        /* == Header == */
        portHeaderPrinter();

        /* == Direction specific export == */
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

        /* == Footer == */
        portFooterPrinter();
    }
}
}
#endif //SPIDER2_PISDFDOTEXPORTERVISITOR_H