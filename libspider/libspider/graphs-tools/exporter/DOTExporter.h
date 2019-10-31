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
#ifndef SPIDER2_DOTEXPORTER_H
#define SPIDER2_DOTEXPORTER_H

/* === Include(s) === */

#include <common/Exporter.h>
#include <graphs/pisdf/Types.h>

namespace Spider {
    namespace PiSDF {

        /* === Class definition === */

        class DOTExporter final : public Exporter {
        public:

            explicit DOTExporter(const Graph *graph) : Exporter(), graph_{graph} { }

            ~DOTExporter() override = default;

            /* === Method(s) === */

            /**
             * @brief Print the graph to default file path.
             * @remark default path: ./pisdf-graph.dot
             */
            void print() const override;

            void print(const std::string &path) const override;

            void print(std::ofstream &file) const override;

        private:
            const Graph *graph_ = nullptr;

            /* === Private static method(s) === */

            static void graphPrinter(std::ofstream &file, const Graph *graph, const std::string &offset = "\t");

            static void vertexPrinter(std::ofstream &file, const Vertex *vertex, const std::string &offset);

            static void edgePrinter(std::ofstream &file, const Edge *edge, const std::string &offset);

            static void paramPrinter(std::ofstream &file, const Param *param, const std::string &offset);

            static void inputIFPrinter(std::ofstream &file,
                                       const Interface *interface,
                                       const std::string &offset);

            static void outputIFPrinter(std::ofstream &file,
                                        const Interface *interface,
                                        const std::string &offset);

            static void interfacePrinter(std::ofstream &file,
                                         const Interface *interface,
                                         const std::string &offset);


            static void dataPortPrinter(std::ofstream &file,
                                        const Edge *edge,
                                        const std::string &offset,
                                        std::uint32_t width,
                                        bool input);

            static void inputDataPortPrinter(std::ofstream &file,
                                             const Edge *edge,
                                             const std::string &offset,
                                             std::uint32_t width);

            static void outputDataPortPrinter(std::ofstream &file,
                                              const Edge *edge,
                                              const std::string &offset,
                                              std::uint32_t width);

            static void dummyDataPortPrinter(std::ofstream &file,
                                             const std::string &offset,
                                             std::uint32_t width,
                                             bool input);
        };
    }
}

#endif //SPIDER2_DOTEXPORTER_H
