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

#include "pisdf.h"
#include <graphs/pisdf/PiSDFGraph.h>
#include <graphs/pisdf/PiSDFVertex.h>
#include <graphs/pisdf/PiSDFInterface.h>
#include <graphs/pisdf/PiSDFParam.h>
#include <graphs/pisdf/PiSDFTypes.h>
#include <graphs/pisdf/PiSDFEdge.h>
#include <graphs/pisdf/PiSDFDelay.h>

/* === Methods implementation === */

PiSDFGraph *Spider::API::createGraph(const std::string &name,
                                     std::uint64_t nActors,
                                     std::uint64_t nEdges,
                                     std::uint64_t nParams,
                                     std::uint64_t nInputInterfaces,
                                     std::uint64_t nOutputInterfaces,
                                     std::uint64_t nConfigActors,
                                     StackID stack) {
    auto *graph = Spider::allocate<PiSDFGraph>(stack);
    Spider::construct(graph,
                      name,
                      nActors,
                      nEdges,
                      nParams,
                      nInputInterfaces,
                      nOutputInterfaces,
                      nConfigActors);
    return graph;
}

PiSDFGraph *Spider::API::createSubraph(PiSDFGraph *graph,
                                       const std::string &name,
                                       std::uint64_t nActors,
                                       std::uint64_t nEdges,
                                       std::uint64_t nParams,
                                       std::uint64_t nInputInterfaces,
                                       std::uint64_t nOutputInterfaces,
                                       std::uint64_t nConfigActors,
                                       StackID stack) {
    auto *vertex = Spider::allocate<PiSDFVertex>(stack);
    Spider::construct(vertex,
                      graph,
                      name,
                      PiSDFVertexType::HIERARCHICAL,
                      nInputInterfaces,
                      nOutputInterfaces,
                      0);
    auto *subgraph = vertex->subgraph();
    subgraph->precacheVertices(nActors);
    subgraph->precacheConfigVertices(nConfigActors);
    subgraph->precacheEdges(nEdges);
    subgraph->precacheParams(nParams);
    return subgraph;
}

PiSDFVertex *Spider::API::createVertex(PiSDFGraph *graph,
                                       const std::string &name,
                                       std::uint32_t nEdgesIN,
                                       std::uint32_t nEdgesOUT,
                                       std::uint32_t nParamsIN,
                                       StackID stack) {
    auto *vertex = Spider::allocate<PiSDFVertex>(stack);
    Spider::construct(vertex,
                      stack,
                      graph,
                      name,
                      PiSDFVertexType::NORMAL,
                      nEdgesIN,
                      nEdgesOUT,
                      nParamsIN,
                      0);
    return vertex;
}

PiSDFVertex *Spider::API::createDuplicate(PiSDFGraph *graph,
                                          const std::string &name,
                                          std::uint32_t nEdgesOUT,
                                          std::uint32_t nParamsIN,
                                          StackID stack) {
    auto *vertex = Spider::allocate<PiSDFVertex>(stack);
    Spider::construct(vertex,
                      stack,
                      graph,
                      name,
                      PiSDFVertexType::DUPLICATE,
                      1, /* = Broadcast vertex has only ONE input edge = */
                      nEdgesOUT,
                      nParamsIN,
                      0);
    return vertex;
}

PiSDFVertex *Spider::API::createFork(PiSDFGraph *graph,
                                     const std::string &name,
                                     std::uint32_t nEdgesOUT,
                                     std::uint32_t nParamsIN,
                                     StackID stack) {
    auto *vertex = Spider::allocate<PiSDFVertex>(stack);
    Spider::construct(vertex,
                      stack,
                      graph,
                      name,
                      PiSDFVertexType::FORK,
                      1, /* = Fork vertex has only ONE input edge = */
                      nEdgesOUT,
                      nParamsIN,
                      0);
    return vertex;
}

