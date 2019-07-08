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

#include <fstream>
#include <iostream>
#include "PiSDFGraph.h"

/* === Static function(s) === */

static void exportSubGraph(FILE *file, PiSDFGraph *graph) {
    fprintf(file, "\tsubgraph {\n");
}

static void exportVertex(FILE *file, PiSDFVertex *vertex, const Spider::string &offset) {
    auto maxRows = std::max(vertex->nEdgesIN(), vertex->nEdgesOUT());
    maxRows = std::max(maxRows, 1u);   /* = Ensure at least one dummy port = */
    maxRows = maxRows + (maxRows - 1); /* = Add the dummy ports = */

    fprintf(file, "%s%s [ shape = none, margin = 0, label = <\n", offset.c_str(), vertex->name().c_str());
    fprintf(file, "%s\t<table border = \"1\" cellspacing=\"0\" cellpadding = \"0\" bgcolor = \"#eeeeee\">\n",
            offset.c_str());

    /* == Header == */
    fprintf(file, "%s\t\t<tr> <td colspan=\"4\" border=\"0\"><font point-size=\"5\"> </font></td></tr>\n",
            offset.c_str());

    /* == Vertex name == */
    fprintf(file, "%s\t\t<tr> <td colspan=\"4\" border=\"0\"><font point-size=\"35\">%s</font></td></tr>\n",
            offset.c_str(), vertex->name().c_str());

    /* == Ports == */
    fprintf(file, "%s\t\t<tr>\n", offset.c_str());

    /* == Input ports == */
    fprintf(file, "%s\t\t\t<td border=\"0\">\n", offset.c_str());
    fprintf(file, "%s\t\t\t\t<table border=\"0\" cellpadding=\"0\" cellspacing=\"1\">\n", offset.c_str());
    {
        auto *it = vertex->inputEdges().begin();
        for (std::uint32_t i = 0; i < maxRows; ++i) {
            fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
            if (i >= vertex->nEdgesIN() || (i % 2)) {
                fprintf(file, "%s\t\t\t\t\t\t<td border=\"0\" bgcolor=\"#eeeeee\">    </td>\n", offset.c_str());
            } else {
                fprintf(file, "%s\t\t\t\t\t\t<td port=\"in_%" PRIu32"\" border=\"1\" bgcolor=\"#87d37c\">    </td>\n",
                        offset.c_str(), (*(it)) ? (*(it))->sinkPortIx() : 0);
                fprintf(file,
                        "%s\t\t\t\t\t\t<td align=\"right\" border=\"0\" bgcolor=\"#eeeeee\"><font point-size=\"15\">width</font></td>\n",
                        offset.c_str());
                it++;
            }
            fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
        }
    }
    fprintf(file, "%s\t\t\t\t</table>\n", offset.c_str());
    fprintf(file, "%s\t\t\t</td>\n", offset.c_str());

    /* == Center column == */
    fprintf(file, "%s\t\t\t<td border=\"0\" colspan=\"2\" cellpadding=\"10\"> </td>\n", offset.c_str());

    /* == Output ports == */
    fprintf(file, "%s\t\t\t<td border=\"0\">\n", offset.c_str());
    fprintf(file, "%s\t\t\t\t<table border=\"0\" cellpadding=\"0\" cellspacing=\"1\">\n", offset.c_str());
    {
        auto *it = vertex->outputEdges().begin();
        for (std::uint32_t i = 0; i < maxRows; ++i) {
            fprintf(file, "%s\t\t\t\t\t<tr>\n", offset.c_str());
            if (i >= vertex->nEdgesOUT() || (i % 2)) {
                fprintf(file, "%s\t\t\t\t\t\t<td border=\"0\" bgcolor=\"#eeeeee\">    </td>\n", offset.c_str());
            } else {
                fprintf(file,
                        "%s\t\t\t\t\t\t<td align=\"right\" border=\"0\" bgcolor=\"#eeeeee\"><font point-size=\"15\">width</font></td>\n",
                        offset.c_str());
                fprintf(file, "%s\t\t\t\t\t\t<td port=\"out_%" PRIu32"\" border=\"1\" bgcolor=\"#ec644b\">    </td>\n",
                        offset.c_str(), (*(it)) ? (*(it))->sourcePortIx() : 0);
                it++;
            }
            fprintf(file, "%s\t\t\t\t\t</tr>\n", offset.c_str());
        }
    }
    fprintf(file, "%s\t\t\t\t</table>\n", offset.c_str());
    fprintf(file, "%s\t\t\t</td>\n", offset.c_str());
    fprintf(file, "%s\t\t</tr>\n", offset.c_str());

    /* == Footer == */
    fprintf(file, "%s\t\t<tr> <td colspan=\"4\" border=\"0\"><font point-size=\"5\"> </font></td></tr>\n",
            offset.c_str());
    fprintf(file, "%s\t</table>>\n", offset.c_str());
    fprintf(file, "%s];\n\n", offset.c_str());
}

