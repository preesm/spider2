/**
 * Copyright or © or Copr. IETR/INSA - Rennes (2019 - 2020) :
 *
 * Florian Arrestier <florian.arrestier@insa-rennes.fr> (2019 - 2020)
 *
 * Spider 2.0 is a dataflow based runtime used to execute dynamic PiSDF
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
#include <memory>
#include <api/global-api.h>

/* === API methods === */

namespace spider {

    namespace api {

        /* === Graph API === */

        /**
         * @brief Creates a @refitem pisdf::Graph with no parent.
         * @param name            Name of the graph.
         * @param actorCount      Number of actors to reserve in the graph (can be extended later).
         * @param edgeCount       Number of edges to reserve in the graph (can be extended later).
         * @param paramCount      Number of params to reserve in the graph (can be extended later).
         * @param inIFCount       Number of input interfaces in the graph (can NOT be extended later).
         * @param outIFCount      Number of output interfaces in the graph (can NOT be extended later).
         * @param cfgActorCount   Number of actors to reserve in the graph (can be extended later).
         * @return pointer to the created @refitem pisdf::Graph.
         */
        pisdf::Graph *createGraph(std::string name,
                                  size_t actorCount = 0,
                                  size_t edgeCount = 0,
                                  size_t paramCount = 0,
                                  size_t inIFCount = 0,
                                  size_t outIFCount = 0,
                                  size_t cfgActorCount = 0);

        /**
         * @brief Destroy a given graph.
         * @param graph Pointer to the graph to destroy.
         */
        void destroyGraph(pisdf::Graph *graph);

        /**
         * @brief Get a @refitem pisdf::Vertex pointer from a @refitem pisdf::Graph pointer.
         * @param graph  Pointer to the graph.
         * @return converted pointer to @refitem pisdf::Vertex.
         */
        pisdf::Vertex *convertGraphToVertex(pisdf::Graph *graph);

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
         * @return pointer to the created @refitem pisdf::Graph.
         * @throws @refitem spider::Exception if parent graph is nullptr.
         */
        pisdf::Graph *createSubgraph(pisdf::Graph *graph,
                                     std::string name,
                                     size_t actorCount = 0,
                                     size_t edgeCount = 0,
                                     size_t paramCount = 0,
                                     size_t inIFCount = 0,
                                     size_t outIFCount = 0,
                                     size_t cfgActorCount = 0);

        /**
         * @brief Creates the proper vertex type using the spider::api.
         * @remark This method consider kernelIx is in range [0..N] with 0.
         * @param graph            Pointer to the graph in which the vertex should be created.
         * @param name             Name of the vertex.
         * @param inputEdgeCount   Number of input edge of the vertex.
         * @param outputEdgeCount  Number of output edge of the vertex.
         * @param type             VertexType of the vertex.
         * @param kernelIx         VertexType of the vertex.
         * @return pointer to the @refitem spider::pisdf::ExecVertex created.
         */
        pisdf::Vertex *createVertexFromType(pisdf::Graph *graph,
                                            std::string name,
                                            size_t inputEdgeCount,
                                            size_t outputEdgeCount,
                                            pisdf::VertexType type,
                                            size_t kernelIx = SIZE_MAX);

