/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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

#include <graphs-tools/helper/visitors/PiSDFDefaultVisitor.h>
#include <graphs-tools/exporter/PiSDFDOTExporter.h>
#include <graphs/pisdf/InHeritedParam.h>
#include <graphs/pisdf/DynamicParam.h>
#include <utility>

namespace spider {
    namespace pisdf {

        /* === Class definition === */

        struct PiSDFDOTExporterVisitor final : public DefaultVisitor {
        public:

            explicit PiSDFDOTExporterVisitor(FILE *file, std::string offset) : file_{ file },
                                                                               offset_{ std::move(offset) } { }

            /* === Method(s) === */

            void visit(Graph *graph) override;

            void visit(ExecVertex *vertex) override;

            void visit(NonExecVertex *vertex) override;

            void visit(Interface *interface) override;

            void visit(Param *param) override;

        private:
            const spider::vector<std::shared_ptr<Param>> *params_ = nullptr;
            FILE *file_;
            std::string offset_;

            /**
             * @brief
             * @param name     Name of the vertex.
             * @param color    Color of the vertex in format "#rrggbbaa"
             * @param border   Size of the border of the vertex.
             * @param style    Style of the vertex shape (rounded, box, etc.).
             */
            void vertexHeaderPrinter(const std::string &name,
                                     const std::string &color = "#ffffff00",
                                     int_fast32_t border = 2,
                                     const std::string &style = "") const;

            int_fast32_t computeMaxDigitCount(const Vertex *vertex) const;

            /**
             * @brief Prints a vertex name to DOT with stripped size.
             * @param vertex       Pointer to the vertex.
             * @param columnCount  Number of column in the html table.
             */
            void vertexNamePrinter(const Vertex *vertex, size_t columnCount) const;

            /**
             * @brief Prints a vertex into DOT format.
             * @param vertex   Pointer to the vertex.
             */
            void vertexPrinter(const Vertex *vertex) const;

            /**
             * @brief Prints a graph interface into DOT format.
             * @param interface Pointer to the interface.
             * @param color     Color of the interface (red for output or green for input).
             */
            void interfaceBodyPrinter(const Interface *interface, const std::string &color) const;

            /**
             * @brief Prints an edge.
             * @param edge Pointer to the edge.
             */
            void edgePrinter(const Edge *edge) const;

            /**
             * @brief Prints a param.
             * @param param  Pointer to the parameter.
             */
            void paramPrinter(const Param *param) const;

            /**
             * @brief Prints a data port.
             * @param edge      Pointer to the edge.
             * @param width     Width of the port.
             * @param color     Color of the port (green for input, red for output)
             * @param direction direction of the data port (true for input, false for output)
             */
            void
            portPrinter(const Edge *edge, int_fast32_t width, const std::string &color, bool direction = true) const;

            /**
             * @brief Prints a dummy data port.
             * @param width      Width of the port.
             * @param color      Color of the vertex containing the port
             * @param direction  Direction, true for input, false for output
             */
            void dummyPortPrinter(int_fast32_t width, const std::string &color, bool direction = true) const;

            /**
             * @brief Prints the header of a data port.
             */
            void portHeaderPrinter() const;

            /**
             * @brief Prints the footer of a data port.
             */
            void portFooterPrinter() const;
        };
    }
}
#endif //SPIDER2_PISDFDOTEXPORTERVISITOR_H
