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
    Spider::cxx11::fprintf(file, "%s\"%s\" [ shape = none, margin = 0, label = <\n", offset.c_str(), name().c_str());
    Spider::cxx11::fprintf(file, "%s\t<table border = \"1\" cellspacing=\"0\" cellpadding = \"0\" bgcolor = \"%s\">\n",
                           offset.c_str(), getBGColor(interfaceType_));

    /* == Header == */
    Spider::cxx11::fprintf(file,
                           "%s\t\t<tr> <td colspan=\"3\" border=\"0\"><font point-size=\"5\"> </font></td></tr>\n",
                           offset.c_str());

    /* == Vertex name == */
    Spider::cxx11::fprintf(file,
                           "%s\t\t<tr> <td colspan=\"3\" border=\"0\"><font point-size=\"35\">%s</font></td></tr>\n",
                           offset.c_str(), name().c_str());

    /* == Input ports == */
    Spider::cxx11::fprintf(file, "%s\t\t<tr>\n", offset.c_str());
    if (inputEdge()) {
        Spider::cxx11::fprintf(file, "%s\t\t\t<td border=\"0\">\n", offset.c_str());
        Spider::cxx11::fprintf(file, "%s\t\t\t\t<table border=\"0\" cellpadding=\"0\" cellspacing=\"1\">\n",
                               offset.c_str());
        /* == Print the output edge port information == */
        Spider::cxx11::fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        Spider::cxx11::fprintf(file, "%s\t\t\t\t\t\t<td port=\"in_0\" border=\"1\" bgcolor=\"#87d37c\">    </td>\n",
                               offset.c_str());
        Spider::cxx11::fprintf(file,
                               "%s\t\t\t\t\t\t<td align=\"right\" border=\"0\" bgcolor=\"%s\"><font point-size=\"15\">in</font></td>\n",
                               offset.c_str(), getBGColor(interfaceType_));
        Spider::cxx11::fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
        Spider::cxx11::fprintf(file, "%s\t\t\t\t</table>\n", offset.c_str());
        Spider::cxx11::fprintf(file, "%s\t\t\t</td>\n", offset.c_str());
    } else {
        /* == Print the dummy port for pretty spacing == */
        Spider::cxx11::fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        Spider::cxx11::fprintf(file, "%s\t\t\t\t\t\t<td border=\"0\" bgcolor=\"%s\">    </td>\n", offset.c_str(),
                               getBGColor(interfaceType_));
        Spider::cxx11::fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
    }

    /* == Center column == */
    Spider::cxx11::fprintf(file, "%s\t\t\t<td border=\"0\" colspan=\"1\" cellpadding=\"10\"> </td>\n", offset.c_str());

    /* == Output ports == */
    if (outputEdge()) {
        Spider::cxx11::fprintf(file, "%s\t\t\t<td border=\"0\">\n", offset.c_str());
        Spider::cxx11::fprintf(file, "%s\t\t\t\t<table border=\"0\" cellpadding=\"0\" cellspacing=\"1\">\n",
                               offset.c_str());
        /* == Print the output edge port information == */
        Spider::cxx11::fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        Spider::cxx11::fprintf(file,
                               "%s\t\t\t\t\t\t<td align=\"right\" border=\"0\" bgcolor=\"%s\"><font point-size=\"15\">out</font></td>\n",
                               offset.c_str(), getBGColor(interfaceType_));
        Spider::cxx11::fprintf(file, "%s\t\t\t\t\t\t<td port=\"out_0\" border=\"1\" bgcolor=\"#ec644b\">    </td>\n",
                               offset.c_str());
        Spider::cxx11::fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
        Spider::cxx11::fprintf(file, "%s\t\t\t\t</table>\n", offset.c_str());
        Spider::cxx11::fprintf(file, "%s\t\t\t</td>\n", offset.c_str());
    } else {
        /* == Print the dummy port for pretty spacing == */
        Spider::cxx11::fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        Spider::cxx11::fprintf(file, "%s\t\t\t\t\t\t<td border=\"0\" bgcolor=\"%s\">    </td>\n", offset.c_str(),
                               getBGColor(interfaceType_));
        Spider::cxx11::fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
    }
    Spider::cxx11::fprintf(file, "%s\t\t</tr>\n", offset.c_str());

    /* == Footer == */
    Spider::cxx11::fprintf(file,
                           "%s\t\t<tr> <td colspan=\"3\" border=\"0\"><font point-size=\"5\"> </font></td></tr>\n",
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
