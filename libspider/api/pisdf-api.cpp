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

#include <api/pisdf-api.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/params/Param.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs/pisdf/params/InHeritedParam.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>

/* === Methods implementation === */

PiSDFGraph *&spider::pisdfGraph() {
    static PiSDFGraph *graph = nullptr;
    return graph;
}

PiSDFGraph *spider::api::createGraph(std::string name,
                                     uint32_t actorCount,
                                     uint32_t edgeCount,
                                     uint32_t paramCount,
                                     uint32_t inIFCount,
                                     uint32_t outIFCount,
                                     uint32_t cfgActorCount,
                                     StackID stack) {
    return spider::make<PiSDFGraph>(stack,
                                    std::move(name),
                                    actorCount,
                                    edgeCount,
                                    paramCount,
                                    inIFCount,
                                    outIFCount,
                                    cfgActorCount,
                                    stack);
}

PiSDFGraph *spider::api::createSubraph(PiSDFGraph *graph,
                                       std::string name,
                                       uint32_t actorCount,
                                       uint32_t edgeCount,
                                       uint32_t paramCount,
                                       uint32_t inIFCount,
                                       uint32_t outIFCount,
                                       uint32_t cfgActorCount,
                                       StackID stack) {
    auto *subgraph = spider::make<PiSDFGraph>(stack,
                                              std::move(name),
                                              actorCount,
                                              edgeCount,
                                              paramCount,
                                              inIFCount,
                                              outIFCount,
                                              cfgActorCount,
                                              stack);
    graph->addVertex(subgraph);
    return subgraph;
}

