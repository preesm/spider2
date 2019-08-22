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

/* === Includes === */

#include <graphs/pisdf/PiSDFGraph.h>
#include "PiSDFInterface.h"

/* === Static function(s) === */

static const char *getBGColor(PiSDFInterfaceType type) {
    switch (type) {
        case PiSDFInterfaceType::INPUT:
            return "#fff68f";
        case PiSDFInterfaceType::OUTPUT:
            return "#dcc6e0";
        default:
            return "eeeeee";
    }
}


/* === Methods implementation === */

PiSDFInterface::PiSDFInterface(PiSDFGraph *graph,
                               std::string name,
                               PiSDFInterfaceType type) : PiSDFVertex(graph,
                                                                      std::move(name),
                                                                      PiSDFVertexType::INTERFACE,
                                                                      type == PiSDFInterfaceType::OUTPUT,
                                                                      type == PiSDFInterfaceType::INPUT,
                                                                      0,
                                                                      0) {
    interfaceType_ = type;
    graph->addInterface(this);
}

std::uint16_t PiSDFInterface::correspondingPortIx() const {
    return ix();
}

void PiSDFInterface::exportDOT(FILE *file, const std::string &offset) const {
    Spider::cxx11::fprintf(file,
                           "%s\"%s\" [shape=plain, style=filled, fillcolor=\"%sff\", width=0, height=0, label = <\n",
                           offset.c_str(), name().c_str(), getBGColor(interfaceType_));
    Spider::cxx11::fprintf(file, "%s\t<table border=\"0\" fixedsize=\"false\" cellspacing=\"0\" cellpadding=\"0\">\n",
                           offset.c_str());

    /* == Vertex name == */
    Spider::cxx11::fprintf(file,
                           "%s\t\t<tr> <td border=\"1\" sides=\"lrt\" colspan=\"4\" fixedsize=\"false\" height=\"10\"></td></tr>\n",
                           offset.c_str());
    Spider::cxx11::fprintf(file,
                           "%s\t\t<tr> <td border=\"1\" sides=\"lr\" colspan=\"4\"><font point-size=\"25\" face=\"inconsolata\">%s</font></td></tr>\n",
                           offset.c_str(), name().c_str());

    /* == Compute widths == */
    auto n = name().size();
    auto centerWidth = static_cast<std::uint32_t>(15. * (n - 8.) * (n > 8) +
                                                  std::ceil(20. * (1 + 1. / (1 + std::exp(-10. * (n - 7.))))));
    double longestRateLen = std::max(0., std::log10(inputEdge()->sinkRate()));
    longestRateLen = std::max(longestRateLen, std::log10(outputEdge()->sourceRate()));
    auto rateWidth = 32 + std::max(static_cast<std::int32_t>(longestRateLen) + 1 - 3, 0) * 8;

    /* == Export data ports == */
    Spider::cxx11::fprintf(file,
                           "%s\t\t<tr> <td border=\"1\" sides=\"lr\" colspan=\"4\" fixedsize=\"false\" height=\"10\"></td></tr>\n",
                           offset.c_str());
    Spider::cxx11::fprintf(file, "%s\t\t<tr>\n", offset.c_str());

    /* == Export input port == */
    exportInputPortDOT(file, offset, rateWidth, inputEdge());

    /* == Middle separation == */
    Spider::cxx11::fprintf(file,
                           "%s\t\t\t<td border=\"0\" colspan=\"2\" bgcolor=\"#00000000\" fixedsize=\"true\" width=\"%" PRIu32"\" height=\"20\"></td>\n",
                           offset.c_str(), centerWidth);

    /* == Export output port == */
    exportOutputPortDOT(file, offset, rateWidth, outputEdge());

    Spider::cxx11::fprintf(file, "%s\t\t</tr>\n", offset.c_str());

    /* == Footer == */
    Spider::cxx11::fprintf(file,
                           "%s\t\t<tr> <td border=\"1\" colspan=\"4\" fixedsize=\"false\" height=\"10\" sides=\"lbr\"></td></tr>\n",
                           offset.c_str());
    Spider::cxx11::fprintf(file, "%s\t</table>>\n", offset.c_str());
    Spider::cxx11::fprintf(file, "%s];\n\n", offset.c_str());
}

const PiSDFEdge *PiSDFInterface::inputEdge() const {
    if (interfaceType_ == PiSDFInterfaceType::INPUT) {
        const auto *graph = containingGraph();
        return graph->inputEdge(this->ix());
    }
    return this->inputEdges()[0];
}

const PiSDFEdge *PiSDFInterface::outputEdge() const {
    if (interfaceType_ == PiSDFInterfaceType::OUTPUT) {
        const auto *graph = containingGraph();
        return graph->outputEdge(this->ix());
    }
    return this->outputEdges()[0];
}
