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

#include <graphs/pisdf/PiSDFVertex.h>
#include <graphs/pisdf/PiSDFGraph.h>
#include <graphs/pisdf/PiSDFEdge.h>
#include <common/memory/Allocator.h>

/* === Static methods === */

static const char *getVertexDotColor(PiSDFSubType subType) {
    switch (subType) {
        case PiSDFSubType::NORMAL:
            return "eeeeee";
        case PiSDFSubType::FORK:
            return "fabe58";
        case PiSDFSubType::JOIN:
            return "aea8d3";
        case PiSDFSubType::BROADCAST:
            return "fdeba7";
        case PiSDFSubType::ROUNDBUFFER:
            return "f1e7fe";
        case PiSDFSubType::INPUT:
            return "87d37c";
        case PiSDFSubType::OUTPUT:
            return "ec644b";
        case PiSDFSubType::INIT:
            return "e4f1fe";
        case PiSDFSubType::END:
            return "e8ecf1";
        default:
            return "eeeeee";
    }
}

/* === Methods implementation === */

PiSDFVertex::PiSDFVertex(PiSDFGraph *graph,
                         PiSDFType type,
                         PiSDFSubType subType,
                         std::uint32_t nEdgesIN,
                         std::uint32_t nEdgesOUT,
                         std::string name) : SetElement(),
                                             graph_{graph},
                                             name_{std::move(name)},
                                             type_{type},
                                             subType_{subType},
                                             nEdgesIN_{nEdgesIN},
                                             nEdgesOUT_{nEdgesOUT},
                                             inputEdgeArray_(StackID::PISDF, nEdgesIN, nullptr),
                                             outputEdgeArray_(StackID::PISDF, nEdgesOUT, nullptr) {
    if (!graph) {
        throwSpiderException("Vertex should belong to a graph.");
    }
    checkSubtypeConsistency();

    if (type == PiSDFType::CONFIG_VERTEX) {
        /* == Configuration actors have a fixed repetition vector value of 1 == */
        repetitionValue_ = 1;
    }

    graph->addVertex(this);
}

void PiSDFVertex::exportDot(FILE *file, const Spider::string &offset) const {
    fprintf(file, "%s\"%s\" [ shape = none, margin = 0, label = <\n", offset.c_str(), name_.c_str());
    fprintf(file, "%s\t<table border = \"1\" cellspacing=\"0\" cellpadding = \"0\" bgcolor = \"#%s\">\n",
            offset.c_str(), getVertexDotColor(subType_));

    /* == Header == */
    fprintf(file, "%s\t\t<tr> <td colspan=\"4\" border=\"0\"><font point-size=\"5\"> </font></td></tr>\n",
            offset.c_str());

    /* == Vertex name == */
    fprintf(file, "%s\t\t<tr> <td colspan=\"4\" border=\"0\"><font point-size=\"35\">%s</font></td></tr>\n",
            offset.c_str(), name_.c_str());

    /* == Input ports == */
    fprintf(file, "%s\t\t<tr>\n", offset.c_str());
    exportInputPortsToDot(file, offset);

    /* == Center column == */
    fprintf(file, "%s\t\t\t<td border=\"0\" colspan=\"2\" cellpadding=\"10\"> </td>\n", offset.c_str());

    /* == Output ports == */
    exportOutputPortsToDot(file, offset);
    fprintf(file, "%s\t\t</tr>\n", offset.c_str());

    /* == Footer == */
    fprintf(file, "%s\t\t<tr> <td colspan=\"4\" border=\"0\"><font point-size=\"5\"> </font></td></tr>\n",
            offset.c_str());
    fprintf(file, "%s\t</table>>\n", offset.c_str());
    fprintf(file, "%s];\n\n", offset.c_str());
}

/* === Private method(s) === */

