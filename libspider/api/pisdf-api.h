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
#ifndef SPIDER2_PISDF_API_H
#define SPIDER2_PISDF_API_H

/* === Include(s) === */

#include <cstdint>
#include <string>
#include <api/global-api.h>

/* === API methods === */

namespace spider {


    namespace pisdf {
        /**
         * @brief Get the user defined graph of the Spider session.
         * @return
         */
        Graph *&applicationGraph();
    }

    namespace api {

        /* === Graph API === */

        /**
         * @brief Creates the main user application graph.
         * @remark this call is equivalent to call spider::api::createSubgraph(pisdf::applicationGraph(), ...).
         * @param name            Name of the graph.
         * @param actorCount      Number of actors to reserve in the graph (can be extended later).
         * @param edgeCount       Number of edges to reserve in the graph (can be extended later).
         * @param paramCount      Number of params to reserve in the graph (can be extended later).
         * @param inIFCount       Number of input interfaces in the graph (can NOT be extended later).
         * @param outIFCount      Number of output interfaces in the graph (can NOT be extended later).
         * @param cfgActorCount   Number of actors to reserve in the graph (can be extended later).
         * @param stack           Stack on which to create the graph.
         * @return pointer to the created @refitem pisdf::Graph.
         * @throws @refitem spider::Exception if a user graph already exists.
         */
        pisdf::Graph *createUserApplicationGraph(std::string name,
                                                 uint32_t actorCount = 0,
                                                 uint32_t edgeCount = 0,
                                                 uint32_t paramCount = 0,
                                                 uint32_t inIFCount = 0,
                                                 uint32_t outIFCount = 0,
                                                 uint32_t cfgActorCount = 0,
                                                 StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::Graph with no parent.
         * @param name            Name of the graph.
         * @param actorCount      Number of actors to reserve in the graph (can be extended later).
         * @param edgeCount       Number of edges to reserve in the graph (can be extended later).
         * @param paramCount      Number of params to reserve in the graph (can be extended later).
         * @param inIFCount       Number of input interfaces in the graph (can NOT be extended later).
         * @param outIFCount      Number of output interfaces in the graph (can NOT be extended later).
         * @param cfgActorCount   Number of actors to reserve in the graph (can be extended later).
         * @param stack           Stack on which to create the graph.
         * @return pointer to the created @refitem pisdf::Graph.
         */
        pisdf::Graph *createGraph(std::string name,
                                  uint32_t actorCount = 0,
                                  uint32_t edgeCount = 0,
                                  uint32_t paramCount = 0,
                                  uint32_t inIFCount = 0,
                                  uint32_t outIFCount = 0,
                                  uint32_t cfgActorCount = 0,
                                  StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::Graph with a parent graph.
         * @param graph           Pointer to the parent graph.
         * @param name            Name of the graph.
         * @param actorCount      Number of actors to reserve in the graph (can be extended later).
         * @param edgeCount       Number of edges to reserve in the graph (can be extended later).
         * @param paramCount      Number of params to reserve in the graph (can be extended later).
         * @param inIFCount       Number of input interfaces in the graph (can NOT be extended later).
         * @param outIFCount      Number of output interfaces in the graph (can NOT be extended later).
         * @param cfgActorCount   Number of actors to reserve in the graph (can be extended later).
         * @param stack           Stack on which to create the graph.
         * @return pointer to the created @refitem pisdf::Graph.
         * @throws @refitem spider::Exception if parent graph is nullptr.
         */
        pisdf::Graph *createSubgraph(pisdf::Graph *graph,
                                     std::string name,
                                     uint32_t actorCount = 0,
                                     uint32_t edgeCount = 0,
                                     uint32_t paramCount = 0,
                                     uint32_t inIFCount = 0,
                                     uint32_t outIFCount = 0,
                                     uint32_t cfgActorCount = 0,
                                     StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::Vertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeINCount   Number of input edges (can NOT be modified afterwards).
         * @param edgeOUTCount  Number of output edges (can NOT be modified afterwards).
         * @param stack         Stack on which the vertex should be created.
         * @return pointer to the created @refitem pisdf::Vertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::ExecVertex *createVertex(pisdf::Graph *graph,
                                        std::string name,
                                        uint32_t edgeINCount = 0,
                                        uint32_t edgeOUTCount = 0,
                                        StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::ForkVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeOUTCount  Number of output edges (can NOT be modified afterwards).
         * @param stack         Stack on which the vertex should be created.
         * @return pointer to the created @refitem pisdf::ForkVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::ExecVertex *createFork(pisdf::Graph *graph,
                                      std::string name,
                                      uint32_t edgeOUTCount = 0,
                                      StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::JoinVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeINCount   Number of input edges (can NOT be modified afterwards).
         * @param stack         Stack on which the vertex should be created.
         * @return pointer to the created @refitem pisdf::JoinVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::ExecVertex *createJoin(pisdf::Graph *graph,
                                      std::string name,
                                      uint32_t edgeINCount = 0,
                                      StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::HeadVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeINCount   Number of input edges (can NOT be modified afterwards).
         * @param stack         Stack on which the vertex should be created.
         * @return pointer to the created @refitem pisdf::HeadVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::ExecVertex *createHead(pisdf::Graph *graph,
                                      std::string name,
                                      uint32_t edgeINCount = 0,
                                      StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::TailVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeINCount   Number of input edges (can NOT be modified afterwards).
         * @param stack         Stack on which the vertex should be created.
         * @return pointer to the created @refitem pisdf::TailVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::ExecVertex *createTail(pisdf::Graph *graph,
                                      std::string name,
                                      uint32_t edgeINCount = 0,
                                      StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::DuplicateVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeOUTCount  Number of output edges (can NOT be modified afterwards).
         * @param stack         Stack on which the vertex should be created.
         * @return pointer to the created @refitem pisdf::DuplicateVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::ExecVertex *createDuplicate(pisdf::Graph *graph,
                                           std::string name,
                                           uint32_t edgeOUTCount = 0,
                                           StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::RepeatVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param stack         Stack on which the vertex should be created.
         * @return pointer to the created @refitem pisdf::RepeatVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::ExecVertex *createRepeat(pisdf::Graph *graph, std::string name, StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::InitVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param stack         Stack on which the vertex should be created.
         * @return pointer to the created @refitem pisdf::InitVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::ExecVertex *createInit(pisdf::Graph *graph, std::string name, StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::EndVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param stack         Stack on which the vertex should be created.
         * @return pointer to the created @refitem pisdf::EndVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::ExecVertex *createEnd(pisdf::Graph *graph, std::string name, StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::ConfigVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeINCount   Number of input edges (can NOT be modified afterwards).
         * @param edgeOUTCount  Number of output edges (can NOT be modified afterwards).
         * @param stack         Stack on which the vertex should be created.
         * @return pointer to the created @refitem pisdf::ConfigVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::ExecVertex *createConfigActor(pisdf::Graph *graph,
                                             std::string name,
                                             uint32_t edgeINCount = 0,
                                             uint32_t edgeOUTCount = 0,
                                             StackID stack = StackID::PISDF);

        /**
         * @brief Change the name of an @refitem pisdf::InputInterface.
         * @param graph  Pointer to the graph the interface belong.
         * @param ix     Index of the input interface.
         * @param name   Name to be set.
         * @return pointer to the corresponding @refitem InputInterface.
         * @throws spider::Exception if graph is nullptr or interface is not found.
         */
        pisdf::InputInterface *setInputInterfaceName(pisdf::Graph *graph, uint32_t ix, std::string name);

        /**
         * @brief Change the name of an @refitem pisdf::OutputInterface.
         * @param graph  Pointer to the graph the interface belong.
         * @param ix     Index of the input interface.
         * @param name   Name to be set.
         * @return pointer to the corresponding @refitem OutputInterface.
         * @throws spider::Exception if graph is nullptr or interface is not found.
         */
        pisdf::OutputInterface *setOutputInterfaceName(pisdf::Graph *graph, uint32_t ix, std::string name);

        /* === Param API === */

        /**
         * @brief Creates a static @refitem pisdf::Param with fixed value.
         * @remark If graph is not nullptr, the parameter will be added to it.
         * @param graph   Pointer to the graph the parameter should be added.
         * @param name    Name of the parameter (will be transformed to lowercase).
         * @param value   Value of the parameter.
         * @param stack   Stack on which the parameter should be created.
         * @return pointer to the created @refitem pisdf::Param.
         */
        pisdf::Param *createStaticParam(pisdf::Graph *graph,
                                        std::string name,
                                        int64_t value,
                                        StackID stack = StackID::PISDF);

        /**
         * @brief Creates a static @refitem pisdf::Param with static expression.
         * @remark If graph is not nullptr, the parameter will be added to it.
         * @param graph      Pointer to the graph the parameter should be added.
         * @param name       Name of the parameter (will be transformed to lowercase).
         * @param expression Expression of the parameter.
         * @param stack      Stack on which the parameter should be created.
         * @return pointer to the created @refitem pisdf::Param.
         */
        pisdf::Param *createStaticParam(pisdf::Graph *graph,
                                        std::string name,
                                        std::string expression,
                                        StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::DynamicParam.
         * @remark If graph is not nullptr, the parameter will be added to it.
         * @param graph      Pointer to the graph the parameter should be added.
         * @param name       Name of the parameter (will be transformed to lowercase).
         * @param stack      Stack on which the parameter should be created.
         * @return pointer to the created @refitem pisdf::DynamicParam.
         */
        pisdf::DynamicParam *createDynamicParam(pisdf::Graph *graph,
                                                std::string name,
                                                StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::DynamicParam with dynamic expression.
         * @remark If graph is not nullptr, the parameter will be added to it.
         * @param graph      Pointer to the graph the parameter should be added.
         * @param name       Name of the parameter (will be transformed to lowercase).
         * @param expression Expression of the parameter.
         * @param stack      Stack on which the parameter should be created.
         * @return pointer to the created @refitem pisdf::DynamicParam.
         */
        pisdf::DynamicParam *createDynamicParam(pisdf::Graph *graph,
                                                std::string name,
                                                std::string expression,
                                                StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::InheritedParam.
         * @remark If the parent parameter is static, then a static parameter will be created instead.
         * @remark If graph is not nullptr, the parameter will be added to it.
         * @param graph   Pointer to the graph the parameter should be added.
         * @param name    Name of the parameter (will be transformed to lowercase).
         * @param parent  Pointer to the parent parameter.
         * @param stack   Stack on which the parameter should be created.
         * @return pointer to the created @refitem pisdf::Param.
         * @throws @refitem spider::Exception if parent parameter is nullptr.
         */
        pisdf::Param *createInheritedParam(pisdf::Graph *graph,
                                           std::string name,
                                           pisdf::Param *parent,
                                           StackID stack = StackID::PISDF);

        /* === Edge API === */

        /**
         * @brief Creates an @refitem pisdf::Edge between two @refitem pisdf::Vertex.
         * @param source             Pointer to the source Vertex.
         * @param srcPortIx          Index of the edge in the outputEdgeArray of the source Vertex.
         * @param srcRateExpression  Expression of the production rate (can be parameterized).
         * @param sink               Pointer to the sink Vertex.
         * @param snkPortIx          Index of the edge in the inputEdgeArray of the sink Vertex.
         * @param snkRateExpression  Expression of the consumption rate (can be parameterized).
         * @param stack              Stack on which the Edge should be created.
         * @return pointer to the created @refitem pisdf::Edge.
         * @throws spider::Exception if either source or sink is nullptr or if source and sink do not belong to the same graph.
         */
        pisdf::Edge *createEdge(pisdf::Vertex *source,
                                size_t srcPortIx,
                                std::string srcRateExpression,
                                pisdf::Vertex *sink,
                                size_t snkPortIx,
                                std::string snkRateExpression,
                                StackID stack = StackID::PISDF);

        /**
         * @brief Creates an @refitem pisdf::Edge between two @refitem pisdf::Vertex.
         * @param source    Pointer to the source Vertex.
         * @param srcPortIx Index of the edge in the outputEdgeArray of the source Vertex.
         * @param srcRate   Value of the production rate.
         * @param sink      Pointer to the sink Vertex.
         * @param snkPortIx Index of the edge in the inputEdgeArray of the sink Vertex.
         * @param snkRate   Value of the consumption rate.
         * @param stack     Stack on which the Edge should be created.
         * @return pointer to the created @refitem pisdf::Edge.
         * @throws spider::Exception if either source or sink is nullptr or if source and sink do not belong to the same graph.
         */
        pisdf::Edge *createEdge(pisdf::Vertex *source,
                                size_t srcPortIx,
                                int64_t srcRate,
                                pisdf::Vertex *sink,
                                size_t snkPortIx,
                                int64_t snkRate,
                                StackID stack = StackID::PISDF);

        /**
         * @brief Creates a @refitem pisdf::Delay on a @refitem pisdf::Edge.
         * @remark For more details on the setter/getter and persistence of the delays:
         *         https://hal.archives-ouvertes.fr/hal-01850252/document
         * @param edge                  Pointer to the edge the Delay should be created on.
         * @param delayExpression       Expression of the delay (can be parameterized).
         * @param setter                Pointer to the setter vertex of the delay (can be nullptr).
         * @param setterPortIx          Index of the edge in the outputEdgeArray of the setter vertex.
         * @param setterRateExpression  Expression of the production rate on the edge from the setter to the delay.
         * @param getter                Pointer to the getter vertex of the delay (can be nullptr).
         * @param getterPortIx          Index of the edge in the inputEdgeArray of the getter vertex.
         * @param getterRateExpression  Expression of the consumption rate on the edge from the delay to the getter.
         * @param persistent            Persistence property of the delay (true = fully persistent, false = non persistent).
         * @param stack                 Stack on which the delay should be created.
         * @return pointer to the created @refitem pisdf::Delay.
         */
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

        /**
         * @brief Creates a @refitem pisdf::Delay on a @refitem pisdf::Edge.
         * @remark For more details on the setter/getter and persistence of the delays:
         *         https://hal.archives-ouvertes.fr/hal-01850252/document
         * @param edge          Pointer to the edge the Delay should be created on.
         * @param delayValue    Value of the delay.
         * @param setter        Pointer to the setter vertex of the delay (can be nullptr).
         * @param setterPortIx  Index of the edge in the outputEdgeArray of the setter vertex.
         * @param setterRate    Value of the production rate on the edge from the setter to the delay.
         * @param getter        Pointer to the getter vertex of the delay (can be nullptr).
         * @param getterPortIx  Index of the edge in the inputEdgeArray of the getter vertex.
         * @param getterRate    Value of the consumption rate on the edge from the delay to the getter.
         * @param persistent    Persistence property of the delay (true = fully persistent, false = non persistent).
         * @param stack         Stack on which the delay should be created.
         * @return pointer to the created @refitem pisdf::Delay.
         */
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

        /**
         * @brief Creates a fully-persistent @refitem pisdf::Delay on a given @refitem pisdf::Edge.
         * @remarks
         * - If the delay is in a hierarchy it will be put up to the top-level.
         * - For more details on the persistence of the delays: https://hal.archives-ouvertes.fr/hal-01850252/document
         * @param edge             Pointer to the edge the Delay should be created on.
         * @param delayExpression  Expression of the delay (can be parameterized but NON dynamic).
         * @param stack            Stack on which the delay should be created.
         * @return pointer to the created @refitem pisdf::Delay.
         */
        pisdf::Delay *
        createPersistentDelay(pisdf::Edge *edge, std::string delayExpression, StackID stack = StackID::PISDF);

        /**
         * @brief Creates a locally-persistent @refitem pisdf::Delay on a given @refitem pisdf::Edge.
         * @remarks
         * - If the delay is in a hierarchy it will be put up to the top-level.
         * - For more details on the persistence of the delays: https://hal.archives-ouvertes.fr/hal-01850252/document
         * @param edge             Pointer to the edge the Delay should be created on.
         * @param delayExpression  Expression of the delay (can be parameterized but NON dynamic).
         * @param levelCount       Number of levels the delay should persist.
         * @param stack            Stack on which the delay should be created.
         * @return pointer to the created @refitem pisdf::Delay.
         */
        pisdf::Delay *
        createLocalPersistentDelay(pisdf::Edge *edge, std::string delayExpression, int32_t levelCount = 1,
                                   StackID stack = StackID::PISDF);

        /**
         * @brief Creates a non-persistent @refitem pisdf::Delay on a given @refitem pisdf::Edge.
         * @remarks
         * - If the delay is in a hierarchy it will be put up to the top-level.
         * - For more details on the persistence of the delays: https://hal.archives-ouvertes.fr/hal-01850252/document
         * @param edge                  Pointer to the edge the Delay should be created on.
         * @param delayExpression       Expression of the delay (can be parameterized but NON dynamic).
         * @param setter                Pointer to the setter vertex of the delay (can be nullptr).
         * @param setterPortIx          Index of the edge in the outputEdgeArray of the setter vertex.
         * @param setterRateExpression  Expression of the production rate on the edge from the setter to the delay.
         * @param getter                Pointer to the getter vertex of the delay (can be nullptr).
         * @param getterPortIx          Index of the edge in the inputEdgeArray of the getter vertex.
         * @param getterRateExpression  Expression of the consumption rate on the edge from the delay to the getter.
         * @param stack                 Stack on which the delay should be created.
         * @return pointer to the created @refitem pisdf::Delay.
         */
        pisdf::Delay *createLocalDelay(pisdf::Edge *edge,
                                       std::string delayExpression,
                                       pisdf::ExecVertex *setter = nullptr,
                                       uint32_t setterPortIx = 0,
                                       std::string setterRateExpression = "0",
                                       pisdf::ExecVertex *getter = nullptr,
                                       uint32_t getterPortIx = 0,
                                       std::string getterRateExpression = "0",
                                       StackID stack = StackID::PISDF);
    }
}

#endif //SPIDER2_PISDF_API_H
