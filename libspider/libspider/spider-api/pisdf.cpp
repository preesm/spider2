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

/* === Methods implementation === */

PiSDFGraph *Spider::createGraph(const std::string &name,
                                std::uint64_t nActors,
                                std::uint64_t nEdges,
                                std::uint64_t nParams,
                                std::uint64_t nInputInterfaces,
                                std::uint64_t nOutputInterfaces,
                                std::uint64_t nConfigActors) {
    auto *graph = Spider::allocate<PiSDFGraph>(StackID::PISDF);
    Spider::construct(graph,
                      Spider::string{name.c_str()},
                      nActors,
                      nEdges,
                      nParams,
                      nInputInterfaces,
                      nOutputInterfaces,
                      nConfigActors);
    return graph;
}

PiSDFVertex *Spider::createVertex(PiSDFGraph *graph,
                                  const std::string &name,
                                  std::uint32_t nEdgesIN,
                                  std::uint32_t nEdgesOUT,
                                  std::uint32_t nParamsIN) {
    auto *vertex = Spider::allocate<PiSDFVertex>(StackID::PISDF);
    Spider::construct(vertex,
                      graph,
                      name,
                      PiSDFType::VERTEX,
                      PiSDFSubType::NORMAL,
                      nEdgesIN,
                      nEdgesOUT,
                      nParamsIN);
    return vertex;
}

PiSDFVertex *Spider::createBroadcast(PiSDFGraph *graph,
                                     const std::string &name,
                                     std::uint32_t nEdgesOUT,
                                     std::uint32_t nParamsIN) {
    auto *vertex = Spider::allocate<PiSDFVertex>(StackID::PISDF);
    Spider::construct(vertex,
                      graph,
                      name,
                      PiSDFType::VERTEX,
                      PiSDFSubType::BROADCAST,
                      1, /* = Broadcast vertex has only ONE input edge = */
                      nEdgesOUT,
                      nParamsIN);
    return vertex;
}

PiSDFVertex *Spider::createFork(PiSDFGraph *graph,
                                const std::string &name,
                                std::uint32_t nEdgesOUT,
                                std::uint32_t nParamsIN) {
    auto *vertex = Spider::allocate<PiSDFVertex>(StackID::PISDF);
    Spider::construct(vertex,
                      graph,
                      name,
                      PiSDFType::VERTEX,
                      PiSDFSubType::FORK,
                      1, /* = Fork vertex has only ONE input edge = */
                      nEdgesOUT,
                      nParamsIN);
    return vertex;
}

PiSDFVertex *Spider::createRoundbuffer(PiSDFGraph *graph,
                                       const std::string &name,
                                       std::uint32_t nEdgesIN,
                                       std::uint32_t nParamsIN) {
    auto *vertex = Spider::allocate<PiSDFVertex>(StackID::PISDF);
    Spider::construct(vertex,
                      graph,
                      name,
                      PiSDFType::VERTEX,
                      PiSDFSubType::ROUNDBUFFER,
                      nEdgesIN,
                      1, /* = Roundbuffer vertex has only ONE output edge = */
                      nParamsIN);
    return vertex;
}

PiSDFVertex *Spider::createJoin(PiSDFGraph *graph,
                                const std::string &name,
                                std::uint32_t nEdgesIN,
                                std::uint32_t nParamsIN) {
    auto *vertex = Spider::allocate<PiSDFVertex>(StackID::PISDF);
    Spider::construct(vertex,
                      graph,
                      name,
                      PiSDFType::VERTEX,
                      PiSDFSubType::JOIN,
                      nEdgesIN,
                      1, /* = Join vertex has only ONE output edge = */
                      nParamsIN);
    return vertex;
}

PiSDFVertex *Spider::createInit(PiSDFGraph *graph, const std::string &name, uint32_t nParamsIN) {
    auto *vertex = Spider::allocate<PiSDFVertex>(StackID::PISDF);
    Spider::construct(vertex,
                      graph,
                      name,
                      PiSDFType::VERTEX,
                      PiSDFSubType::INIT,
                      0, /* = Init vertex do not have input edges = */
                      1, /* = Init vertex has only ONE output edge = */
                      nParamsIN);
    return vertex;
}

PiSDFVertex *Spider::createEnd(PiSDFGraph *graph, const std::string &name, uint32_t nParamsIN) {
    auto *vertex = Spider::allocate<PiSDFVertex>(StackID::PISDF);
    Spider::construct(vertex,
                      graph,
                      name,
                      PiSDFType::VERTEX,
                      PiSDFSubType::END,
                      1, /* = End vertex has only ONE input edge = */
                      0, /* = End vertex do not have output edges = */
                      nParamsIN);
    return vertex;
}

PiSDFVertex *Spider::createConfigActor(PiSDFGraph *graph,
                                       const std::string &name,
                                       std::uint32_t nEdgesIN,
                                       std::uint32_t nEdgesOUT,
                                       std::uint32_t nParamsIN,
                                       std::uint32_t nParamsOUT) {
    auto *vertex = Spider::allocate<PiSDFVertex>(StackID::PISDF);
    Spider::construct(vertex,
                      graph,
                      name,
                      PiSDFType::CONFIG_VERTEX,
                      PiSDFSubType::NORMAL,
                      nEdgesIN,
                      nEdgesOUT,
                      nParamsIN,
                      nParamsOUT);
    return vertex;
}

PiSDFInterface *Spider::createInputInterface(PiSDFGraph *graph,
                                             const std::string &name,
                                             std::uint32_t portIx,
                                             PiSDFEdge *inputEdge,
                                             PiSDFEdge *outputEdge) {
    auto *interface = Spider::allocate<PiSDFInterface>(StackID::PISDF);
    Spider::construct(interface,
                      graph,
                      name,
                      PiSDFInterfaceType::INPUT,
                      portIx,
                      inputEdge,
                      outputEdge);
    return interface;
}

PiSDFInterface *Spider::createOutputInterface(PiSDFGraph *graph,
                                              const std::string &name,
                                              std::uint32_t portIx,
                                              PiSDFEdge *inputEdge,
                                              PiSDFEdge *outputEdge) {
    auto *interface = Spider::allocate<PiSDFInterface>(StackID::PISDF);
    Spider::construct(interface,
                      graph,
                      name,
                      PiSDFInterfaceType::OUTPUT,
                      portIx,
                      inputEdge,
                      outputEdge);
    return interface;
}
