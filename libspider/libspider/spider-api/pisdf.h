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
#ifndef SPIDER2_PISDF_H
#define SPIDER2_PISDF_H

/* === Include(s) === */

#include <cstdint>
#include <string>
#include <spider-api/general.h>

/* === API methods === */

namespace Spider {

    /* === Forward declare graph names === */

    namespace PiSDF {
        class Vertex;

        class ExecVertex;

        class JoinVertex;

        class ForkVertex;

        class TailVertex;

        class HeadVertex;

        class DuplicateVertex;

        class UpSampleVertex;

        class DownSampleVertex;

        class InitVertex;

        class EndVertex;

        class Graph;

        class Param;

        class DynamicParam;

        class InHeritedParam;

        class Edge;

        class Delay;

        class Interface;

        class InputInterface;

        class OutputInterface;

        class Refinement;
    }

    /**
     * @brief Get the user defined graph of the Spider session.
     * @return
     */
    PiSDF::Graph *&pisdfGraph();

    namespace API {

        /* === Graph API === */

        PiSDF::Graph *createGraph(std::string name,
                                  std::uint32_t actorCount = 0,
                                  std::uint32_t edgeCount = 0,
                                  std::uint32_t paramCount = 0,
                                  std::uint32_t inIFCount = 0,
                                  std::uint32_t outIFCount = 0,
                                  std::uint32_t cfgActorCount = 0,
                                  StackID stack = StackID::PISDF);

        PiSDF::Graph *createSubraph(PiSDF::Graph *graph,
                                    std::string name,
                                    std::uint32_t actorCount = 0,
                                    std::uint32_t edgeCount = 0,
                                    std::uint32_t paramCount = 0,
                                    std::uint32_t inIFCount = 0,
                                    std::uint32_t outIFCount = 0,
                                    std::uint32_t cfgActorCount = 0,
                                    StackID stack = StackID::PISDF);

        PiSDF::ExecVertex *createVertex(PiSDF::Graph *graph,
                                        std::string name,
                                        std::uint32_t edgeINCount = 0,
                                        std::uint32_t edgeOUTCount = 0,
                                        StackID stack = StackID::PISDF);

        PiSDF::ExecVertex *createVertex(PiSDF::Graph *graph,
                                        std::uint32_t refinementIx,
                                        std::string name,
                                        std::uint32_t edgeINCount = 0,
                                        std::uint32_t edgeOUTCount = 0,
                                        StackID stack = StackID::PISDF);

        PiSDF::ForkVertex *createFork(PiSDF::Graph *graph,
                                      std::string name,
                                      std::uint32_t edgeOUTCount = 0,
                                      StackID stack = StackID::PISDF);

        PiSDF::JoinVertex *createJoin(PiSDF::Graph *graph,
                                      std::string name,
                                      std::uint32_t edgeINCount = 0,
                                      StackID stack = StackID::PISDF);

        PiSDF::HeadVertex *createHead(PiSDF::Graph *graph,
                                      std::string name,
                                      std::uint32_t edgeINCount = 0,
                                      StackID stack = StackID::PISDF);

        PiSDF::TailVertex *createTail(PiSDF::Graph *graph,
                                      std::string name,
                                      std::uint32_t edgeINCount = 0,
                                      StackID stack = StackID::PISDF);

        PiSDF::DuplicateVertex *createDuplicate(PiSDF::Graph *graph,
                                                std::string name,
                                                std::uint32_t edgeOUTCount = 0,
                                                StackID stack = StackID::PISDF);

        PiSDF::UpSampleVertex *createUpsample(PiSDF::Graph *graph,
                                              std::string name,
                                              StackID stack = StackID::PISDF);

        PiSDF::InitVertex *createInit(PiSDF::Graph *graph,
                                      std::string name,
                                      StackID stack = StackID::PISDF);

        PiSDF::EndVertex *createEnd(PiSDF::Graph *graph,
                                    std::string name,
                                    StackID stack = StackID::PISDF);

        PiSDF::ExecVertex *createConfigActor(PiSDF::Graph *graph,
                                             std::string name,
                                             std::uint32_t edgeINCount = 0,
                                             std::uint32_t edgeOUTCount = 0,
                                             StackID stack = StackID::PISDF);

        PiSDF::InputInterface *setInputInterfaceName(PiSDF::Graph *graph,
                                                     std::uint32_t ix,
                                                     std::string name);

        PiSDF::OutputInterface *setOutputInterfaceName(PiSDF::Graph *graph,
                                                       std::uint32_t ix,
                                                       std::string name);

        /* === Param API === */

        PiSDF::Param *createStaticParam(PiSDF::Graph *graph,
                                        std::string name,
                                        std::int64_t value,
                                        StackID stack = StackID::PISDF);

        PiSDF::Param *createStaticParam(PiSDF::Graph *graph,
                                        std::string name,
                                        std::string expression,
                                        StackID stack = StackID::PISDF);

        PiSDF::DynamicParam *createDynamicParam(PiSDF::Graph *graph,
                                                std::string name,
                                                StackID stack = StackID::PISDF);

        PiSDF::DynamicParam *createDynamicParam(PiSDF::Graph *graph,
                                                std::string name,
                                                std::string expression,
                                                StackID stack = StackID::PISDF);

        PiSDF::Param *createInheritedParam(PiSDF::Graph *graph,
                                           std::string name,
                                           PiSDF::Param *parent,
                                           StackID stack = StackID::PISDF);

        /* === Edge API === */

        PiSDF::Edge *createEdge(PiSDF::Vertex *source,
                                std::uint16_t srcPortIx,
                                std::string srcRateExpression,
                                PiSDF::Vertex *sink,
                                std::uint16_t snkPortIx,
                                std::string snkRateExpression,
                                StackID stack = StackID::PISDF);

        PiSDF::Edge *createEdge(PiSDF::Vertex *source,
                                std::uint16_t srcPortIx,
                                std::int64_t srcRate,
                                PiSDF::Vertex *sink,
                                std::uint16_t snkPortIx,
                                std::int64_t snkRate,
                                StackID stack = StackID::PISDF);

        PiSDF::Delay *createDelay(PiSDF::Edge *edge,
                                  std::string delayExpression,
                                  PiSDF::ExecVertex *setter = nullptr,
                                  std::uint32_t setterPortIx = 0,
                                  const std::string &setterRateExpression = "0",
                                  PiSDF::ExecVertex *getter = nullptr,
                                  std::uint32_t getterPortIx = 0,
                                  const std::string &getterRateExpression = "0",
                                  bool persistent = true,
                                  StackID stack = StackID::PISDF);

        PiSDF::Delay *createDelay(PiSDF::Edge *edge,
                                  std::int64_t delayValue,
                                  PiSDF::ExecVertex *setter = nullptr,
                                  std::uint32_t setterPortIx = 0,
                                  std::int64_t setterRate = 0,
                                  PiSDF::ExecVertex *getter = nullptr,
                                  std::uint32_t getterPortIx = 0,
                                  std::int64_t getterRate = 0,
                                  bool persistent = true,
                                  StackID stack = StackID::PISDF);
    }
}

#endif //SPIDER2_PISDF_H
