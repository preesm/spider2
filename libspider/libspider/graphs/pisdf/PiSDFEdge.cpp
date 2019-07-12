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

#include <graphs/pisdf/PiSDFEdge.h>
#include <graphs/pisdf/PiSDFGraph.h>
#include <graphs/pisdf/PiSDFVertex.h>

/* === Methods implementation === */

PiSDFEdge::PiSDFEdge(PiSDFGraph *graph,
                     PiSDFVertex *source,
                     std::uint32_t srcPortIx,
                     const Spider::string &prodExpr,
                     PiSDFVertex *sink,
                     std::uint32_t snkPortIx,
                     const Spider::string &consExpr) : graph_{graph},
                                                       source_{source},
                                                       sink_{sink} {
    if (!graph) {
        throwSpiderException("Edge should belong to a graph.");
    }
    setSource(source, srcPortIx, prodExpr);
    setSink(sink, snkPortIx, consExpr);
    graph->addEdge(this);
}

PiSDFEdge::PiSDFEdge(PiSDFGraph *graph,
                     PiSDFInterface *sourceIf,
                     const Spider::string &prodExpr,
                     PiSDFInterface *sinkIf,
                     const Spider::string &consExpr) : graph_{graph},
                                                       sourceIf_{sourceIf},
                                                       sinkIf_{sinkIf} {
    if (!graph) {
        throwSpiderException("Edge should belong to a graph.");
    }
    if (sourceIf == sinkIf) {
        throwSpiderException("Can not have self loop on interface: %s.", sourceIf->name().c_str());
    }
    setSource(sourceIf, prodExpr);
    setSink(sinkIf, consExpr);
    graph->addEdge(this);
}

PiSDFEdge::PiSDFEdge(PiSDFGraph *graph,
                     PiSDFInterface *sourceIf,
                     const Spider::string &prodExpr,
                     PiSDFVertex *sink,
                     std::uint32_t snkPortIx,
                     const Spider::string &consExpr) : graph_{graph},
                                                       sink_{sink},
                                                       sourceIf_{sourceIf} {
    if (!graph) {
        throwSpiderException("Edge should belong to a graph.");
    }
    if (graph != sourceIf->containingGraph() &&
        sourceIf->containingGraph() == sink->containingGraph()) {
        throwSpiderException("Edge should belong to the same graph as the sink and the interface.");
    }
    setSource(sourceIf, prodExpr);
    setSink(sink, snkPortIx, consExpr);
    graph->addEdge(this);
}

PiSDFEdge::PiSDFEdge(PiSDFGraph *graph,
                     PiSDFVertex *source,
                     std::uint32_t srcPortIx,
                     const Spider::string &prodExpr,
                     PiSDFInterface *sinkIf,
                     const Spider::string &consExpr) : graph_{graph},
                                                       source_{source},
                                                       sinkIf_{sinkIf} {
    if (!graph) {
        throwSpiderException("Edge should belong to a graph.");
    }
    if (graph != sinkIf->containingGraph() &&
        sinkIf->containingGraph() == source->containingGraph()) {
        throwSpiderException("Edge should belong to the same graph as the source and the interface.");
    }
    setSource(source, srcPortIx, prodExpr);
    setSink(sinkIf, consExpr);
    graph->addEdge(this);
}

PiSDFEdge::~PiSDFEdge() {
    if (sourceRateExpr_) {
        Spider::destroy(sourceRateExpr_);
        Spider::deallocate(sourceRateExpr_);
    }

    if (sinkRateExpr_) {
        Spider::destroy(sinkRateExpr_);
        Spider::deallocate(sinkRateExpr_);
    }
}

std::uint64_t PiSDFEdge::sourceRate() const {
    return sourceRateExpr_->evaluate();
}


std::uint64_t PiSDFEdge::sinkRate() const {
    return sinkRateExpr_->evaluate();
}

void PiSDFEdge::setSource(PiSDFVertex *vertex, std::uint32_t srcPortIx, const Spider::string &prodExpr) {
    source_ = vertex;
    sourcePortIx_ = srcPortIx;
    if (sourceRateExpr_) {
        Spider::destroy(sourceRateExpr_);
    } else {
        sourceRateExpr_ = Spider::allocate<Expression>(StackID::PISDF);
    }
    Spider::construct(sourceRateExpr_, prodExpr, graph_);
    source_->setOutputEdge(this, srcPortIx);
}

void PiSDFEdge::setSource(PiSDFInterface *interface, const Spider::string &prodExpr) {
    if (source_) {
        throwSpiderException("Can not connect iif [%s]. Edge already has a source: [%s].",
                             interface->name().c_str(),
                             source_->name().c_str());
    }
    sourceIf_ = interface;
    sourcePortIx_ = 0;
    if (sourceRateExpr_) {
        Spider::destroy(sourceRateExpr_);
    } else {
        sourceRateExpr_ = Spider::allocate<Expression>(StackID::PISDF);
    }
    Spider::construct(sourceRateExpr_, prodExpr, graph_);
    sourceIf_->setOutputEdge(this);
}

void PiSDFEdge::setSink(PiSDFVertex *vertex, std::uint32_t snkPortIx, const Spider::string &consExpr) {

    sink_ = vertex;
    sinkPortIx_ = snkPortIx;
    if (sinkRateExpr_) {
        Spider::destroy(sinkRateExpr_);
    } else {
        sinkRateExpr_ = Spider::allocate<Expression>(StackID::PISDF);
    }
    Spider::construct(sinkRateExpr_, consExpr, graph_);
    sink_->setInputEdge(this, snkPortIx);
}

void PiSDFEdge::setSink(PiSDFInterface *interface, const Spider::string &consExpr) {
    if (sink_) {
        throwSpiderException("Can not connect oif [%s]. Edge already has a sink: [%s].",
                             interface->name().c_str(),
                             sink_->name().c_str());
    }
    sinkIf_ = interface;
    sinkPortIx_ = 0;
    if (sinkRateExpr_) {
        Spider::destroy(sinkRateExpr_);
    } else {
        sinkRateExpr_ = Spider::allocate<Expression>(StackID::PISDF);
    }
    Spider::construct(sinkRateExpr_, consExpr, graph_);
    sinkIf_->setInputEdge(this);
}

void PiSDFEdge::exportDot(FILE *file, const Spider::string &offset) const {
    fprintf(file,
            "%s\"%s\":out_%" PRIu32":e -> \"%s\":in_%" PRIu32":w [penwidth=3, "
            "color=\"#393c3c\", "
            "dir=forward, "
            "headlabel=\"%" PRIu64"   \", "
            "taillabel=\" %" PRIu64"\"];\n",
            offset.c_str(),
            source_ ? source_->name().c_str() : sourceIf_->name().c_str(),
            sourcePortIx_,
            sink_ ? sink_->name().c_str() : sinkIf_->name().c_str(),
            sinkPortIx_,
            sinkRate(),
            sourceRate());
}