        /**
         * @brief Creates a @refitem pisdf::ExecVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeINCount   Number of input edges (can NOT be modified afterwards).
         * @param edgeOUTCount  Number of output edges (can NOT be modified afterwards).
         * @return pointer to the created @refitem pisdf::Vertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::Vertex *createVertex(pisdf::Graph *graph,
                                    std::string name,
                                    size_t edgeINCount = 0,
                                    size_t edgeOUTCount = 0);

        /**
         * @brief Creates a @refitem pisdf::NonExecVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeINCount   Number of input edges (can NOT be modified afterwards).
         * @param edgeOUTCount  Number of output edges (can NOT be modified afterwards).
         * @return pointer to the created @refitem pisdf::Vertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::Vertex *
        createNonExecVertex(pisdf::Graph *graph, std::string name, size_t edgeINCount = 0, size_t edgeOUTCount = 0);

        /**
         * @brief Creates a @refitem pisdf::ForkVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeOUTCount  Number of output edges (can NOT be modified afterwards).
         * @return pointer to the created @refitem pisdf::ForkVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::Vertex *createFork(pisdf::Graph *graph, std::string name, size_t edgeOUTCount = 0);

        /**
         * @brief Creates a @refitem pisdf::JoinVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeINCount   Number of input edges (can NOT be modified afterwards).
         * @return pointer to the created @refitem pisdf::JoinVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::Vertex *createJoin(pisdf::Graph *graph, std::string name, size_t edgeINCount = 0);

        /**
         * @brief Creates a @refitem pisdf::HeadVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeINCount   Number of input edges (can NOT be modified afterwards).
         * @return pointer to the created @refitem pisdf::HeadVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::Vertex *createHead(pisdf::Graph *graph, std::string name, size_t edgeINCount = 0);

        /**
         * @brief Creates a @refitem pisdf::TailVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeINCount   Number of input edges (can NOT be modified afterwards).
         * @return pointer to the created @refitem pisdf::TailVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::Vertex *createTail(pisdf::Graph *graph, std::string name, size_t edgeINCount = 0);

        /**
         * @brief Creates a @refitem pisdf::DuplicateVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeOUTCount  Number of output edges (can NOT be modified afterwards).
         * @return pointer to the created @refitem pisdf::DuplicateVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::Vertex *createDuplicate(pisdf::Graph *graph, std::string name, size_t edgeOUTCount = 0);

        /**
         * @brief Creates a @refitem pisdf::RepeatVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @return pointer to the created @refitem pisdf::RepeatVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::Vertex *createRepeat(pisdf::Graph *graph, std::string name);

        /**
         * @brief Creates a @refitem pisdf::InitVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @return pointer to the created @refitem pisdf::InitVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::Vertex *createInit(pisdf::Graph *graph, std::string name);

        /**
         * @brief Creates a @refitem pisdf::EndVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @return pointer to the created @refitem pisdf::EndVertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::Vertex *createEnd(pisdf::Graph *graph, std::string name);

        /**
         * @brief Creates a @refitem pisdf::ConfigVertex.
         * @param graph         Pointer to the parent graph the vertex should be added.
         * @param name          Name of the vertex.
         * @param edgeINCount   Number of input edges (can NOT be modified afterwards).
         * @param edgeOUTCount  Number of output edges (can NOT be modified afterwards).
         * @return pointer to the created @refitem pisdf::Vertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         */
        pisdf::Vertex *
        createConfigActor(pisdf::Graph *graph, std::string name, size_t edgeINCount = 0, size_t edgeOUTCount = 0);

        /**
         * @brief Creates an external input interface to convey data from outside the dataflow application.
         * @warning No check is made on the provided buffer size, it is the user responsability to ensure
         *          the size will match the execution need.
         * @param graph    Pointer to the parent graph the vertex should be added.
         * @param name     Name of the interface.
         * @param buffer   Buffer to transmit data.
         * @return pointer to the created @refitem pisdf::Vertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         * @throws @refitem spider::Exception if the provided buffer is nullptr.
         */
        pisdf::Vertex *createExternInputInterface(pisdf::Graph *graph, std::string name, void *buffer);

        /**
         * @brief Creates an external input interface to convey data from outside the dataflow application.
         * @warning No check is made on the provided buffer size, it is the user responsability to ensure
         *          the size will match the execution need.
         * @param graph    Pointer to the parent graph the vertex should be added.
         * @param name     Name of the interface.
         * @param buffer   Buffer to transmit data.
         * @return pointer to the created @refitem pisdf::Vertex.
         * @throws @refitem spider::Exception if the parent graph is nullptr.
         * @throws @refitem spider::Exception if the provided buffer is nullptr.
         */
        pisdf::Vertex *createExternOutputInterface(pisdf::Graph *graph, std::string name, void *buffer);

        /**
         * @brief Get an input @refitem pisdf::Interface of a given graph.
         * @param graph Pointer to the graph the interface belong.
         * @param ix    Index of the input interface.
         * @return pointer to the corresponding @refitem pisdf::Vertex.
         * @throws spider::Exception if graph is nullptr or interface is not found.
         */
        pisdf::Vertex *getInputInterface(const pisdf::Graph *graph, size_t ix);

