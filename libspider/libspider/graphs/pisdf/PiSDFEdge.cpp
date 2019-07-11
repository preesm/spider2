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

PiSDFEdge::PiSDFEdge(PiSDFGraph *graph, PiSDFVertex *source, PiSDFVertex *sink) : graph_{graph},
                                                                                  source_{source},
                                                                                  sink_{sink} {
    if (!graph) {
        throwSpiderException("Edge should belong to a graph.");
    }
    graph->addEdge(this);
}

PiSDFEdge::PiSDFEdge(PiSDFGraph *graph,
                     PiSDFVertex *source,
                     std::uint32_t srcPortIx,
                     std::string prodExpr,
                     PiSDFVertex *sink,
                     std::uint32_t snkPortIx,
                     std::string consExpr) : PiSDFEdge(graph, source, sink) {
    setSource(source, srcPortIx, std::move(prodExpr));
    setSink(sink, snkPortIx, std::move(consExpr));
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

void PiSDFEdge::setSource(PiSDFVertex *vertex, std::uint32_t srcPortIx, std::string prodExpr) {
    source_ = vertex;
    sourcePortIx_ = srcPortIx;
    if (sourceRateExpr_) {
        Spider::destroy(sourceRateExpr_);
    } else {
        sourceRateExpr_ = Spider::allocate<Expression>(StackID::PISDF);
    }
    Spider::construct(sourceRateExpr_, prodExpr, graph_);
    source_->setOutputEdge(this, srcPortIx);
    if (source_->isHierarchical()) {
        /* == Fetch the corresponding interface == */
        auto *subgraph = source_->subgraph();
        auto *outputIf = subgraph->outputInterfaces()[srcPortIx];
        sourceIf_ = outputIf;
        sourceIf_->setOutputEdge(this);
    }
}

void PiSDFEdge::setSink(PiSDFVertex *vertex, std::uint32_t snkPortIx, std::string consExpr) {
    sink_ = vertex;
    sinkPortIx_ = snkPortIx;
    if (sinkRateExpr_) {
        Spider::destroy(sinkRateExpr_);
    } else {
        sinkRateExpr_ = Spider::allocate<Expression>(StackID::PISDF);
    }
    Spider::construct(sinkRateExpr_, consExpr, graph_);
    sink_->setInputEdge(this, snkPortIx);
    if (sink_->isHierarchical()) {
        /* == Fetch the corresponding interface == */
        auto *subgraph = sink_->subgraph();
        auto *inputIf = subgraph->inputInterfaces()[snkPortIx];
        sinkIf_ = inputIf;
        sinkIf_->setInputEdge(this);
    }
}

void PiSDFEdge::exportDot(FILE *file) const {
    fprintf(file,
            "\t\"%s\":out_%" PRIu32":e -> \"%s\":in_%" PRIu32":w [penwidth=3, "
            "color=\"#393c3c\", "
            "dir=forward, "
            "headlabel=\"%" PRIu64"   \", "
            "taillabel=\" %" PRIu64"\"];\n",
            source_->name().c_str(),
            sourcePortIx_,
            sink_->name().c_str(),
            sinkPortIx_,
            sinkRate(),
            sourceRate());
}

