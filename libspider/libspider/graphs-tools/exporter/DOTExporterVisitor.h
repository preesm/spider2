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
#ifndef SPIDER2_DOTEXPORTERVISITOR_H
#define SPIDER2_DOTEXPORTERVISITOR_H

/* === Include(s) === */

#include <graphs/pisdf/visitors/DefaultVisitor.h>
#include <graphs-tools/exporter/DOTExporter.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>
#include <graphs/pisdf/params/InHeritedParam.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <cmath>

namespace Spider {
    namespace PiSDF {

        /* === Class definition === */

        struct DOTExporterVisitor final : public DefaultVisitor {
        public:

            explicit DOTExporterVisitor(const DOTExporter *exporter,
                                        std::ofstream &file,
                                        const std::string &offset) : exporter_{ exporter },
                                                                     file_{ file },
                                                                     offset_{ offset } { }

            /* === Method(s) === */

            void visit(Graph *graph) override;

            inline void visit(ExecVertex *vertex) override {
                if (vertex->subtype() == VertexType::DELAY) {
                    return;
                }
                /* == Header == */
                vertexHeaderPrinter(vertex->name(), vertexColor(vertex));

                /* == Vertex printer == */
                vertexBodyPrinter(vertex);
            }

            inline void visit(InputInterface *interface) override {
                /* == Header == */
                vertexHeaderPrinter("input-" + interface->name());

                /* == Interface printer == */
                interfaceBodyPrinter(interface, "#87d37cff");
            }

            inline void visit(OutputInterface *interface) override {
                /* == Header == */
                vertexHeaderPrinter("output-" + interface->name());

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
            const DOTExporter *exporter_ = nullptr;
            std::ofstream &file_;
            std::string offset_;

            static std::string vertexColor(ExecVertex *vertex) {
                switch (vertex->subtype()) {
                    case PiSDFVertexType::DELAY:
                    case PiSDFVertexType::CONFIG:
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
                    case PiSDFVertexType::REPEAT:
                        return "#fff68fff";
                    default:
                        return "eeeeeeff";
                }
            }

            inline void vertexHeaderPrinter(const std::string &name, const std::string &color = "#ffffff00") const {
                file_ << offset_ << R"(")" << name
                      << R"(" [shape=plain, style=filled, fillcolor=")" << color << R"(", width=0, height=0, label=<)"
                      << '\n';
                file_ << offset_ << '\t' << R"(<table border="0" fixedsize="false" cellspacing="0" cellpadding="0">)"
                      << '\n';

            }

            std::pair<std::int32_t, std::int32_t> computeConstantWidth(Vertex *vertex) const;

            void vertexBodyPrinter(ExecVertex *vertex) const;

            void interfaceBodyPrinter(Interface *interface, const std::string &color) const;

            void edgePrinter(Edge *edge) const;

            void paramPrinter(Param *param) const;

            template<bool = true>
            inline void dummyPortPrinter(std::uint32_t width) const {
                /* == Header == */
                portHeaderPrinter();

                /* == Direction specific export == */
                file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
                      << R"(<td border="1" sides="l" bgcolor="#ffffff00" align="left" fixedsize="true" width="20" height="20"></td>)"
                      << '\n';
                file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
                      << R"(<td border="0" align="left" bgcolor="#ffffff00" fixedsize="true" width=")" << width
                      << R"(" height="20"><font color="#ffffff00" point-size="12" face="inconsolata"> 0</font></td>)"
                      << '\n';

                /* == Footer == */
                portFooterPrinter();
            }

            template<bool>
            inline void portPrinter(const Edge *edge, std::uint32_t width) const {
                /* == Header == */
                portHeaderPrinter();

                /* == Direction specific export == */
                file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
                      << R"(<td port="in_)" << edge->sinkPortIx()
                      << R"(" border="1" bgcolor="#87d37cff" align="left" fixedsize="true" width="20" height="20"></td>)"
                      << '\n';
                file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
                      << R"(<td border="0" align="left" bgcolor="#ffffff00" fixedsize="true" width=")" << width
                      << R"(" height="20"><font point-size="12" face="inconsolata"> )"
                      << edge->sinkRateExpression().evaluate(exporter_->params_)
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
        inline void DOTExporterVisitor::dummyPortPrinter<false>(std::uint32_t width) const {
            /* == Header == */
            portHeaderPrinter();

            /* == Direction specific export == */
            file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
                  << R"(<td border="0" align="right" bgcolor="#ffffff00" fixedsize="true" width=")" << width
                  << R"(" height="20"><font color="#00000000" point-size="12" face="inconsolata">0 </font></td>)"
                  << '\n';
            file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
                  << R"(<td border="1" sides="r" bgcolor="#ffffff00" align="left" fixedsize="true" width="20" height="20"></td>)"
                  << '\n';

            /* == Footer == */
            portFooterPrinter();
        }

        template<>
        inline void DOTExporterVisitor::portPrinter<false>(const Edge *edge, std::uint32_t width) const {
            /* == Header == */
            portHeaderPrinter();

            /* == Direction specific export == */
            file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
                  << R"(<td border="0" align="right" bgcolor="#ffffff00" fixedsize="true" width=")" << width
                  << R"(" height="20"><font point-size="12" face="inconsolata">)"
                  << edge->sourceRateExpression().evaluate(exporter_->params_)
                  << R"( </font></td>)" << '\n';
            file_ << offset_ << '\t' << '\t' << '\t' << '\t' << '\t' << '\t'
                  << R"(<td port="out_)" << edge->sourcePortIx()
                  << R"(" border="1" bgcolor="#ec644bff" align="left" fixedsize="true" width="20" height="20"></td>)"
                  << '\n';

            /* == Footer == */
            portFooterPrinter();
        }
    }
}
#endif //SPIDER2_DOTEXPORTERVISITOR_H