        /**
         * @brief Get an input @refitem pisdf::Interface of a given graph.
         * @param graph Pointer to the graph the interface belong.
         * @param ix    Index of the input interface.
         * @return pointer to the corresponding @refitem pisdf::Vertex.
         * @throws spider::Exception if graph is nullptr or interface is not found.
         */
        pisdf::Vertex *getOutputInterface(const pisdf::Graph *graph, size_t ix);

        /**
         * @brief Change the name of an input @refitem pisdf::Interface.
         * @param graph  Pointer to the graph the interface belong.
         * @param ix     Index of the input interface.
         * @param name   Name to be set.
         * @return pointer to the corresponding @refitem pisdf::Vertex.
         * @throws spider::Exception if graph is nullptr or interface is not found.
         */
        pisdf::Vertex *setInputInterfaceName(const pisdf::Graph *graph, size_t ix, std::string name);

        /**
         * @brief Change the name of an output @refitem pisdf::Interface.
         * @param graph  Pointer to the graph the interface belong.
         * @param ix     Index of the input interface.
         * @param name   Name to be set.
         * @return pointer to the corresponding @refitem pisdf::Vertex.
         * @throws spider::Exception if graph is nullptr or interface is not found.
         */
        pisdf::Vertex *setOutputInterfaceName(const pisdf::Graph *graph, size_t ix, std::string name);

        /* === Param API === */

        /**
         * @brief Creates a fully static parameter (@refitem pisdf::Param with fixed value).
         * @remark If graph is not nullptr, the parameter will be added to it.
         * @param graph   Pointer to the graph the parameter should be added.
         * @param name    Name of the parameter (will be transformed to lowercase).
         * @param value   Value of the parameter.
         * @return pointer to the created @refitem pisdf::Param.
         */
        std::shared_ptr<pisdf::Param> createStaticParam(pisdf::Graph *graph, std::string name, int64_t value);

        /**
         * @brief Creates a fully dynamic parameter (@refitem pisdf::DynamicParam).
         * @remark If graph is not nullptr, the parameter will be added to it.
         * @param graph      Pointer to the graph the parameter should be added.
         * @param name       Name of the parameter (will be transformed to lowercase).
         * @return pointer to the created @refitem pisdf::DynamicParam.
         */
        std::shared_ptr<spider::pisdf::Param> createDynamicParam(pisdf::Graph *graph, std::string name);

        /**
         * @brief Creates a @refitem pisdf::Param with expression.
         * @remark If expression is dynamic then a @refitem pisdf::DynamicParam is created.
         * @remark If graph is not nullptr, the parameter will be added to it.
         * @param graph      Pointer to the graph the parameter should be added.
         * @param name       Name of the parameter (will be transformed to lowercase).
         * @param expression Expression of the parameter.
         * @return pointer to the created @refitem pisdf::Param.
         */
        std::shared_ptr<pisdf::Param> createDerivedParam(pisdf::Graph *graph, std::string name, std::string expression);

        /**
         * @brief Creates an inherited parameter. (@refitem pisdf::InheritedParam).
         * @remark If the parent parameter is static, then a static parameter will be created instead.
         * @remark If graph is not nullptr, the parameter will be added to it.
         * @param graph   Pointer to the graph the parameter should be added.
         * @param name    Name of the parameter (will be transformed to lowercase).
         * @param parent  Pointer to the parent parameter.
         * @return pointer to the created @refitem pisdf::Param.
         * @throws @refitem spider::Exception if parent parameter is nullptr.
         */
        std::shared_ptr<spider::pisdf::Param>
        createInheritedParam(pisdf::Graph *graph, std::string name, std::shared_ptr<pisdf::Param> parent);

        /**
         * @brief Add an input parameter to a given Vertex.
         * @remark If param or vertex is nullptr, nothing happen.
         * @param vertex  Pointer to the vertex to evaluate.
         * @param param   Pointer to the parameter to add.
         */
        void addInputParamToVertex(pisdf::Vertex *vertex, std::shared_ptr<spider::pisdf::Param> param);

        /**
         * @brief Add an input parameter to a given Vertex for its refinement.
         * @remark If param or vertex is nullptr, nothing happen.
         * @remark A secondary call to vertex->addInputParam is done.
         * @param vertex  Pointer to the vertex to evaluate.
         * @param param   Pointer to the parameter to add.
         */
        void addInputRefinementParamToVertex(pisdf::Vertex *vertex, std::shared_ptr<spider::pisdf::Param> param);

