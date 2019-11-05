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

#include <spider-api/pisdf.h>
#include <graphs/pisdf/Graph.h>
#include <graphs/pisdf/Param.h>
#include <graphs/pisdf/Edge.h>
#include <graphs/pisdf/Delay.h>
#include <graphs/pisdf/ExecVertex.h>
#include <graphs/pisdf/specials/Specials.h>
#include <graphs/pisdf/params/DynamicParam.h>
#include <graphs/pisdf/params/InHeritedParam.h>
#include <graphs/pisdf/interfaces/InputInterface.h>
#include <graphs/pisdf/interfaces/OutputInterface.h>

/* === Methods implementation === */

PiSDFGraph *&Spider::pisdfGraph() {
    static PiSDFGraph *graph = nullptr;
    return graph;
}

PiSDFGraph *Spider::API::createGraph(std::string name,
                                     std::uint32_t actorCount,
                                     std::uint32_t edgeCount,
                                     std::uint32_t paramCount,
                                     std::uint32_t inIFCount,
                                     std::uint32_t outIFCount,
                                     std::uint32_t cfgActorCount,
                                     StackID stack) {
    auto *graph = Spider::allocate<PiSDFGraph>(stack);
    Spider::construct(graph,
                      std::move(name),
                      actorCount,
                      edgeCount,
                      paramCount,
                      inIFCount,
                      outIFCount,
                      cfgActorCount,
                      nullptr,
                      stack);
    return graph;
}

PiSDFGraph *Spider::API::createSubraph(PiSDFGraph *graph,
                                       std::string name,
                                       std::uint32_t actorCount,
                                       std::uint32_t edgeCount,
                                       std::uint32_t paramCount,
                                       std::uint32_t inIFCount,
                                       std::uint32_t outIFCount,
                                       std::uint32_t cfgActorCount,
                                       StackID stack) {
    auto *subgraph = Spider::allocate<PiSDFGraph>(stack);
    Spider::construct(subgraph,
                      std::move(name),
                      actorCount,
                      edgeCount,
                      paramCount,
                      inIFCount,
                      outIFCount,
                      cfgActorCount,
                      graph,
                      stack);
    graph->addVertex(subgraph);
    return subgraph;
}

