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
                       std::uint64_t nConfigActors) : name_{std::move(name)} {
    vertexVector_.reserve(nActors);
    edgeVector_.reserve(nEdges);
    paramVector_.reserve(nParams);
    configVertexVector_.reserve(nConfigActors);
    inputInterfaceVector_.reserve(nInputInterfaces);
    outputInterfaceVector_.reserve(nOutputInterfaces);
}

PiSDFGraph::PiSDFGraph(PiSDFVertex *parent,
                       std::string name,
                       std::uint64_t nActors,
                       std::uint64_t nEdges,
                       std::uint64_t nParams,
                       std::uint64_t nInputInterfaces,
                       std::uint64_t nOutputInterfaces,
                       std::uint64_t nConfigActors) : PiSDFGraph(name,
                                                                 nActors,
                                                                 nEdges,
                                                                 nParams,
                                                                 nInputInterfaces,
                                                                 nOutputInterfaces,
                                                                 nConfigActors) {
    if (!parent) {
        throwSpiderException("Can not create subgraph with null parent.");
    }
    parent_ = parent;
    parent->containingGraph()->addSubgraph(this);
}

PiSDFGraph::~PiSDFGraph() {
    for (auto &v : vertexVector_) {
        Spider::destroy(v);
        Spider::deallocate(v);
        v = nullptr;
    }

    for (auto &sg: subgraphVector_) {
        Spider::destroy(sg);
        Spider::deallocate(sg);
        sg = nullptr;
    }

    for (auto &e:edgeVector_) {
        Spider::destroy(e);
        Spider::deallocate(e);
        e = nullptr;
    }

    for (auto &p:paramVector_) {
        Spider::destroy(p);
        Spider::deallocate(p);
        p = nullptr;
    }

    for (auto &inIf:inputInterfaceVector_) {
        Spider::destroy(inIf);
        Spider::deallocate(inIf);
        inIf = nullptr;
    }

    for (auto &outIf:outputInterfaceVector_) {
        Spider::destroy(outIf);
        Spider::deallocate(outIf);
        outIf = nullptr;
    }

    for (auto &c:configVertexVector_) {
        Spider::destroy(c);
        Spider::deallocate(c);
        c = nullptr;
    }
}

void PiSDFGraph::removeVertex(PiSDFVertex *vertex) {
    if (!vertex) {
        return;
    }
    if (vertex->containingGraph() != this) {
        throwSpiderException("Trying to remove a vertex [%s] that don't belong to this graph.", vertex->name().c_str());
    }
    auto ix = vertex->getIx();
    if (vertexVector_[ix] != vertex) {
        throwSpiderException("Different vertex in ix position. Expected: %s -- Got: %s", vertex->name().c_str(),
                             vertexVector_[ix]->name().c_str());
    }
    vertexVector_[ix] = vertexVector_.back();
    vertexVector_[ix]->setIx(ix);
    vertexVector_.pop_back();
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
    subgraphVector_.removeFromValue(subgraph);
    Spider::destroy(subgraph);
    Spider::deallocate(subgraph);

    /* == Recompute the static property == */
    if (!wasStatic) {
        static_ = hasDynamicParameters_;
        if (static_) {
            for (auto &g:subgraphVector_) {
                static_ &= g->isStatic();
            }
        }
    }
}

void PiSDFGraph::removeEdge(PiSDFEdge *edge) {
    if (!edge) {
        return;
    }
    if (edge->containingGraph() != this) {
        throwSpiderException("Trying to remove an edge not from this graph.");
    }
    auto ix = edge->getIx();
    if (edgeVector_[ix] != edge) {
        throwSpiderException("Different edge in ix position. Expected: %s -- Got: %s", edge->name().c_str(),
                             edgeVector_[ix]->name().c_str());
    }
    edgeVector_[ix] = edgeVector_.back();
    edgeVector_[ix]->setIx(ix);
    edgeVector_.pop_back();
    Spider::destroy(edge);
    Spider::deallocate(edge);
}

void PiSDFGraph::removeParam(PiSDFParam *param) {
    if (!param) {
        return;
    }
    if (param->containingGraph() != this) {
        throwSpiderException("Trying to remove an edge not from this graph.");
    }
    auto ix = param->getIx();
    if (paramVector_[ix] != param) {
        throwSpiderException("Different parameter in ix position. Expected: %s -- Got: %s", param->name().c_str(),
                             paramVector_[ix]->name().c_str());
    }
    paramVector_[ix] = paramVector_.back();
    paramVector_[ix]->setIx(ix);
    paramVector_.pop_back();
    Spider::destroy(param);
    Spider::deallocate(param);
}

void PiSDFGraph::addSubgraph(PiSDFGraph *subgraph) {
    if (!subgraph) {
        throwSpiderException("Can not add nullptr subgraph.");
    }
    subgraphVector_.addTail(subgraph);
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
    if (parent_) {
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
    for (const auto &v:vertexVector_) {
        v->exportDot(file, fwOffset);
    }

    if (parent_) {
        fprintf(file, "\n%s// Interfaces\n", fwOffset.c_str());
        for (const auto &i:inputInterfaceVector_) {
            i->exportDot(file, fwOffset);
        }
        for (const auto &o:outputInterfaceVector_) {
            o->exportDot(file, fwOffset);
        }
    }

    if (paramVector_.size()) {
        fprintf(file, "\n%s// Parameters\n", fwOffset.c_str());
        for (const auto &p:paramVector_) {
            p->exportDot(file, fwOffset);
        }
    }

    fprintf(file, "\n%s// Subgraphs\n", fwOffset.c_str());
    for (const auto &subgraph:subgraphVector_) {
        subgraph->exportDot(file, fwOffset);
    }

    fprintf(file, "\n%s// Vertex edges\n", fwOffset.c_str());
    for (const auto &e:edgeVector_) {
        e->exportDot(file, fwOffset);
    }

    fprintf(file, "%s}\n", parent_ ? offset.c_str() : "");
}
