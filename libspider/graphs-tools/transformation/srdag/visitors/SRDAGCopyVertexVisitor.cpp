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

/* === Include(s) === */

#include <graphs-tools/transformation/srdag/visitors/SRDAGCopyVertexVisitor.h>
#include <graphs/pisdf/SpecialVertex.h>
#include <graphs/pisdf/ExecVertex.h>
#include <api/pisdf-api.h>

/* === Function(s) definition === */

void spider::srdag::SRDAGCopyVertexVisitor::visit(pisdf::DelayVertex *vertex) {
    /* == This a trick to ensure proper coherence even with recursive delay init == */
    /* == For given scenario:   A -> | delay | -> B
     *                         setter --^ --> getter
     *    This will produce this:
     *                          setter -> | delay | -> getter
     *                               A -> |       | -> B
     *    But in reality the vertex does not make it after the SR-Transformation.
     */
    api::createVertex(srdag_, buildCloneName(vertex, 0), 2, 2);
    ix_ = srdag_->vertexCount() - 1;
}

void spider::srdag::SRDAGCopyVertexVisitor::visit(pisdf::ExecVertex *vertex) {
    makeClone(vertex);
}

void spider::srdag::SRDAGCopyVertexVisitor::visit(pisdf::Graph *graph) {
    /* == Clone the vertex == */
    ix_ = 0;
    for (uint32_t it = 0; it < graph->repetitionValue(); ++it) {
        auto *clone = api::createNonExecVertex(srdag_,
                                               buildCloneName(graph, it),
                                               static_cast<uint32_t>(graph->inputEdgeCount()),
                                               static_cast<uint32_t>(graph->outputEdgeCount()));
        clone->setRepetitionValue(graph->repetitionValue());
        /* == Set the instance value of the vertex == */
        clone->setInstanceValue(it);
    }
    ix_ = (srdag_->vertexCount() - 1) - (graph->repetitionValue() - 1);
}

/* === Private method(s) === */

std::string spider::srdag::SRDAGCopyVertexVisitor::buildCloneName(const pisdf::Vertex *vertex, uint32_t firing) const {
    const auto *graphRef = job_.root_ ? job_.reference_ : srdag_->vertex(*(job_.srdagIx_));
    std::string name = graphRef->name();
    name.reserve(name.length() + vertex->name().length() + 12);
    name.append(":").append(vertex->name()).append("-").append(std::to_string(firing));
    return name;
}

void spider::srdag::SRDAGCopyVertexVisitor::makeClone(pisdf::Vertex *vertex) {
    for (uint32_t it = 0; it < vertex->repetitionValue(); ++it) {
        auto *clone = vertex->emptyClone(buildCloneName(vertex, it));

        /* == Add clone to the srdag == */
        srdag_->addVertex(clone);

        /* == Change the instance value of the clone == */
        clone->setInstanceValue(it);

        /* == Get the cloned parameters == */
        for (const auto &param : vertex->inputParamVector()) {
            clone->addInputParameter(job_.params_[param->ix()]);
        }
        for (const auto &param : vertex->refinementParamVector()) {
            clone->addRefinementParameter(job_.params_[param->ix()]);
        }
        for (const auto &param : vertex->outputParamVector()) {
            clone->addOutputParameter(job_.params_[param->ix()]);
        }
    }
    ix_ = (srdag_->vertexCount() - 1) - (vertex->repetitionValue() - 1);
}