        /**
         * @brief Add an output parameter to a given Vertex.
         * @remark If param or vertex is nullptr, nothing happen.
         * @param vertex  Pointer to the vertex to evaluate.
         * @param param   Pointer to the parameter to add.
         * @throw spider::Exception if vertex is not of type @refitem VertexType::ConfigVertex.
         */
        void addOutputParamToVertex(pisdf::Vertex *vertex, std::shared_ptr<spider::pisdf::Param> param);

        /* === Edge API === */

        /**
         * @brief Creates an @refitem pisdf::Edge between two @refitem pisdf::Vertex.
         * @param source             Pointer to the source Vertex.
         * @param srcPortIx          Index of the edge in the outputEdgeArray of the source Vertex.
         * @param srcRateExpression  Expression of the production rate (can be parameterized).
         * @param sink               Pointer to the sink Vertex.
         * @param snkPortIx          Index of the edge in the inputEdgeArray of the sink Vertex.
         * @param snkRateExpression  Expression of the consumption rate (can be parameterized).
         * @return pointer to the created @refitem pisdf::Edge.
         * @throws spider::Exception if either source or sink is nullptr or if source and sink do not belong to the same graph.
         */
        pisdf::Edge *createEdge(pisdf::Vertex *source,
                                size_t srcPortIx,
                                std::string srcRateExpression,
                                pisdf::Vertex *sink,
                                size_t snkPortIx,
                                std::string snkRateExpression);

        /**
         * @brief Creates an @refitem pisdf::Edge between two @refitem pisdf::Vertex.
         * @param source    Pointer to the source Vertex.
         * @param srcPortIx Index of the edge in the outputEdgeArray of the source Vertex.
         * @param srcRate   Value of the production rate.
         * @param sink      Pointer to the sink Vertex.
         * @param snkPortIx Index of the edge in the inputEdgeArray of the sink Vertex.
         * @param snkRate   Value of the consumption rate.
         * @return pointer to the created @refitem pisdf::Edge.
         * @throws spider::Exception if either source or sink is nullptr or if source and sink do not belong to the same graph.
         */
        pisdf::Edge *createEdge(pisdf::Vertex *source,
                                size_t srcPortIx,
                                int64_t srcRate,
                                pisdf::Vertex *sink,
                                size_t snkPortIx,
                                int64_t snkRate);

        /**
         * @brief Creates a fully-persistent @refitem pisdf::Delay on a given @refitem pisdf::Edge.
         * @remarks
         * - If the delay is in a hierarchy it will be put up to the top-level.
         * - For more details on the persistence of the delays: https://hal.archives-ouvertes.fr/hal-01850252/document
         * @param edge             Pointer to the edge the Delay should be created on.
         * @param delayExpression  Expression of the delay (can be parameterized but NON dynamic).
         * @return pointer to the created @refitem pisdf::Delay.
         */
        pisdf::Delay *
        createPersistentDelay(pisdf::Edge *edge, std::string delayExpression);

        /**
         * @brief Creates a locally-persistent @refitem pisdf::Delay on a given @refitem pisdf::Edge.
         * @remarks
         * - If the delay is in a hierarchy it will be put up to the top-level.
         * - For more details on the persistence of the delays: https://hal.archives-ouvertes.fr/hal-01850252/document
         * @param edge             Pointer to the edge the Delay should be created on.
         * @param delayExpression  Expression of the delay (can be parameterized but NON dynamic).
         * @param levelCount       Number of levels the delay should persist.
         * @return pointer to the created @refitem pisdf::Delay.
         */
        pisdf::Delay *
        createLocalPersistentDelay(pisdf::Edge *edge, std::string delayExpression, int_fast32_t levelCount = 1);

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
         * @return pointer to the created @refitem pisdf::Delay.
         */
        pisdf::Delay *createLocalDelay(pisdf::Edge *edge,
                                       std::string delayExpression,
                                       pisdf::Vertex *setter = nullptr,
                                       size_t setterPortIx = 0,
                                       std::string setterRateExpression = "0",
                                       pisdf::Vertex *getter = nullptr,
                                       size_t getterPortIx = 0,
                                       std::string getterRateExpression = "0");
    }
}

#endif //SPIDER2_PISDF_API_H