PiSDFVertex *Spider::API::createRoundbuffer(PiSDFGraph *graph,
                                            const std::string &name,
                                            std::uint32_t nEdgesIN,
                                            std::uint32_t nParamsIN,
                                            StackID stack) {
    auto *vertex = Spider::allocate<PiSDFVertex>(stack);
    Spider::construct(vertex,
                      stack,
                      graph,
                      name,
                      PiSDFVertexType::ROUNDBUFFER,
                      nEdgesIN,
                      1, /* = Roundbuffer vertex has only ONE output edge = */
                      nParamsIN,
                      0);
    return vertex;
}

PiSDFVertex *Spider::API::createJoin(PiSDFGraph *graph,
                                     const std::string &name,
                                     std::uint32_t nEdgesIN,
                                     std::uint32_t nParamsIN,
                                     StackID stack) {
    auto *vertex = Spider::allocate<PiSDFVertex>(stack);
    Spider::construct(vertex,
                      stack,
                      graph,
                      name,
                      PiSDFVertexType::JOIN,
                      nEdgesIN,
                      1, /* = Join vertex has only ONE output edge = */
                      nParamsIN,
                      0);
    return vertex;
}

PiSDFVertex *Spider::API::createInit(PiSDFGraph *graph, const std::string &name, uint32_t nParamsIN, StackID stack) {
    auto *vertex = Spider::allocate<PiSDFVertex>(stack);
    Spider::construct(vertex,
                      stack,
                      graph,
                      name,
                      PiSDFVertexType::INIT,
                      0, /* = Init vertex do not have input edges = */
                      1, /* = Init vertex has only ONE output edge = */
                      nParamsIN,
                      0);
    return vertex;
}

PiSDFVertex *Spider::API::createEnd(PiSDFGraph *graph, const std::string &name, uint32_t nParamsIN, StackID stack) {
    auto *vertex = Spider::allocate<PiSDFVertex>(stack);
    Spider::construct(vertex,
                      stack,
                      graph,
                      name,
                      PiSDFVertexType::END,
                      1, /* = End vertex has only ONE input edge = */
                      0, /* = End vertex do not have output edges = */
                      nParamsIN,
                      0);
    return vertex;
}

PiSDFVertex *Spider::API::createConfigActor(PiSDFGraph *graph,
                                            const std::string &name,
                                            std::uint32_t nEdgesIN,
                                            std::uint32_t nEdgesOUT,
                                            std::uint32_t nParamsIN,
                                            std::uint32_t nParamsOUT,
                                            StackID stack) {
    auto *vertex = Spider::allocate<PiSDFVertex>(stack);
    Spider::construct(vertex,
                      stack,
                      graph,
                      name,
                      PiSDFVertexType::CONFIG,
                      nEdgesIN,
                      nEdgesOUT,
                      nParamsIN,
                      nParamsOUT);
    return vertex;
}

PiSDFInterface *Spider::API::createInputInterface(PiSDFGraph *graph,
                                                  const std::string &name,
                                                  StackID stack) {
    auto *interface = Spider::allocate<PiSDFInterface>(stack);
    Spider::construct(interface,
                      graph,
                      name,
                      PiSDFInterfaceType::INPUT);
    return interface;
}

PiSDFInterface *Spider::API::createOutputInterface(PiSDFGraph *graph,
                                                   const std::string &name,
                                                   StackID stack) {
    auto *interface = Spider::allocate<PiSDFInterface>(stack);
    Spider::construct(interface,
                      graph,
                      name,
                      PiSDFInterfaceType::OUTPUT);
    return interface;
}

/* === Param creation API === */

PiSDFParam *Spider::API::createStaticParam(PiSDFGraph *graph,
                                           const std::string &name,
                                           ParamInt64 value,
                                           StackID stack) {
    auto *param = Spider::allocate<PiSDFParam>(stack);
    Spider::construct(param,
                      graph,
                      name,
                      value);
    return param;
}

PiSDFParam *Spider::API::createDynamicParam(PiSDFGraph *graph,
                                            const std::string &name,
                                            PiSDFVertex *setter,
                                            StackID stack) {
    auto *param = Spider::allocate<PiSDFParam>(stack);
    Spider::construct(param,
                      graph,
                      name,
                      setter);
    return param;
}

