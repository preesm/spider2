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
#include <spider-api/global.h>

/* === API methods === */

namespace spider {

    /**
     * @brief Get the user defined graph of the Spider session.
     * @return
     */
    pisdf::Graph *&pisdfGraph();

    namespace api {

        /* === Graph API === */

        pisdf::Graph *createGraph(std::string name,
                                  uint32_t actorCount = 0,
                                  uint32_t edgeCount = 0,
                                  uint32_t paramCount = 0,
                                  uint32_t inIFCount = 0,
                                  uint32_t outIFCount = 0,
                                  uint32_t cfgActorCount = 0,
                                  StackID stack = StackID::PISDF);

        pisdf::Graph *createSubraph(pisdf::Graph *graph,
                                    std::string name,
                                    uint32_t actorCount = 0,
                                    uint32_t edgeCount = 0,
                                    uint32_t paramCount = 0,
                                    uint32_t inIFCount = 0,
                                    uint32_t outIFCount = 0,
                                    uint32_t cfgActorCount = 0,
                                    StackID stack = StackID::PISDF);

        pisdf::ExecVertex *createVertex(pisdf::Graph *graph,
                                        std::string name,
                                        uint32_t edgeINCount = 0,
                                        uint32_t edgeOUTCount = 0,
                                        StackID stack = StackID::PISDF);

        pisdf::ExecVertex *createVertex(pisdf::Graph *graph,
                                        uint32_t refinementIx,
                                        std::string name,
                                        uint32_t edgeINCount = 0,
                                        uint32_t edgeOUTCount = 0,
                                        StackID stack = StackID::PISDF);

        pisdf::ForkVertex *createFork(pisdf::Graph *graph,
                                      std::string name,
                                      uint32_t edgeOUTCount = 0,
                                      StackID stack = StackID::PISDF);

        pisdf::JoinVertex *createJoin(pisdf::Graph *graph,
                                      std::string name,
                                      uint32_t edgeINCount = 0,
                                      StackID stack = StackID::PISDF);

        pisdf::HeadVertex *createHead(pisdf::Graph *graph,
                                      std::string name,
                                      uint32_t edgeINCount = 0,
                                      StackID stack = StackID::PISDF);

        pisdf::TailVertex *createTail(pisdf::Graph *graph,
                                      std::string name,
                                      uint32_t edgeINCount = 0,
                                      StackID stack = StackID::PISDF);

        pisdf::DuplicateVertex *createDuplicate(pisdf::Graph *graph,
                                                std::string name,
                                                uint32_t edgeOUTCount = 0,
                                                StackID stack = StackID::PISDF);

        pisdf::RepeatVertex *createRepeat(pisdf::Graph *graph, std::string name, StackID stack = StackID::PISDF);

        pisdf::InitVertex *createInit(pisdf::Graph *graph, std::string name, StackID stack = StackID::PISDF);

        pisdf::EndVertex *createEnd(pisdf::Graph *graph, std::string name, StackID stack = StackID::PISDF);

        pisdf::ConfigVertex *createConfigActor(pisdf::Graph *graph,
                                               std::string name,
                                               uint32_t edgeINCount = 0,
                                               uint32_t edgeOUTCount = 0,
                                               StackID stack = StackID::PISDF);

        pisdf::InputInterface *setInputInterfaceName(pisdf::Graph *graph, uint32_t ix, std::string name);

        pisdf::OutputInterface *setOutputInterfaceName(pisdf::Graph *graph, uint32_t ix, std::string name);

        /* === Param API === */

        pisdf::Param *createStaticParam(pisdf::Graph *graph,
                                        std::string name,
                                        int64_t value,
                                        StackID stack = StackID::PISDF);

        pisdf::Param *createStaticParam(pisdf::Graph *graph,
                                        std::string name,
                                        std::string expression,
                                        StackID stack = StackID::PISDF);

        pisdf::DynamicParam *createDynamicParam(pisdf::Graph *graph,
                                                std::string name,
                                                StackID stack = StackID::PISDF);

        pisdf::DynamicParam *createDynamicParam(pisdf::Graph *graph,
                                                std::string name,
                                                std::string expression,
                                                StackID stack = StackID::PISDF);

        pisdf::Param *createInheritedParam(pisdf::Graph *graph,
                                           std::string name,
                                           pisdf::Param *parent,
                                           StackID stack = StackID::PISDF);

        /* === Edge API === */

        pisdf::Edge *createEdge(pisdf::Vertex *source,
                                uint32_t srcPortIx,
                                std::string srcRateExpression,
                                pisdf::Vertex *sink,
                                uint32_t snkPortIx,
                                std::string snkRateExpression,
                                StackID stack = StackID::PISDF);

        pisdf::Edge *createEdge(pisdf::Vertex *source,
                                uint32_t srcPortIx,
                                int64_t srcRate,
                                pisdf::Vertex *sink,
                                uint32_t snkPortIx,
                                int64_t snkRate,
                                StackID stack = StackID::PISDF);

        pisdf::Delay *createDelay(pisdf::Edge *edge,
                                  std::string delayExpression,
                                  pisdf::ExecVertex *setter = nullptr,
                                  uint32_t setterPortIx = 0,
                                  const std::string &setterRateExpression = "0",
                                  pisdf::ExecVertex *getter = nullptr,
                                  uint32_t getterPortIx = 0,
                                  const std::string &getterRateExpression = "0",
                                  bool persistent = true,
                                  StackID stack = StackID::PISDF);

        pisdf::Delay *createDelay(pisdf::Edge *edge,
                                  int64_t delayValue,
                                  pisdf::ExecVertex *setter = nullptr,
                                  uint32_t setterPortIx = 0,
                                  int64_t setterRate = 0,
                                  pisdf::ExecVertex *getter = nullptr,
                                  uint32_t getterPortIx = 0,
                                  int64_t getterRate = 0,
                                  bool persistent = true,
                                  StackID stack = StackID::PISDF);
    }
}

#endif //SPIDER2_PISDF_H