PiSDFVertex *spider::api::createVertex(PiSDFGraph *graph,
                                       std::string name,
                                       uint32_t edgeINCount,
                                       uint32_t edgeOUTCount,
                                       StackID stack) {
    auto *vertex = spider::make<PiSDFVertex>(stack, std::move(name), edgeINCount, edgeOUTCount, stack);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFVertex *spider::api::createVertex(PiSDFGraph *graph,
                                       uint32_t refinementIx,
                                       std::string name,
                                       uint32_t edgeINCount,
                                       uint32_t edgeOUTCount,
                                       StackID stack) {
    auto *vertex = spider::api::createVertex(graph, std::move(name), edgeINCount, edgeOUTCount, stack);
    vertex->setRefinementIx(refinementIx);
    return vertex;
}

PiSDFForkVertex *
spider::api::createFork(PiSDFGraph *graph, std::string name, uint32_t edgeOUTCount, StackID stack) {
    auto *vertex = spider::make<PiSDFForkVertex>(stack, std::move(name), edgeOUTCount, stack);
    vertex->setRefinementIx(0);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFJoinVertex *
spider::api::createJoin(PiSDFGraph *graph, std::string name, uint32_t edgeINCount, StackID stack) {
    auto *vertex = spider::make<PiSDFJoinVertex>(stack, std::move(name), edgeINCount, stack);
    vertex->setRefinementIx(1);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFHeadVertex *
spider::api::createHead(PiSDFGraph *graph, std::string name, uint32_t edgeINCount, StackID stack) {
    auto *vertex = spider::make<PiSDFHeadVertex>(stack, std::move(name), edgeINCount, stack);
    vertex->setRefinementIx(2);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFTailVertex *
spider::api::createTail(PiSDFGraph *graph, std::string name, uint32_t edgeINCount, StackID stack) {
    auto *vertex = spider::make<PiSDFTailVertex>(stack, std::move(name), edgeINCount, stack);
    vertex->setRefinementIx(3);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFDuplicateVertex *
spider::api::createDuplicate(PiSDFGraph *graph, std::string name, uint32_t edgeOUTCount, StackID stack) {
    auto *vertex = spider::make<PiSDFDuplicateVertex>(stack, std::move(name), edgeOUTCount, stack);
    vertex->setRefinementIx(4);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFRepeatVertex *spider::api::createRepeat(PiSDFGraph *graph, std::string name, StackID stack) {
    auto *vertex = spider::make<PiSDFRepeatVertex>(stack, std::move(name), stack);
    vertex->setRefinementIx(5);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFInitVertex *spider::api::createInit(PiSDFGraph *graph, std::string name, StackID stack) {
    auto *vertex = spider::make<PiSDFInitVertex>(stack, std::move(name), stack);
    vertex->setRefinementIx(6);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFEndVertex *spider::api::createEnd(PiSDFGraph *graph, std::string name, StackID stack) {
    auto *vertex = spider::make<PiSDFEndVertex>(stack, std::move(name), stack);
    vertex->setRefinementIx(7);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFCFGVertex *spider::api::createConfigActor(PiSDFGraph *graph,
                                               std::string name,
                                               uint32_t edgeINCount,
                                               uint32_t edgeOUTCount,
                                               StackID stack) {
    auto *vertex = spider::make<PiSDFCFGVertex>(stack, std::move(name), edgeINCount, edgeOUTCount, stack);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFInputInterface *spider::api::setInputInterfaceName(PiSDFGraph *graph, uint32_t ix, std::string name) {
    auto *interface = graph->inputInterface(ix);
    if (!interface) {
        throwSpiderException("no input interface at index %"
                                     PRIu32
                                     " in graph [%s]", graph->name().c_str());
    }
    interface->setName(std::move(name));
    return interface;
}

PiSDFOutputInterface *spider::api::setOutputInterfaceName(PiSDFGraph *graph, uint32_t ix, std::string name) {
    auto *interface = graph->outputInterface(ix);
    if (!interface) {
        throwSpiderException("no output interface at index %"
                                     PRIu32
                                     " in graph [%s]", graph->name().c_str());
    }
    interface->setName(std::move(name));
    return interface;
}

/* === Param creation API === */

PiSDFParam *spider::api::createStaticParam(PiSDFGraph *graph, std::string name, int64_t value, StackID stack) {
    auto *param = spider::make<PiSDFParam>(stack, std::move(name), value);
    if (graph) {
        graph->addParam(param);
    }
    return param;
}

PiSDFParam *spider::api::createStaticParam(PiSDFGraph *graph, std::string name, std::string expression, StackID stack) {
    if (graph) {
        auto *param = spider::make<PiSDFParam>(stack, std::move(name),
                                               Expression(std::move(expression), graph->params()));
        graph->addParam(param);
        return param;
    }
    auto *param = spider::make<PiSDFParam>(stack, std::move(name), Expression(std::move(expression)));
    return param;
}

PiSDFDynamicParam *spider::api::createDynamicParam(PiSDFGraph *graph, std::string name, StackID stack) {
    auto *param = spider::make<PiSDFDynamicParam>(stack, std::move(name), Expression(0));
    if (graph) {
        graph->addParam(param);
    }
    return param;
}

PiSDFDynamicParam *
spider::api::createDynamicParam(PiSDFGraph *graph, std::string name, std::string expression, StackID stack) {
    if (graph) {
        auto *param = spider::make<PiSDFDynamicParam>(stack, std::move(name),
                                                      Expression(std::move(expression), graph->params()));
        graph->addParam(param);
        return param;
    }
    auto *param = spider::make<PiSDFDynamicParam>(stack, std::move(name), Expression(std::move(expression)));
    return param;
}

PiSDFParam *spider::api::createInheritedParam(PiSDFGraph *graph, std::string name, PiSDFParam *parent, StackID stack) {
    if (!parent) {
        throwSpiderException("Cannot instantiate inherited parameter with null parent.");
    }
    if (!parent->dynamic()) {
        return spider::api::createStaticParam(graph, std::move(name), parent->value(), stack);
    }
    auto *param = spider::make<PiSDFInHeritedParam>(stack, std::move(name), parent);
    if (graph) {
        graph->addParam(param);
    }
    return param;
}

/* === Edge API === */

PiSDFEdge *spider::api::createEdge(PiSDFAbstractVertex *source,
                                   uint32_t srcPortIx,
                                   std::string srcRateExpression,
                                   PiSDFAbstractVertex *sink,
                                   uint32_t snkPortIx,
                                   std::string snkRateExpression,
                                   StackID stack) {
    auto *edge = spider::make<PiSDFEdge>(stack,
                                         source,
                                         srcPortIx,
                                         Expression(std::move(srcRateExpression), source->graph()->params()),
                                         sink,
                                         snkPortIx,
                                         Expression(std::move(snkRateExpression), sink->graph()->params()));
    source->graph()->addEdge(edge);
    return edge;
}

PiSDFEdge *spider::api::createEdge(PiSDFAbstractVertex *source,
                                   uint32_t srcPortIx,
                                   int64_t srcRate,
                                   PiSDFAbstractVertex *sink,
                                   uint32_t snkPortIx,
                                   int64_t snkRate,
                                   StackID stack) {
    auto *edge = spider::make<PiSDFEdge>(stack,
                                         source,
                                         srcPortIx,
                                         Expression(srcRate),
                                         sink,
                                         snkPortIx,
                                         Expression(snkRate));
    source->graph()->addEdge(edge);
    return edge;
}

PiSDFDelay *spider::api::createDelay(PiSDFEdge *edge,
                                     std::string delayExpression,
                                     PiSDFVertex *setter,
                                     uint32_t setterPortIx,
                                     const std::string &setterRateExpression,
                                     PiSDFVertex *getter,
                                     uint32_t getterPortIx,
                                     const std::string &getterRateExpression,
                                     bool persistent,
                                     StackID stack) {
    if (delayExpression == "0" && log_enabled()) {
        spider::log::warning("delay with null value on edge [%s] ignored.\n",
                             edge->name().c_str());
        return nullptr;
    }
    const auto expression = delayExpression;
    auto *delay = spider::make<PiSDFDelay>(stack,
                                           Expression(std::move(delayExpression), edge->graph()->params()),
                                           edge,
                                           setter,
                                           setterPortIx,
                                           Expression(setter ? setterRateExpression : expression,
                                                      edge->graph()->params()),
                                           getter,
                                           getterPortIx,
                                           Expression(getter ? getterRateExpression : expression,
                                                      edge->graph()->params()),
                                           persistent);
    return delay;
}

PiSDFDelay *spider::api::createDelay(PiSDFEdge *edge,
                                     int64_t value,
                                     PiSDFVertex *setter,
                                     uint32_t setterPortIx,
                                     int64_t setterRate,
                                     PiSDFVertex *getter,
                                     uint32_t getterPortIx,
                                     int64_t getterRate,
                                     bool persistent,
                                     StackID stack) {
    if (!value && log_enabled()) {
        spider::log::warning("delay with null value on edge [%s] ignored.\n",
                             edge->name().c_str());
        return nullptr;
    }
    return spider::make<PiSDFDelay>(stack,
                                    Expression(value),
                                    edge,
                                    setter,
                                    setterPortIx,
                                    Expression(setter ? setterRate : value),
                                    getter,
                                    getterPortIx,
                                    Expression(getter ? getterRate : value),
                                    persistent);
}