PiSDFParam *Spider::API::createDependentParam(PiSDFGraph *graph,
                                              const std::string &name,
                                              const std::string &expression,
                                              StackID stack) {
    auto *param = Spider::allocate<PiSDFParam>(stack);
    Spider::construct(param,
                      graph,
                      name,
                      expression);
    return param;
}

PiSDFParam *Spider::API::createInheritedParam(PiSDFGraph *graph,
                                              const std::string &name,
                                              PiSDFParam *parent,
                                              StackID stack) {
    if (!parent) {
        throwSpiderException("Cannot instantiate inherited parameter with null parent.");
    }
    auto *param = Spider::allocate<PiSDFParam>(stack);
    Spider::construct(param,
                      graph,
                      name,
                      parent);
    return param;
}

PiSDFEdge *Spider::API::createEdge(PiSDFGraph *graph,
                                   PiSDFVertex *source,
                                   std::uint16_t srcPortIx,
                                   const std::string &srcRateExpression,
                                   PiSDFVertex *sink,
                                   std::uint16_t snkPortIx,
                                   const std::string &snkRateExpression,
                                   StackID stack) {
    auto *edge = Spider::allocate<PiSDFEdge>(stack);
    Spider::construct(edge, graph, source, srcPortIx, srcRateExpression, sink, snkPortIx, snkRateExpression);
    return edge;
}

PiSDFEdge *Spider::API::createEdge(PiSDFGraph *graph,
                                   PiSDFVertex *source,
                                   std::uint16_t srcPortIx,
                                   std::int64_t srcRate,
                                   PiSDFVertex *sink,
                                   std::uint16_t snkPortIx,
                                   std::int64_t snkRate,
                                   StackID stack) {
    auto *edge = Spider::allocate<PiSDFEdge>(stack);
    Spider::construct(edge, graph, source, srcPortIx, srcRate, sink, snkPortIx, snkRate);
    return edge;
}

PiSDFEdge *Spider::API::createEdge(PiSDFGraph *graph,
                                   PiSDFVertex *source,
                                   std::uint16_t srcPortIx,
                                   std::int64_t srcRate,
                                   PiSDFVertex *sink,
                                   std::uint16_t snkPortIx,
                                   const std::string &snkRateExpression,
                                   StackID stack) {
    auto *edge = Spider::allocate<PiSDFEdge>(stack);
    Spider::construct(edge, graph, source, srcPortIx, std::to_string(srcRate), sink, snkPortIx, snkRateExpression);
    return edge;
}

PiSDFEdge *Spider::API::createEdge(PiSDFGraph *graph,
                                   PiSDFVertex *source,
                                   std::uint16_t srcPortIx,
                                   const std::string &srcRateExpression,
                                   PiSDFVertex *sink,
                                   std::uint16_t snkPortIx,
                                   std::int64_t snkRate,
                                   StackID stack) {
    auto *edge = Spider::allocate<PiSDFEdge>(stack);
    Spider::construct(edge, graph, source, srcPortIx, srcRateExpression, sink, snkPortIx, std::to_string(snkRate));
    return edge;
}

PiSDFDelay *Spider::API::createDelay(PiSDFEdge *edge,
                                     const std::string &delayExpression,
                                     bool persistent,
                                     PiSDFVertex *setter,
                                     PiSDFVertex *getter,
                                     std::uint32_t setterPortIx,
                                     std::uint32_t getterPortIx,
                                     StackID stack) {
    auto *delay = Spider::allocate<PiSDFDelay>(stack);
    Spider::construct(delay, edge, delayExpression, persistent, setter, getter, setterPortIx, getterPortIx);
    return delay;
}

PiSDFDelay *Spider::API::createDelay(PiSDFEdge *edge,
                                     std::int64_t delayValue,
                                     bool persistent,
                                     PiSDFVertex *setter,
                                     PiSDFVertex *getter,
                                     std::uint32_t setterPortIx,
                                     std::uint32_t getterPortIx,
                                     StackID stack) {
    auto *delay = Spider::allocate<PiSDFDelay>(stack);
    Spider::construct(delay, edge, delayValue, persistent, setter, getter, setterPortIx, getterPortIx);
    return delay;
}