static void exportEdge(FILE *file, PiSDFEdge *edge) {
    fprintf(file,
            "\t%s:out_%" PRIu32":e -> %s:in_%" PRIu32":w [penwidth=2, color=\"#393c3c\", dir=forward, headlabel=\"%" PRIu64"   \", taillabel=\" %" PRIu64"\"];\n",
            edge->source()->name().c_str(),
            edge->sourcePortIx(),
            edge->sink()->name().c_str(),
            edge->sinkPortIx(),
            edge->sinkRate(),
            edge->sourceRate());
}

/* === Methods implementation === */

PiSDFGraph::PiSDFGraph(std::uint64_t nActors,
                       std::uint64_t nEdges,
                       std::uint64_t nParams,
                       std::uint64_t nInputInterfaces,
                       std::uint64_t nOutputInterfaces,
                       std::uint64_t nConfigActors) : vertexSet_{StackID::GENERAL, nActors},
                                                      edgeSet_{StackID::GENERAL, nEdges},
                                                      paramSet_{StackID::GENERAL, nParams},
                                                      inputInterfaceSet_{StackID::GENERAL, nInputInterfaces},
                                                      outputInterfaceSet_{StackID::GENERAL, nOutputInterfaces},
                                                      configSet_{StackID::GENERAL, nConfigActors} {

}

PiSDFGraph::~PiSDFGraph() {
    for (auto &v : vertexSet_) {
        Spider::destroy(v);
        Spider::deallocate(v);
    }

    for (auto &e:edgeSet_) {
        Spider::destroy(e);
        Spider::deallocate(e);
    }

    for (auto &p:paramSet_) {
        Spider::destroy(p);
        Spider::deallocate(p);
    }

    for (auto &inIf:inputInterfaceSet_) {
        Spider::destroy(inIf);
        Spider::deallocate(inIf);
    }

    for (auto &outIf:outputInterfaceSet_) {
        Spider::destroy(outIf);
        Spider::deallocate(outIf);
    }

    for (auto &c:configSet_) {
        Spider::destroy(c);
        Spider::deallocate(c);
    }
}

void PiSDFGraph::removeVertex(PiSDFVertex *vertex) {
    if (!vertex) {
        return;
    }
    if (!vertexSet_.contains(vertex)) {
        throwSpiderException("Trying to remove a vertex [%s] that don't belong to this graph.", vertex->name().c_str());
    }
    if (vertex->isHierarchical()) {
        auto *subgraph = vertex->subgraph();
        auto wasStatic = subgraph->isStatic();
        subgraphList_.removeFromValue(subgraph);
        Spider::destroy(subgraph);
        Spider::deallocate(subgraph);

        /* == Recompute the static property == */
        if (wasStatic) {
            static_ = hasDynamicParameters_;
            if (static_) {
                for (auto &g:subgraphList_) {
                    static_ &= g->isStatic();
                }
            }
        }
    }
    vertexSet_.remove(vertex);
    Spider::destroy(vertex);
    Spider::deallocate(vertex);
}

void PiSDFGraph::addSubGraph(PiSDFVertex *vertex) {
    if (!vertex->isHierarchical()) {
        throwSpiderException("Can not add subgraph from non-hierarchical vertex.");
    }
    auto *subgraph = vertex->subgraph();
    subgraphList_.addTail(subgraph);
    static_ &= subgraph->static_;
}

void PiSDFGraph::exportGraph(const std::string &path) const {
    auto *file = std::fopen(path.c_str(), "w+");

    fprintf(file, "digraph {\n");
    fprintf(file, "\tlabel=%s;\n", parentVertex_ != nullptr ? parentVertex_->name().c_str() : "topgraph");
    fprintf(file, "\trankdir=LR;\n");
    fprintf(file, "\tranksep=\"2\";\n");
    fprintf(file, "\t// Vertices\n");

    Spider::string offset{"\t"};
    for (auto &v:vertexSet_) {
//        if (v->isHierarchical()) {
//            exportSubGraph(file, v->subgraph());
//        }
        exportVertex(file, v, offset);
    }

    fprintf(file, "\t// Edges\n");
    for (auto &e:edgeSet_) {
        exportEdge(file, e);
    }
    fprintf(file, "}\n");

    std::fclose(file);
}