void PiSDFVertex::exportInputPortsToDot(FILE *file,
                                        const Spider::string &offset) const {
    fprintf(file, "%s\t\t\t<td border=\"0\">\n", offset.c_str());
    fprintf(file, "%s\t\t\t\t<table border=\"0\" cellpadding=\"0\" cellspacing=\"1\">\n", offset.c_str());
    for (const auto &e: inputEdgeArray_) {
        /* == Print the input port associated to the edge == */
        fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t\t\t<td port=\"in_%" PRIu32"\" border=\"1\" bgcolor=\"#87d37c\">    </td>\n",
                offset.c_str(), e->sinkPortIx());
        fprintf(file,
                "%s\t\t\t\t\t\t<td align=\"right\" border=\"0\" bgcolor=\"#%s\"><font point-size=\"15\">width</font></td>\n",
                offset.c_str(),
                getVertexDotColor(subType_));
        fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());

        /* == Print the dummy port for pretty spacing == */
        fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t\t\t<td border=\"0\" bgcolor=\"#%s\">    </td>\n",
                offset.c_str(),
                getVertexDotColor(subType_));
        fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
    }
    if (!nEdgesIN_) {
        /* == Print the dummy port for pretty spacing == */
        fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t\t\t<td border=\"0\" bgcolor=\"#%s\">    </td>\n",
                offset.c_str(),
                getVertexDotColor(subType_));
        fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
    }

    /* == Print dummy extra input ports to match with output ports (if needed) == */
    for (auto i = nEdgesIN_; i < nEdgesOUT_; ++i) {
        fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t\t\t<td border=\"0\" bgcolor=\"#%s\">    </td>\n",
                offset.c_str(),
                getVertexDotColor(subType_));
        fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
    }
    fprintf(file, "%s\t\t\t\t</table>\n", offset.c_str());
    fprintf(file, "%s\t\t\t</td>\n", offset.c_str());
}

void PiSDFVertex::exportOutputPortsToDot(FILE *file,
                                         const Spider::string &offset) const {
    fprintf(file, "%s\t\t\t<td border=\"0\">\n", offset.c_str());
    fprintf(file, "%s\t\t\t\t<table border=\"0\" cellpadding=\"0\" cellspacing=\"1\">\n", offset.c_str());
    for (const auto &e:outputEdgeArray_) {
        /* == Print the output edge port information == */
        fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        fprintf(file,
                "%s\t\t\t\t\t\t<td align=\"right\" border=\"0\" bgcolor=\"#%s\"><font point-size=\"15\">width</font></td>\n",
                offset.c_str(),
                getVertexDotColor(subType_));
        fprintf(file, "%s\t\t\t\t\t\t<td port=\"out_%" PRIu32"\" border=\"1\" bgcolor=\"#ec644b\">    </td>\n",
                offset.c_str(), e->sourcePortIx());
        fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());

        /* == Print the dummy port for pretty spacing == */
        fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t\t\t<td border=\"0\" bgcolor=\"#%s\">    </td>\n", offset.c_str(),
                getVertexDotColor(subType_));
        fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
    }
    if (!nEdgesOUT_) {
        /* == Print the dummy port for pretty spacing == */
        fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t\t\t<td border=\"0\" bgcolor=\"#%s\">    </td>\n",
                offset.c_str(),
                getVertexDotColor(subType_));
        fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
    }

    /* == Print dummy extra input ports to match with output ports (if needed) == */
    for (auto i = nEdgesOUT_; i < nEdgesIN_; ++i) {
        fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
        fprintf(file, "%s\t\t\t\t\t\t<td border=\"0\" bgcolor=\"#%s\">    </td>\n",
                offset.c_str(),
                getVertexDotColor(subType_));
        fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
    }
    fprintf(file, "%s\t\t\t\t</table>\n", offset.c_str());
    fprintf(file, "%s\t\t\t</td>\n", offset.c_str());
}

void PiSDFVertex::checkSubtypeConsistency() const {
    if ((subType_ == PiSDFSubType::INPUT || subType_ == PiSDFSubType::OUTPUT)) {
        throwSpiderException("Vertex can not have subtype INPUT nor OUTPUT.");
    }

    if (nEdgesIN_ > 1 && (subType_ == PiSDFSubType::FORK ||
                          subType_ == PiSDFSubType::BROADCAST)) {
        throwSpiderException("Fork and Broadcast actors can only have one input edge.");
    } else if (nEdgesIN_ && subType_ == PiSDFSubType::INIT) {
        throwSpiderException("Init actors can not have input edge !");
    }
    if (nEdgesOUT_ > 1 && (subType_ == PiSDFSubType::JOIN ||
                           subType_ == PiSDFSubType::ROUNDBUFFER)) {
        throwSpiderException("Join and Roundbuffer actors can only have one input edge.");
    } else if (nEdgesOUT_ && subType_ == PiSDFSubType::END) {
        throwSpiderException("End actors can not have output edge !");
    }
}