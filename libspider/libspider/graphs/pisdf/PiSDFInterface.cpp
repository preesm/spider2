/*
 * Copyright or © or Copr. IETR/INSA - Rennes (2013 - 2018) :
 *
 * Antoine Morvan <antoine.morvan@insa-rennes.fr> (2018)
 * Clément Guy <clement.guy@insa-rennes.fr> (2014)
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2018)
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

/* === Methods implementation === */

PiSDFInterface::PiSDFInterface(PiSDFGraph *graph,
                               PiSDFSubType type,
                               std::uint32_t ix,
                               PiSDFEdge *inputEdge,
                               PiSDFEdge *outputEdge,
                               std::string name) : graph_{graph},
                                                   type_{type},
                                                   ix_{ix},
                                                   inputEdge_{inputEdge},
                                                   outputEdge_{outputEdge},
                                                   name_{std::move(name)} {

    graph->addInterface(this);
}

static const char * getBGColor(PiSDFSubType type) {
    switch (type) {
        case PiSDFSubType::INPUT:
            return "c8f7c5";
        case PiSDFSubType::OUTPUT:
            return "f1a9a0";
        default:
            return "eeeeee";
    }
}

void PiSDFInterface::exportDot(FILE *file, const Spider::string &offset) const {
    fprintf(file, "%s\"%s\" [ shape = none, margin = 0, label = <\n", offset.c_str(), name_.c_str());
    fprintf(file, "%s\t<table border = \"1\" cellspacing=\"0\" cellpadding = \"0\" bgcolor = \"#%s\">\n",
            offset.c_str(), getBGColor(type_));

    /* == Header == */
    fprintf(file, "%s\t\t<tr> <td colspan=\"3\" border=\"0\"><font point-size=\"5\"> </font></td></tr>\n",
            offset.c_str());

    /* == Vertex name == */
    fprintf(file, "%s\t\t<tr> <td colspan=\"3\" border=\"0\"><font point-size=\"35\">%s</font></td></tr>\n",
            offset.c_str(), name_.c_str());

    /* == Input ports == */
    fprintf(file, "%s\t\t<tr>\n", offset.c_str());
    if (inputEdge_) {
        fprintf(file, "%s\t\t\t<td border=\"0\">\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t<table border=\"0\" cellpadding=\"0\" cellspacing=\"1\">\n", offset.c_str());
        /* == Print the output edge port information == */
        fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t\t\t<td port=\"in_0\" border=\"1\" bgcolor=\"#87d37c\">    </td>\n", offset.c_str());
        fprintf(file,
                "%s\t\t\t\t\t\t<td align=\"right\" border=\"0\" bgcolor=\"#%s\"><font point-size=\"15\">in</font></td>\n",
                offset.c_str(), getBGColor(type_));
        fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t</table>\n", offset.c_str());
        fprintf(file, "%s\t\t\t</td>\n", offset.c_str());
    } else {
        /* == Print the dummy port for pretty spacing == */
        fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t\t\t<td border=\"0\" bgcolor=\"#%s\">    </td>\n", offset.c_str(), getBGColor(type_));
        fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
    }

    /* == Center column == */
    fprintf(file, "%s\t\t\t<td border=\"0\" colspan=\"1\" cellpadding=\"10\"> </td>\n", offset.c_str());

    /* == Output ports == */
    if (outputEdge_) {
        fprintf(file, "%s\t\t\t<td border=\"0\">\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t<table border=\"0\" cellpadding=\"0\" cellspacing=\"1\">\n", offset.c_str());
        /* == Print the output edge port information == */
        fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        fprintf(file,
                "%s\t\t\t\t\t\t<td align=\"right\" border=\"0\" bgcolor=\"#%s\"><font point-size=\"15\">out</font></td>\n",
                offset.c_str(), getBGColor(type_));
        fprintf(file, "%s\t\t\t\t\t\t<td port=\"out_0\" border=\"1\" bgcolor=\"#ec644b\">    </td>\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t</table>\n", offset.c_str());
        fprintf(file, "%s\t\t\t</td>\n", offset.c_str());
    } else {
        /* == Print the dummy port for pretty spacing == */
        fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t\t\t<td border=\"0\" bgcolor=\"#%s\">    </td>\n", offset.c_str(), getBGColor(type_));
        fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
    }
    fprintf(file, "%s\t\t</tr>\n", offset.c_str());

    /* == Footer == */
    fprintf(file, "%s\t\t<tr> <td colspan=\"3\" border=\"0\"><font point-size=\"5\"> </font></td></tr>\n",
            offset.c_str());
    fprintf(file, "%s\t</table>>\n", offset.c_str());
    fprintf(file, "%s];\n\n", offset.c_str());
}
