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

/* === Methods implementation === */

PiSDFGraph::PiSDFGraph(std::string name,
                       std::uint64_t nActors,
                       std::uint64_t nEdges,
                       std::uint64_t nParams,
                       std::uint64_t nInputInterfaces,
                       std::uint64_t nOutputInterfaces,
                       std::uint64_t nConfigActors) : name_{std::move(name)},
                                                      vertexSet_{StackID::PISDF, nActors},
                                                      edgeSet_{StackID::PISDF, nEdges},
                                                      paramSet_{StackID::PISDF, nParams},
                                                      inputInterfaceSet_{StackID::PISDF, nInputInterfaces},
                                                      outputInterfaceSet_{StackID::PISDF, nOutputInterfaces},
                                                      configSet_{StackID::PISDF, nConfigActors} {

}

PiSDFGraph::PiSDFGraph(std::string name,
                       PiSDFGraph *parent,
                       std::uint64_t nActors,
                       std::uint64_t nEdges,
                       std::uint64_t nParams,
                       std::uint64_t nInputInterfaces,
                       std::uint64_t nOutputInterfaces,
                       std::uint64_t nConfigActors) : name_{std::move(name)},
                                                      vertexSet_{StackID::PISDF, nActors},
                                                      edgeSet_{StackID::PISDF, nEdges},
                                                      paramSet_{StackID::PISDF, nParams},
                                                      inputInterfaceSet_{StackID::PISDF, nInputInterfaces},
                                                      outputInterfaceSet_{StackID::PISDF, nOutputInterfaces},
                                                      configSet_{StackID::PISDF, nConfigActors},
                                                      parentGraph_{parent} {
    if (!parent) {
        throwSpiderException("Can not create subgraph with null parent.");
    }
    parent->addSubgraph(this);
}

PiSDFGraph::~PiSDFGraph() {
    for (auto &v : vertexSet_) {
        Spider::destroy(v);
        Spider::deallocate(v);
    }

    for (auto &sg: subgraphList_) {
        Spider::destroy(sg);
        Spider::deallocate(sg);
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
    vertexSet_.remove(vertex);
    Spider::destroy(vertex);
    Spider::deallocate(vertex);
}

void PiSDFGraph::removeSubgraph(PiSDFGraph *subgraph) {
    if (!subgraph) {
        return;
    }
    if (!subgraphs().contains(subgraph)) {
        throwSpiderException("Trying to remove a vertex [%s] that don't belong to this graph.",
                             subgraph->name().c_str());
    }
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

void PiSDFGraph::addSubgraph(PiSDFGraph *subgraph) {
    if (!subgraph) {
        throwSpiderException("Can not add nullptr subgraph.");
    }
    subgraphList_.addTail(subgraph);
    static_ &= subgraph->static_;
}

void PiSDFGraph::exportDot(const std::string &path) const {
    auto *file = std::fopen(path.c_str(), "w+");

    exportDotHelper(file, "\t");

    std::fclose(file);
}

void PiSDFGraph::exportDot(FILE *file, const std::string &offset) const {
    exportDotHelper(file, offset);
}

/* === Private method(s) === */

void PiSDFGraph::exportDotHelper(FILE *file, const std::string &offset) const {
    auto fwOffset{offset};
    if (parentGraph_) {
        fprintf(file, "%ssubgraph cluster {\n", fwOffset.c_str());
        fwOffset += "\t";
        fprintf(file, "%slabel=\"%s\";\n", fwOffset.c_str(), name_.c_str());
        fprintf(file, "%sstyle=dotted;\n", fwOffset.c_str());
        fprintf(file, "%sfillcolor=\"#ffffff\";\n", fwOffset.c_str());
        fprintf(file, "%scolor=\"#393c3c\";\n", fwOffset.c_str());
        fprintf(file, "%spenwidth=2;\n", fwOffset.c_str());
    } else {
        fprintf(file, "digraph {\n");
        fprintf(file, "\tlabel=topgraph;\n");
        fprintf(file, "\trankdir=LR;\n");
        fprintf(file, "\tranksep=\"2\";\n");
    }

    fprintf(file, "\n%s// Vertices\n", fwOffset.c_str());
    for (const auto &v:vertexSet_) {
        v->exportDot(file, fwOffset);
    }

    if (parentGraph_) {
        fprintf(file, "\n%s// Interfaces\n", fwOffset.c_str());
        for (const auto &i:inputInterfaceSet_) {
            i->exportDot(file, fwOffset);
        }
        for (const auto &o:outputInterfaceSet_) {
            o->exportDot(file, fwOffset);
        }
    }

    if (paramSet_.occupied()) {
        fprintf(file, "\n%s// Parameters\n", fwOffset.c_str());
        for (const auto &p:paramSet_) {
            p->exportDot(file, fwOffset);
        }
    }

    fprintf(file, "\n%s// Subgraphs\n", fwOffset.c_str());
    for (const auto &subgraph:subgraphList_) {
        subgraph->exportDot(file, fwOffset);
    }

    fprintf(file, "\n%s// Vertex edges\n", fwOffset.c_str());
    for (const auto &e:edgeSet_) {
        e->exportDot(file, fwOffset);
    }

    fprintf(file, "%s}\n", parentGraph_ ? offset.c_str() : "");
}