PiSDFVertex *Spider::API::createVertex(PiSDFGraph *graph,
                                       std::string name,
                                       std::uint32_t edgeINCount,
                                       std::uint32_t edgeOUTCount,
                                       StackID stack) {
    auto *vertex = Spider::allocate<PiSDFVertex>(stack);
    Spider::construct(vertex,
                      std::move(name),
                      PiSDFVertexType::NORMAL,
                      edgeINCount,
                      edgeOUTCount,
                      graph,
                      stack);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFJoinVertex *Spider::API::createJoin(PiSDFGraph *graph,
                                         std::string name,
                                         std::uint32_t edgeINCount,
                                         StackID stack) {
    auto *vertex = Spider::allocate<PiSDFJoinVertex>(stack);
    Spider::construct(vertex,
                      std::move(name),
                      edgeINCount,
                      graph,
                      stack);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFForkVertex *Spider::API::createFork(PiSDFGraph *graph,
                                         std::string name,
                                         std::uint32_t edgeOUTCount,
                                         std::uint32_t nParamsIN,
                                         StackID stack) {
    auto *vertex = Spider::allocate<PiSDFForkVertex>(stack);
    Spider::construct(vertex,
                      std::move(name),
                      edgeOUTCount,
                      graph,
                      stack);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFTailVertex *Spider::API::createTail(PiSDFGraph *graph,
                                         std::string name,
                                         std::uint32_t edgeINCount,
                                         std::uint32_t nParamsIN,
                                         StackID stack) {
    auto *vertex = Spider::allocate<PiSDFTailVertex>(stack);
    Spider::construct(vertex,
                      std::move(name),
                      edgeINCount,
                      graph,
                      stack);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFHeadVertex *Spider::API::createHead(PiSDFGraph *graph,
                                         std::string name,
                                         std::uint32_t edgeINCount,
                                         std::uint32_t nParamsIN,
                                         StackID stack) {
    auto *vertex = Spider::allocate<PiSDFHeadVertex>(stack);
    Spider::construct(vertex,
                      std::move(name),
                      edgeINCount,
                      graph,
                      stack);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFDuplicateVertex *Spider::API::createDuplicate(PiSDFGraph *graph,
                                                   std::string name,
                                                   std::uint32_t edgeOUTCount,
                                                   std::uint32_t nParamsIN,
                                                   StackID stack) {
    auto *vertex = Spider::allocate<PiSDFDuplicateVertex>(stack);
    Spider::construct(vertex,
                      std::move(name),
                      edgeOUTCount,
                      graph,
                      stack);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFUpSampleVertex *Spider::API::createUpsample(PiSDFGraph *graph,
                                                 std::string name,
                                                 std::uint32_t nParamsIN,
                                                 StackID stack) {
    auto *vertex = Spider::allocate<PiSDFUpSampleVertex>(stack);
    Spider::construct(vertex,
                      std::move(name),
                      graph,
                      stack);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFDownSampleVertex *Spider::API::createDownsample(PiSDFGraph *graph,
                                                     std::string name,
                                                     std::uint32_t nParamsIN,
                                                     StackID stack) {
    auto *vertex = Spider::allocate<PiSDFDownSampleVertex>(stack);
    Spider::construct(vertex,
                      std::move(name),
                      graph,
                      stack);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFInitVertex *Spider::API::createInit(PiSDFGraph *graph,
                                         std::string name,
                                         uint32_t nParamsIN,
                                         StackID stack) {
    auto *vertex = Spider::allocate<PiSDFInitVertex>(stack);
    Spider::construct(vertex,
                      std::move(name),
                      graph,
                      stack);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFEndVertex *Spider::API::createEnd(PiSDFGraph *graph,
                                       std::string name,
                                       uint32_t nParamsIN,
                                       StackID stack) {
    auto *vertex = Spider::allocate<PiSDFEndVertex>(stack);
    Spider::construct(vertex,
                      std::move(name),
                      graph,
                      stack);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFVertex *Spider::API::createConfigActor(PiSDFGraph *graph,
                                            std::string name,
                                            std::uint32_t edgeINCount,
                                            std::uint32_t edgeOUTCount,
                                            std::uint32_t nParamsIN,
                                            std::uint32_t nParamsOUT,
                                            StackID stack) {
    auto *vertex = Spider::allocate<PiSDFVertex>(stack);
    Spider::construct(vertex,
                      std::move(name),
                      PiSDFVertexType::NORMAL,
                      edgeINCount,
                      edgeOUTCount,
                      graph,
                      stack);
    graph->addVertex(vertex);
    return vertex;
}

PiSDFInputInterface *Spider::API::setInputInterfaceName(PiSDFGraph *graph,
                                                        std::uint32_t ix,
                                                        std::string name) {
    auto *interface = graph->inputInterface(ix);
    if (!interface) {
        throwSpiderException("no input interface at index %"
                                     PRIu32
                                     " in graph [%s]", graph->name().c_str());
    }
    interface->setName(std::move(name));
    return interface;
}

PiSDFOutputInterface *Spider::API::setOutputInterfaceName(PiSDFGraph *graph,
                                                          std::uint32_t ix,
                                                          std::string name) {
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

PiSDFParam *Spider::API::createStaticParam(PiSDFGraph *graph,
                                           std::string name,
                                           std::int64_t value,
                                           StackID stack) {
    auto *param = Spider::allocate<PiSDFParam>(stack);
    Spider::construct(param,
                      std::move(name),
                      graph,
                      value);
    graph->addParam(param);
    return param;
}

PiSDFParam *Spider::API::createStaticParam(PiSDFGraph *graph,
                                           std::string name,
                                           std::string expression,
                                           StackID stack) {
    auto *param = Spider::allocate<PiSDFParam>(stack);
    Spider::construct(param,
                      std::move(name),
                      graph,
                      Expression(std::move(expression), graph));
    graph->addParam(param);
    return param;
}

PiSDFDynamicParam *Spider::API::createDynamicParam(PiSDFGraph *graph,
                                                   std::string name,
                                                   StackID stack) {
    auto *param = Spider::allocate<PiSDFDynamicParam>(stack);
    Spider::construct(param,
                      std::move(name),
                      graph,
                      Expression(0));
    graph->addParam(param);
    return param;
}

PiSDFDynamicParam *Spider::API::createDynamicParam(PiSDFGraph *graph,
                                                   std::string name,
                                                   std::string expression,
                                                   StackID stack) {
    auto *param = Spider::allocate<PiSDFDynamicParam>(stack);
    Spider::construct(param,
                      std::move(name),
                      graph,
                      Expression(std::move(expression), graph));
    graph->addParam(param);
    return param;
}

PiSDFInHeritedParam *Spider::API::createInheritedParam(PiSDFGraph *graph,
                                                       std::string name,
                                                       PiSDFParam *parent,
                                                       StackID stack) {
    if (!parent) {
        throwSpiderException("Cannot instantiate inherited parameter with null parent.");
    }
    auto *param = Spider::allocate<PiSDFInHeritedParam>(stack);
    Spider::construct(param,
                      std::move(name),
                      graph,
                      parent);
    graph->addParam(param);
    return param;
}

/* === Edge API === */

PiSDFEdge *Spider::API::createEdge(PiSDFAbstractVertex *source,
                                   std::uint16_t srcPortIx,
                                   std::string srcRateExpression,
                                   PiSDFAbstractVertex *sink,
                                   std::uint16_t snkPortIx,
                                   std::string snkRateExpression,
                                   StackID stack) {
    auto *edge = Spider::allocate<PiSDFEdge>(stack);
    Spider::construct(edge,
                      source,
                      srcPortIx,
                      Expression(std::move(srcRateExpression), source->containingGraph()),
                      sink,
                      snkPortIx,
                      Expression(std::move(snkRateExpression), sink->containingGraph()));
    source->containingGraph()->addEdge(edge);
    return edge;
}

PiSDFEdge *Spider::API::createEdge(PiSDFAbstractVertex *source,
                                   std::uint16_t srcPortIx,
                                   std::int64_t srcRate,
                                   PiSDFAbstractVertex *sink,
                                   std::uint16_t snkPortIx,
                                   std::int64_t snkRate,
                                   StackID stack) {
    auto *edge = Spider::allocate<PiSDFEdge>(stack);
    Spider::construct(edge,
                      source,
                      srcPortIx,
                      Expression(srcRate),
                      sink,
                      snkPortIx,
                      Expression(snkRate));
    source->containingGraph()->addEdge(edge);
    return edge;
}

PiSDFDelay *Spider::API::createDelay(PiSDFEdge *edge,
                                     std::string delayExpression,
                                     PiSDFVertex *setter,
                                     std::uint32_t setterPortIx,
                                     const std::string &setterRateExpression,
                                     PiSDFVertex *getter,
                                     std::uint32_t getterPortIx,
                                     const std::string &getterRateExpression,
                                     bool persistent,
                                     StackID stack) {
    if (delayExpression == "0") {
        Spider::Logger::printWarning(LOG_GENERAL, "delay with null value on edge [%s] ignored.\n",
                                     edge->name().c_str());
        return nullptr;
    }
    auto *delay = Spider::allocate<PiSDFDelay>(stack);
    const auto expression = delayExpression;
    Spider::construct(delay,
                      Expression(std::move(delayExpression), edge->containingGraph()),
                      edge,
                      setter,
                      setterPortIx,
                      Expression(setter ? setterRateExpression : expression, edge->containingGraph()),
                      getter,
                      getterPortIx,
                      Expression(getter ? getterRateExpression : expression, edge->containingGraph()),
                      persistent);
    return delay;
}

PiSDFDelay *Spider::API::createDelay(PiSDFEdge *edge,
                                     std::int64_t value,
                                     PiSDFVertex *setter,
                                     std::uint32_t setterPortIx,
                                     std::int64_t setterRate,
                                     PiSDFVertex *getter,
                                     std::uint32_t getterPortIx,
                                     std::int64_t getterRate,
                                     bool persistent,
                                     StackID stack) {
    if (!value) {
        Spider::Logger::printWarning(LOG_GENERAL, "delay with null value on edge [%s] ignored.\n",
                                     edge->name().c_str());
        return nullptr;
    }
    auto *delay = Spider::allocate<PiSDFDelay>(stack);
    Spider::construct(delay,
                      Expression(value),
                      edge,
                      setter,
                      setterPortIx,
                      Expression(setter ? setterRate : value),
                      getter,
                      getterPortIx,
                      Expression(getter ? getterRate : value),
                      persistent);
    return delay;
}


