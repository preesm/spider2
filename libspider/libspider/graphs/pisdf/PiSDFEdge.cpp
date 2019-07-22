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
                     std::uint16_t srcPortIx,
                     const std::string &prodExpr,
                     PiSDFVertex *sink,
                     std::uint16_t snkPortIx,
                     const std::string &consExpr) : graph_{graph} {
    if (!graph) {
        throwSpiderException("Edge should belong to a graph.");
    }
    sourcePort_ = Spider::allocate<PiSDFPort>(StackID::PISDF);
    sinkPort_ = Spider::allocate<PiSDFPort>(StackID::PISDF);
    connectSource(source, srcPortIx, prodExpr);
    connectSink(sink, snkPortIx, consExpr);
    graph->addEdge(this);
}

PiSDFEdge::PiSDFEdge(PiSDFGraph *graph,
                     PiSDFVertex *source,
                     std::uint16_t srcPortIx,
                     std::int64_t srcRate,
                     PiSDFVertex *sink,
                     std::uint16_t snkPortIx,
                     std::int64_t snkRate) : graph_{graph} {
    if (!graph) {
        throwSpiderException("Edge should belong to a graph.");
    }
    sourcePort_ = Spider::allocate<PiSDFPort>(StackID::PISDF);
    sinkPort_ = Spider::allocate<PiSDFPort>(StackID::PISDF);
    connectSource(source, srcPortIx, srcRate);
    connectSink(sink, snkPortIx, snkRate);
    graph->addEdge(this);
}

PiSDFEdge::~PiSDFEdge() {
    if (sourcePort_) {
        Spider::destroy(sourcePort_);
        Spider::deallocate(sourcePort_);
    }

    if (sinkPort_) {
        Spider::destroy(sinkPort_);
        Spider::deallocate(sinkPort_);
    }
}

void PiSDFEdge::connectSource(PiSDFVertex *vertex, std::uint16_t portIx, const std::string &prodExpr) {
    if (source_) {
        throwSpiderException("Trying to connect edge source to already connected edge.");
    }
    source_ = vertex;
    Spider::destroy(sourcePort_);
    Spider::construct(sourcePort_, vertex->containingGraph(), prodExpr);
    sourcePort_->connectEdge(this, portIx);
    source_->setOutputEdge(this, portIx);
}

void PiSDFEdge::connectSink(PiSDFVertex *vertex, std::uint32_t portIx, const std::string &consExpr) {
    if (sink_) {
        throwSpiderException("Trying to connect edge sink to already connected edge.");
    }
    sink_ = vertex;
    Spider::destroy(sinkPort_);
    Spider::construct(sinkPort_, vertex->containingGraph(), consExpr);
    sinkPort_->connectEdge(this, portIx);
    sink_->setInputEdge(this, portIx);
}

void PiSDFEdge::connectSource(PiSDFVertex *vertex, std::uint16_t portIx, std::int64_t prod) {
    if (source_) {
        throwSpiderException("Trying to connect edge source to already connected edge.");
    }
    source_ = vertex;
    Spider::destroy(sourcePort_);
    Spider::construct(sourcePort_, prod);
    sourcePort_->connectEdge(this, portIx);
    source_->setOutputEdge(this, portIx);
}

void PiSDFEdge::connectSink(PiSDFVertex *vertex, std::uint32_t portIx, std::int64_t cons) {
    if (sink_) {
        throwSpiderException("Trying to connect edge sink to already connected edge.");
    }
    sink_ = vertex;
    Spider::destroy(sinkPort_);
    Spider::construct(sinkPort_, cons);
    sinkPort_->connectEdge(this, portIx);
    source_->setInputEdge(this, portIx);
}

void PiSDFEdge::exportDot(FILE *file, const std::string &offset) const {
    fprintf(file,
            "%s\"%s\":out_%" PRIu32":e -> \"%s\":in_%" PRIu32":w [penwidth=3, "
            "color=\"#393c3c\", "
            "dir=forward, "
            "headlabel=\"%" PRIu64"   \", "
            "taillabel=\" %" PRIu64"\"];\n",
            offset.c_str(),
            source_->isHierarchical() ? source_->subgraph()->outputInterfaces()[sourcePort_->ix()]->name().c_str()
                                      : source_->name().c_str(),
            source_->isHierarchical() ? 0 : sourcePort_->ix(),
            sink_->isHierarchical() ? sink_->subgraph()->inputInterfaces()[sinkPort_->ix()]->name().c_str()
                                    : sink_->name().c_str(),
            sink_->isHierarchical() ? 0 : sinkPort_->ix(),
            sinkRate(),
            sourceRate());
}

PiSDFVertex *PiSDFEdge::source(bool forward) const {
    if (forward && source_ && source_->isHierarchical()) {
        return source_->subgraph()->outputInterfaces()[sourcePort_->ix()]->inputEdge()->source(true);
    }
    return source_;
}

PiSDFVertex *PiSDFEdge::sink(bool forward) const {
    if (forward && sink_ && sink_->isHierarchical()) {
        return sink_->subgraph()->inputInterfaces()[sinkPort_->ix()]->outputEdge()->sink(true);
    }
    return sink_;
}

std::int64_t PiSDFEdge::sourceRate() const {
    return sourcePort_->rate();
}

std::int64_t PiSDFEdge::sinkRate() const {
    return sinkPort_->rate();
}

void PiSDFEdge::disconnectSource() {
    source_ = nullptr;
    Spider::destroy(sourcePort_);
}

void PiSDFEdge::disconnectSink() {
    sink_ = nullptr;
    Spider::destroy(sinkPort_);
}

