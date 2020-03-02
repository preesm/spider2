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
#include <graphs/pisdf/ExecVertex.h>
#include <api/pisdf-api.h>

/* === Function(s) definition === */

void spider::srdag::SRDAGCopyVertexVisitor::visit(pisdf::ExecVertex *vertex) {
    if (vertex->subtype() == pisdf::VertexType::DELAY) {
        /* == This a trick to ensure proper coherence even with recursive delay init == */
        /* == For given scenario:   A -> | delay | -> B
         *                         setter --^ --> getter
         *    This will produce this:
         *                          setter -> | delay | -> getter
         *                               A -> |       | -> B
         *    But in reality the vertex does not make it after the SR-Transformation.
         */
        api::createVertex(srdag_, std::move(buildCloneName(vertex).append("(0)")), 2, 2);
        ix_ = srdag_->vertexCount() - 1;
    } else {
        makeClone(vertex);
    }
}

void spider::srdag::SRDAGCopyVertexVisitor::visit(pisdf::Graph *graph) {
    /* == Clone the vertex == */
    ix_ = 0;
    auto name = buildCloneName(graph);
    for (uint32_t it = 0; it < graph->repetitionValue(); ++it) {
        auto *clone = api::createNonExecVertex(srdag_,
                                               name + std::to_string(it) + ")",
                                               static_cast<uint32_t>(graph->inputEdgeCount()),
                                               static_cast<uint32_t>(graph->outputEdgeCount()));
        clone->setRepetitionValue(graph->repetitionValue());
        /* == Set the instance value of the vertex == */
        clone->setInstanceValue(it);
        auto *containingGraph = graph->graph();
        std::for_each(containingGraph->params().begin(),
                      containingGraph->params().end(),
                      [&clone, this](const std::shared_ptr<pisdf::Param> &p) {
                          clone->addInputParameter(job_.params_[p->ix()]);
                      });
    }
    ix_ = (srdag_->vertexCount() - 1) - (graph->repetitionValue() - 1);
}

/* === Private method(s) === */

std::string spider::srdag::SRDAGCopyVertexVisitor::buildCloneName(const pisdf::Vertex *vertex) const {
    if (job_.root_) {
        return std::string(vertex->name()).append("(");
    }
    return std::string(job_.srdagInstance_->name()).append(":").append(vertex->name()).append("(");
}

void spider::srdag::SRDAGCopyVertexVisitor::makeClone(pisdf::Vertex *vertex) {
    auto name = buildCloneName(vertex);
    for (uint32_t it = 0; it < vertex->repetitionValue(); ++it) {
        auto *clone = make<pisdf::ExecVertex, StackID::PISDF>(vertex->subtype(),
                                                              name + std::to_string(it) + ")",
                                                              vertex->inputEdgeCount(),
                                                              vertex->outputEdgeCount());

        vertex->setAsReference(clone);

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

